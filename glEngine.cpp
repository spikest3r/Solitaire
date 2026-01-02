#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "glEngine.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "save.h"
#include <stdio.h>
#include <thread>

#define UNDO 3001
#define NEW_GAME 1001
#define LOAD_GAME 2001
#define SAVE_GAME 2002
#define TOGGLE_BG 3005

#define WM_UPDATE_TIMER (WM_USER+1)

// EXPERIMENTAL
// This is flag to enable experimental, in development features
// Such featurures might be unstable or very buggy
// #define EXPERIMENTAL

const char* vertexShader = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform mat4 uMVP;
uniform vec4 uUVRect;
uniform vec2 quadSize;
out vec2 TexCoord;
void main()
{
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    vec2 localUV = (aPos + quadSize*0.5)/quadSize;
    TexCoord = mix(uUVRect.xy, uUVRect.zw, localUV);
}
)";

const char* fragmentShader = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D uTexture;
void main()
{
    FragColor = texture(uTexture,TexCoord);
}
)";

float cardVertices[8];
float bgVertices[8] = {
    0.5f,0.5f,
    0.5f,-0.5f,
    -0.5f,-0.5f,
    -0.5f,0.5f,
};
unsigned int cardIndices[] = { 0,1,2, 2,3,0 };

bool Engine::loadAtlas() {
    DWORD atlasSize;
    void* rawAtlasData = resources->loadResource(100, L"PNG", &atlasSize);
    if (!rawAtlasData) {
        MessageBoxA(NULL, "Failed to load resource","Fatal error",MB_OK|MB_ICONERROR);
        return false;
    }

    int w, h, c;
    unsigned char* atlas = stbi_load_from_memory(
        reinterpret_cast<unsigned char*>(rawAtlasData),
        atlasSize,
        &w,
        &h,
        &c,
        0
    );
    if (!atlas) return false;

    int cols = 13, rows = 5;
    cardW = w / (float)cols;
    cardH = h / (float)rows;
    float cardAspect = cardH / cardW;

    float windowAspect = windowWidth / (float)windowHeight;

    float quadW = 1.0f;
    float quadH = quadW * cardAspect;
    quadW /= windowAspect;

    float scale = 0.3f;
    quadW *= scale;
    quadH *= scale;

    cardVertices[0] = -quadW / 2; cardVertices[1] = -quadH / 2;
    cardVertices[2] = quadW / 2; cardVertices[3] = -quadH / 2;
    cardVertices[4] = quadW / 2; cardVertices[5] = quadH / 2;
    cardVertices[6] = -quadW / 2; cardVertices[7] = quadH / 2;

    glGenTextures(1, &atlasTexture);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);

    GLenum format = (c == 1) ? GL_RED : (c == 3) ? GL_RGB : GL_RGBA;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, atlas);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(atlas);
    return true;
}

bool Engine::loadBackgrounds() {
    const int backgrounds = 3;
    glGenTextures(3, bgTexture);
    for (int i = 0; i < backgrounds; i++) {
        DWORD size;
        void* data = resources->loadResource(201 + i,L"PNG",&size);
        if (!data) {
            MessageBoxA(NULL, "Failed to load resource!", "Fatal error", MB_OK | MB_ICONERROR);
            return false;
        }
        int w, h, c;
        unsigned char* texData = stbi_load_from_memory(reinterpret_cast<unsigned char*>(data), size, &w, &h, &c, 0);
        if (!texData) {
            MessageBoxA(NULL, "Failed to load resource!", "Fatal error", MB_OK | MB_ICONERROR);
            return false;
        }
        GLenum format = (c == 1) ? GL_RED : (c == 3) ? GL_RGB : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, bgTexture[i]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, texData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        stbi_image_free(texData);
    }

    glGenVertexArrays(1, &backgroundQuad.VAO);
    glBindVertexArray(backgroundQuad.VAO);
    glGenBuffers(1, &backgroundQuad.VBO);
    glGenBuffers(1, &backgroundQuad.EBO);

    glBindBuffer(GL_ARRAY_BUFFER, backgroundQuad.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bgVertices), bgVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backgroundQuad.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cardIndices), cardIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    selectedBackground = 0;

    return true;
}

CardUV Engine::getCardUV(int rank, int s) {
    const int cols = 13, rows = 5;
    int suit = 4-s;
    float uPerCard = 1.0f / cols;
    float vPerCard = 1.0f / rows;
    CardUV card;
    card.u0 = rank * uPerCard;
    card.u1 = (rank + 1) * uPerCard;
    card.v0 = suit * vPerCard;
    card.v1 = (suit + 1) * vPerCard;
    return card;
}

void Engine::TimerThreadFunc() {
    while (timerRunning) { // invalidate thread with overriding time variable
        if (!pauseTimer) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            time++;
            PostMessage(hwnd, WM_UPDATE_TIMER, 0, 0);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

Engine::Engine(const char* name, int w, int h, bool* success) {
    windowWidth = w;
    windowHeight = h;

    resources = new Resource("solitaire.dll", success);
    if (!*success) return;

    stbi_set_flip_vertically_on_load(true);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(w, h, name, NULL, NULL);
    if (!window) {
        MessageBoxA(NULL,"Failed to create window!","Fatal engine error",MB_OK|MB_ICONERROR);
        *success = false;
        return;
    }
    initWinapi();

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        MessageBoxA(NULL, "Failed to initialize OpenGl context!", "Fatal engine error", MB_OK | MB_ICONERROR);
        *success = false;
        return;
    }

    glViewport(0, 0, w, h);
    if (!loadAtlas()) {
        *success = false;
        return;
    }
    if (!loadBackgrounds()) {
        *success = false;
        return;
    }
    if (!loadSounds()) {
        MessageBoxA(NULL, "Failed to load resource", "Fatal error", MB_OK | MB_ICONERROR);
        *success = false;
        return;
    }

    proj = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);

    glGenVertexArrays(1, &cardQuad.VAO);
    glBindVertexArray(cardQuad.VAO);
    glGenBuffers(1, &cardQuad.VBO);
    glGenBuffers(1, &cardQuad.EBO);

    glBindBuffer(GL_ARRAY_BUFFER, cardQuad.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cardVertices), cardVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cardQuad.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cardIndices), cardIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    textureShader = Shader(vertexShader, fragmentShader);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(false);

    texLoc = glGetUniformLocation(textureShader.ID, "uTexture");
    quadSizeLoc = glGetUniformLocation(textureShader.ID, "quadSize");
    uvLoc = glGetUniformLocation(textureShader.ID, "uUVRect");

    initCards();

    verticalSpacing = (cardVertices[5] - cardVertices[1]) * 0.12;

#ifdef EXPERIMENTAL
    MessageBoxA(hwnd, "This build was compiled with experimental features!\nPlease be aware of bugs, errors or undefined behavior.", "Experimental features build", MB_OK | MB_ICONWARNING);
#endif

    if (SaveExists()) {
        auto message = MessageBoxA(hwnd, "Saved game has been found. Do you wish to load it?", "Load saved game?", MB_ICONINFORMATION | MB_YESNO);
        if (message == IDYES) {
            LoadGame();
        }
        else {
            ResetGameTimer(0);
        }
    }
    else {
        ResetGameTimer(0);
    }

    statusMessageThread = std::thread(&Engine::StatusThreadFunc, this);
}

bool Engine::SaveExists() {
    if (std::ifstream("save.bin")) return true;
    else return false;
}

void Engine::loop() {
    update();
    render();

    glfwSwapBuffers(window);
    glfwWaitEvents();
}

void Engine::update() {
    currentMoveIsLegal = false;

    double cursorX, cursorY;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    ndcX = (2.0f * cursorX) / windowWidth - 1.0f;
    ndcY = 1.0f - (2.0f * cursorY) / windowHeight;

    if (mousePressed) {
        if (!deckMouseHold) {
            deckMouseHold = true;
            if (hoverCard(ndcX, ndcY, glm::vec3(cardNdcX, cardNdcY + 0.55f, 0.0f))) {
                dontSaveMore = false; // this line fixes deck state bug
                SavePreviousState();
                if (!deck.empty()) {
                    revealedDeck.push_back(deck.back());
                    deck.pop_back();
                    playSound(cardSwitchSound);
                    onCardMoved();
                }
                else {
                    deck.reserve(revealedDeck.size());
                    for (auto it = revealedDeck.rbegin(); it != revealedDeck.rend(); ++it)
                        deck.push_back(*it);
                    revealedDeck.clear();
                }
                dragActive = true;
            }
            else {
                dragActive = false;
                dontSaveMore = false;
            }
        }
    }
    else {
        deckMouseHold = false;
    }
    
    if (!revealedDeck.empty()) {
        if (hoverCard(ndcX, ndcY, revealedDeckPosition) && mousePressed) {
            if (draggingStack.empty()) {
                draggingStack.push_back(revealedDeck.back());
                draggingDeckCard = true;
            }
        }
    }

    bool validHover = false;
    for (int i = 0; i < 7; i++) {
        glm::vec3 cardPos2 = glm::vec3(cardNdcX + (i * 0.25f), cardNdcY, 0.0f);

        if (hoverCard(ndcX, ndcY, cardPos2) && mousePressed && !draggingStack.empty()) {
            validHover = true;
            currentMoveIsLegal = draggingStack[0].rank == 11;
            destinationColumn = i;
        }

        for (int j = columns[i].size() - 1; j >= 0; j--) {
            if (j == columns[i].size() - 1 && !columns[i][j].revealed && updateColumns && !mousePressed) { 
                columns[i][j].revealed = true;
                updateColumns = false;
            }
            glm::vec3 cardPositionww = glm::vec3(cardNdcX + (i * 0.25f), cardNdcY - (j * verticalSpacing) + (cardVertices[5] - cardVertices[1]) / 6.5f, 0.0f);
            if (mousePressed) {
                if (hoverCard(ndcX, ndcY, cardPositionww) && columns[i][j].revealed) {
                    if (draggingStack.empty()) {
                        SavePreviousState();
                        dontSaveMore = true;

                        currentMoveIsLegal = false;
                        homeColumn = i;
                        std::vector<int> indicesToErase;
                        for (int z = j; z <= columns[i].size(); z++) {
                            if (z >= columns[i].size()) break;
                            draggingStack.push_back(columns[i][z]);
                            indicesToErase.push_back(z);
                        }
                        for (int y = indicesToErase.size() - 1; y >= 0; y--) {
                            if (indicesToErase[y] < columns[i].size()) {
                                columns[i].erase(columns[i].begin() + indicesToErase[y]);
                            }
                        }
                    }
                    else {
                        validHover = true;
                        currentMoveIsLegal = legalMove(columns[i][j], draggingStack[0]);
                        destinationColumn = i;
                        hoverOverBase = false;
                    }
                    dragActive = true;
                }
                else {
                    dragActive = false;
                    //dontSaveMore = false;
                }
            }
        }
    }

    bool win[4];
    int x = 0;
    for (auto& base : bases) {
        win[x] = base.size() == 13;
        glm::vec3 pos = glm::vec3(cardNdcX + 0.75f + (x * 0.25f), cardNdcY + 0.5f, 0.0f);
        bool empty = base.empty();

        if (mousePressed) {
            if (hoverCard(ndcX, ndcY, pos)) {
                if (draggingStack.empty()) {
                    SavePreviousState();
                    dontSaveMore = true;

                    // drag from base
                    draggingStack.push_back(base.back());
                    base.pop_back();
                }
                else {
                    // drag to base
                    validHover = true;
                    int backRank = !empty ? base.back().rank : 0;
                    backRank = backRank == 12 ? 0 : backRank + 1;
                    currentMoveIsLegal = empty ? draggingStack[0].rank == 12 : draggingStack[0].suit == base[0].suit && draggingStack[0].rank + 1 - backRank == 1;
                    if (!empty) printf("%i\n", draggingStack[0].rank - backRank);
                    hoverOverBase = true;
                    destinationBase = x;
                    dragActive = true;
                }
            }
            else {
                dragActive = false;
            }
        }

        x++;
    }

    for (auto& w : win) {
        winning = true;
        if (!w) {
            winning = false;
            break;
        }
    }

    mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (!draggingStack.empty() && !draggingDeckCard) {

        if (!mousePressed) {
            if (currentMoveIsLegal && validHover) {
                if (!hoverOverBase) {
                    for (auto& card : draggingStack) {
                        columns[destinationColumn].push_back(card);
                    }
                    updateColumns = true;
                    onCardMoved();
                }
                else {
                    SavePreviousState();
                    bases[destinationBase].push_back(draggingStack[0]);
                    hoverOverBase = false;
                    updateColumns = true;
                    onCardMoved();
                }
                playSound(cardPlaceSound);
            }
            else {
                bool isLast;
                LoadPreviousState(&isLast);
                SetUndoAvailability(!isLast);
                /*for (auto& card : draggingStack) {
                    columns[homeColumn].push_back(card);
                }*/
            }
            draggingStack.clear();
        }
    }

    if (draggingDeckCard) {
        if (!mousePressed) {
            if (currentMoveIsLegal) {
                SavePreviousState();

                if (!hoverOverBase) {
                    for (auto& card : draggingStack) {
                        columns[destinationColumn].push_back(card);
                    }
                }
                else {
                    bases[destinationBase].push_back(draggingStack[0]);
                    hoverOverBase = false;
                }

                revealedDeck.pop_back();
                draggingDeckCard = false;

                onCardMoved();
                playSound(cardPlaceSound);
                dontSaveMore = false;
            }
            else {
                for (auto& card : draggingStack) {
                    draggingDeckCard = false;
                }
            }
            draggingStack.clear();
        }
    }

    if (winning) {
        time = -1;
        MessageBoxA(NULL, "You won!", "Congratulations!", MB_OK | MB_ICONINFORMATION);
        initCards();
    }

    if (mousePressed && !draggingStack.empty() && validHover) {
        SetCursor(currentMoveIsLegal ? legal : illegal);
        isCursorArrow = false;
    }
    else if(!isCursorArrow) {
        isCursorArrow = true; // avoid hammering api to always set same cursor
        SetCursor(arrow);
    }
}

void Engine::render() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //render bg
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bgTexture[selectedBackground]);
    glBindVertexArray(backgroundQuad.VAO);
    textureShader.use();
    glUniform4f(uvLoc, 0.0f, 0.0f, 1.0f, 1.0f);
    glUniform2f(quadSizeLoc, 1.0f, 1.0f);
    glUniform1i(texLoc, 0);
    textureShader.passUniformMat4("uMVP", glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f)));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // render field
    CardObject deckFace;
    deckFace.revealed = deck.empty();
    deckFace.suit = 4;
    deckFace.rank = deck.empty() ? 2 : 0;
    renderCard(deckFace, glm::vec3(cardNdcX, cardNdcY + 0.5f, 0.0f));

    if (!revealedDeck.empty()) {
        if (revealedDeck.size() >= 2) {
            renderCard(revealedDeck[revealedDeck.size() - 2], revealedDeckPosition);
        }
        if (!draggingDeckCard) renderCard(revealedDeck.back(), revealedDeckPosition);
    }

    for (int i = 0; i < 7; i++) {
        glm::vec3 cardPos3 = glm::vec3(cardNdcX + (i * 0.25f), cardNdcY, 0.0f);
        CardObject obj;
        obj.rank = 2;
        obj.suit = 4;
        obj.revealed = true;
        renderCard(obj, cardPos3);
    }

    int a = 0;
    for (auto& pile : columns) {
        int b = 0;
        for (auto& card : pile) {
            glm::vec3 cardPos = glm::vec3(cardNdcX + (a * 0.25f), cardNdcY - (b * verticalSpacing), 0.0f);
            renderCard(card, cardPos);
            b++;
        }
        a++;
    }
    int x = 0;
    for (auto& base : bases) {
        glm::vec3 pos = glm::vec3(cardNdcX + 0.75f + (x * 0.25f), cardNdcY + 0.5f, 0.0f);
        bool empty = base.empty();
        
        CardObject obj;
        obj.rank = empty ? 2 : base.back().rank;
        obj.suit = empty ? 4 : base.back().suit;
        obj.revealed = true;
        renderCard(obj, pos);

        x++;
    }

    if (!draggingStack.empty() && !draggingDeckCard) {
        int cardIndex = 0;
        for (auto& card : draggingStack) {
            card.draggingPos = glm::vec3(ndcX, ndcY - (cardIndex * 0.1f), 0.0f);
            renderCard(card, card.draggingPos);
            cardIndex++;
        }
    }

    if (draggingDeckCard) {
        int cardIndex = 0;
        for (auto& card : draggingStack) {
            card.draggingPos = glm::vec3(ndcX, ndcY - (cardIndex * 0.1f), 0.0f);
            renderCard(card, card.draggingPos);
            cardIndex++;
        }
    }
}

void Engine::SavePreviousState() {
    if (dontSaveMore) return;

    GameState previousState = {};

    for (int i = 0; i < 4; ++i) {
        previousState.bases[i] = bases[i];
    }

    //for (int i = 0; i < 52; ++i)
    //    previousState.cards[i] = cards[i];

    for (int i = 0; i < 7; ++i)
        previousState.columns[i] = columns[i];
    
    previousState.deck = deck;
    previousState.revealedDeck = revealedDeck;

    previousMoves.push_back(previousState);
    SetUndoAvailability(true); // just pushed previous move
}

bool Engine::LoadPreviousState(bool* isLast) {
    if (previousMoves.empty() && isLast) { 
        *isLast = true; 
        return false; 
    }

    GameState previousMove = previousMoves.back(); // error here
    previousMoves.pop_back();
    if (previousMoves.empty() && isLast) { 
        *isLast = true; 
    }

    for (int i = 0; i < 4; ++i)
        bases[i] = previousMove.bases[i];

    //for (int i = 0; i < 52; ++i)
    //    cards[i] = previousMove.cards[i];

    for (int i = 0; i < 7; ++i)
        columns[i] = previousMove.columns[i];

    deck = previousMove.deck;
    revealedDeck = previousMove.revealedDeck;

    updateColumns = true;
    return true;
}

void Engine::onCardMoved() {
    userSavedGame = false; // no longer same as save state
}

bool Engine::legalMove(const CardObject& topCard, const CardObject& secondCard) {
    int topRank = topCard.rank == 12 ? 0 : topCard.rank + 1;
    int secondRank = secondCard.rank == 12 ? 0 : secondCard.rank + 1;
    bool isAlternateColor = ((topCard.suit == 0 || topCard.suit == 1) && (secondCard.suit == 2 || secondCard.suit == 3)) ||
        ((topCard.suit == 2 || topCard.suit == 3) && (secondCard.suit == 1 || secondCard.suit == 0));
    bool isDescendingRank = (topRank - secondRank == 1);

    return isAlternateColor && isDescendingRank;
}

void Engine::renderCard(CardObject object, glm::vec3 position) {
    CardUV card = getCardUV(object.revealed ? object.rank : 0, object.revealed ? object.suit : 4);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);
    textureShader.use();
    glUniform4f(uvLoc, card.u0, card.v0, card.u1, card.v1);
    glUniform2f(quadSizeLoc, cardVertices[2] - cardVertices[0], cardVertices[5] - cardVertices[1]);
    glUniform1i(texLoc, 1);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    textureShader.passUniformMat4("uMVP", model);
    glBindVertexArray(cardQuad.VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Engine::initCards() {
    ZeroMemory(cards, sizeof(cards));
    for (int i = 0; i < 7; i++) {
        columns[i].clear();
    }

    winning = true;

    // 7 rows
    // 4 bases
    // 1 main deck

    for (auto& column : columns) {
        column.reserve(20); // reserve max cards expected in each column
    }
    deck.clear();
    revealedDeck.clear();
    deckIndex = 0;
    deck.reserve(52); // reserve 52 cards for the deck
    for (auto& base : bases) {
        base.clear();
    }

    int cardIndex = 0;
    for (int suit = 0; suit < 4; suit++) {
        for (int rank = 0; rank < 13; rank++) {
            cards[cardIndex].suit = suit;
            cards[cardIndex].rank = rank;
            cardIndex++;
        }
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(cards, cards+52, rng);

    g_cardIndex = 0;

    for (int i = 0; i < 7; i++) {
        int cardsInPile = i + 1;
        for (int j = 0; j < cardsInPile; j++) {
            columns[i].push_back(cards[g_cardIndex]);
            if (j == cardsInPile - 1) {
                columns[i][j].revealed = true;
            }
            g_cardIndex++;
        }
    }

    for (int i = g_cardIndex; i < 52; i++) {
        CardObject obj = cards[i];
        obj.revealed = true;
        deck.push_back(obj);
    }
    deckIndex = 0;

    previousMoves.clear();
    SetUndoAvailability(false); // previousMoves was cleared
}

void Engine::terminate() {
    UnregisterHotKey(hwnd, NEW_GAME);
    UnregisterHotKey(hwnd, SAVE_GAME);
    UnregisterHotKey(hwnd, LOAD_GAME);
    UnregisterHotKey(hwnd, UNDO);

    KillGameTimer();

    if (statusMessageThread.joinable())
        statusMessageThread.join();

    glfwTerminate();
}

bool Engine::running() { 
    isRunning = !glfwWindowShouldClose(window);
    return isRunning;
}

bool Engine::hoverCard(float ndcX, float ndcY, glm::vec3 cardPos) {
    const float scale = 0.3f;
    float halfWidth = (cardVertices[2] - cardVertices[0]) / 2.0f;
    float halfHeight = (cardVertices[5] - cardVertices[1]) / 2.0f;

    float left = cardPos.x - halfWidth;
    float right = cardPos.x + halfWidth;
    float top = cardPos.y + halfHeight;
    float bottom = cardPos.y - halfHeight;

    return ndcX >= left && ndcX <= right && ndcY >= bottom && ndcY <= top;
}
bool Engine::loadSounds() {
    cardPlaceSound = (LPCWSTR)resources->loadResource(106, L"WAVE", nullptr);
    if (!cardPlaceSound) return false;
    cardSwitchSound = (LPCWSTR)resources->loadResource(107, L"WAVE", nullptr);
    if (!cardSwitchSound) return false;

    return true;
}

void Engine::playSound(LPCWSTR sound) {
    PlaySound(sound, NULL, SND_MEMORY | SND_ASYNC);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR uIdSubclass,DWORD_PTR dwRefData) {
    Engine* self = reinterpret_cast<Engine*>(dwRefData);
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wparam);
        self->handleMenu(id);
        return 0;
    }
    case WM_HOTKEY: {
        int id = wparam;  // wParam = hotkey ID directly
        self->handleMenu(id);
        return 0;
    }
    case WM_UPDATE_TIMER: {
        TCHAR buf[64];
        swprintf(buf, 64, L"Time: %d", self->GetTime());
        SendMessageA(self->Get_hStatus(), SB_SETTEXT, 1, (LPARAM)buf);
        break;
    }
    }
    if (msg == WM_CLOSE) {
        self->pauseTimer = true;
        // we check if user saved game, if they saved (flag == true), we simply send IDNO to let WM_CLOSE reach glfw, if they didn't save, we show messagebox and pass whatever user selects
        auto message = self->GetUserSavedGameFlag() ? IDNO : MessageBoxA(hwnd, "Would you like to save the game before exiting?", "Exiting game", MB_ICONQUESTION | MB_YESNOCANCEL);
        switch (message) {
        case IDYES: {
            bool saveSuccess = self->SaveGame();
            self->SetUserSavedGameFlag(saveSuccess);
            if (!saveSuccess) return 0; // early exit since save isn't successfull
            // fall through
        }
        case IDNO:
            break; // WM_CLOSE reaches GLFW
        case IDCANCEL:
            self->pauseTimer = false;
            return 0; // WM_CLOSE doesn't reach GLFW
        }
    }
    return self->ogWndProc(hwnd, msg, wparam, lparam);
}

void Engine::registerHotkeys() {
    RegisterHotKey(hwnd, SAVE_GAME, MOD_CONTROL, 'S');
    RegisterHotKey(hwnd, LOAD_GAME, MOD_CONTROL, 'L');
    RegisterHotKey(hwnd, NEW_GAME, MOD_CONTROL, 'N');
    RegisterHotKey(hwnd, UNDO, MOD_CONTROL, 'Z');
}

void Engine::createStatusBar() {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    hStatus = CreateWindowEx(
        0,
        STATUSCLASSNAME,        // status bar class
        NULL,                   // no text yet
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,             // x,y,w,h ignored for status bar
        hwnd,             // parent window
        (HMENU)1,
        GetModuleHandle(NULL),
        NULL
    );

    // optional: divide into parts
    int parts[] = { 300, 400, 900, -1 }; // two parts: first 150 px, second takes rest
    SendMessage(hStatus, SB_SETPARTS, 4, (LPARAM)parts);

    // set initial text
    SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("Ready"));
    SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)TEXT("Time: 0"));
    SendMessage(hStatus, SB_SETTEXT, 3, (LPARAM)TEXT("v2.1"));
}

void Engine::initWinapi() {
    hwnd = glfwGetWin32Window(window);

    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(540));
    HICON hIconSmall = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(540));
    if (hIcon)
    {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
    }

    hmenu = CreateMenu();
    gamePopup = CreatePopupMenu();
    AppendMenu(gamePopup, MF_STRING, UNDO, TEXT("Undo move"));
    AppendMenu(gamePopup, MF_SEPARATOR, 0, 0);
    AppendMenu(gamePopup, MF_STRING, NEW_GAME, TEXT("New Game"));
    HMENU gameThemePopup = CreatePopupMenu();
    AppendMenu(gameThemePopup, MF_STRING, 1002, TEXT("Green"));
    AppendMenu(gameThemePopup, MF_STRING, 1004, TEXT("Red"));
    AppendMenu(gameThemePopup, MF_STRING, 1003, TEXT("Nature"));
    AppendMenu(gamePopup, MF_POPUP, (UINT_PTR)gameThemePopup, TEXT("Theme"));
    AppendMenu(gamePopup, MF_SEPARATOR, 0, 0);
    AppendMenu(gamePopup, MF_STRING, SAVE_GAME, TEXT("Save game"));
    AppendMenu(gamePopup, MF_STRING, LOAD_GAME, TEXT("Load game"));
    AppendMenu(gamePopup, MF_SEPARATOR, 0, 0);
    AppendMenu(gamePopup, MF_STRING, 1005, TEXT("Exit"));
    AppendMenu(hmenu, MF_POPUP, (UINT_PTR)gamePopup, TEXT("Game"));
    AppendMenu(hmenu, MF_STRING, 1006, TEXT("About"));
    SetMenu(hwnd, hmenu);

    ogWndProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    SetWindowSubclass(hwnd, WndProc, 0, reinterpret_cast<DWORD_PTR>(this));

    arrow = LoadCursor(NULL, IDC_ARROW);
    illegal = LoadCursor(NULL, IDC_NO);
    legal = LoadCursor(NULL, IDC_HAND);

    registerHotkeys();
    createStatusBar();
}

bool Engine::SaveGame() {
    if (SaveExists()) {
        // TODO: Make save file name selectable
        pauseTimer = true;
        auto message = MessageBoxA(hwnd, "Save file 'save.bin' already exists! Override?", "Warning", MB_ICONWARNING | MB_YESNO);
        if (message == IDNO) {
            pauseTimer = false;
            PushStatusMessage(L"Error saving game!");
            return false;
        }
    }
    Save(cards, columns, bases, deck, revealedDeck, previousMoves, time);
    pauseTimer = false;
    PushStatusMessage(L"Game saved successfully!");
    return true;
}

void Engine::LoadGame() {
    KillGameTimer();
    int newTime = 0;
    Load(cards, columns, bases, deck, revealedDeck, previousMoves, newTime);
    userSavedGame = true;
    bool isLast = previousMoves.empty();
    SetUndoAvailability(!isLast);
    ResetGameTimer(newTime);
}

void Engine::KillGameTimer() {
    timerRunning = false;
    pauseTimer = false;
    if(timerThread.joinable())
        timerThread.join();
}

void Engine::ResetGameTimer(int t) {
    TCHAR buf[64];
    swprintf(buf, 64, L"Time: %i", t);
    SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)(buf));
    time = t;
    pauseTimer = false;
    timerRunning = true;
    timerThread = std::thread(&Engine::TimerThreadFunc, this);
}

void Engine::handleMenu(int id) {
    switch (id) {
    case NEW_GAME: {
        pauseTimer = true;
        auto message = MessageBoxA(hwnd, "Would you like to save this game before starting a new one?", "New game", MB_YESNOCANCEL | MB_ICONQUESTION);
        switch (message) {
        case IDYES:
            if (!(userSavedGame = SaveGame())) break;
            // fall through
        case IDNO:
            initCards();
            pauseTimer = false;
            KillGameTimer();
            ResetGameTimer(0);
            break;
        case IDCANCEL:
            break;
        }
        break;
    }
    case 1002:
    case 1003:
    case 1004: {
        int bg = -(1002 - id);
        selectedBackground = bg;
        break;
    }
    case 1005:
        glfwSetWindowShouldClose(window, true);
        break;
    case 1006:
    {
        pauseTimer = true;
        MessageBoxA(NULL, "Made by spikest3r\nolehsheremeta.com", "About Solitaire", MB_OK | MB_ICONINFORMATION);
        pauseTimer = false;
        break;
    }

    case SAVE_GAME:
    {
        userSavedGame = SaveGame();
        break;
    }
    case LOAD_GAME:
    {
        pauseTimer = true;
        auto message = MessageBoxA(hwnd, "This will end the current game. Unsaved game will be lost. Load the save anyway?", "Warning", MB_ICONWARNING | MB_YESNO);
        if (message == IDYES) {
            LoadGame();
        }
        pauseTimer = false;
        break;
    }
    case UNDO:
    {
        bool isLast = false;
        bool success = LoadPreviousState(&isLast); // fetch whether move we reverted to was last in list
        PushStatusMessage(success ? L"Undid move" : L"Nothing to undo");
        SetUndoAvailability(!isLast);
        userSavedGame = false; // game state has changed
        break;
    }
    }
}

void Engine::SetUndoAvailability(bool flag) {
    EnableMenuItem(gamePopup, UNDO, MF_BYCOMMAND | (flag ? MF_ENABLED : MF_GRAYED));
}

void Engine::PushStatusMessage(LPCWSTR statusMessage) {
    SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)statusMessage);
    statusMessageDirty = true;
    statusMessageTime = statusMessageTimeout;
}

void Engine::StatusThreadFunc() {
    constexpr int timeIncrement = 10;

    while (isRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeIncrement));

        if (statusMessageTime > 0) {
            statusMessageTime-=timeIncrement;
        }
        else {
            if (statusMessageDirty) {
                SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("Ready"));
                statusMessageDirty = false;
            }
        }
    }
}