#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "glEngine.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
}

void Engine::loop() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //render bg
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bgTexture[selectedBackground]);
    glBindVertexArray(backgroundQuad.VAO);
    textureShader.use();
    glUniform4f(uvLoc, 0.0f, 0.0f, 1.0f, 1.0f);
    glUniform2f(quadSizeLoc, 1.0f,1.0f);
    glUniform1i(texLoc, 0);
    textureShader.passUniformMat4("uMVP", glm::scale(glm::mat4(1.0f),glm::vec3(2.0f,2.0f,1.0f)));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    currentMoveIsLegal = false;

    double cursorX, cursorY;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    float ndcX = (2.0f * cursorX) / windowWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * cursorY) / windowHeight;

    CardObject deckFace;
    deckFace.revealed = deck.empty();
    deckFace.suit = 4;
    deckFace.rank = deck.empty() ? 2 : 0;
    renderCard(deckFace, glm::vec3(cardNdcX, cardNdcY+0.5f, 0.0f));
    if (mousePressed) {
        if (!deckMouseHold) {
            deckMouseHold = true;
            if (hoverCard(ndcX, ndcY, glm::vec3(cardNdcX, cardNdcY + 0.55f, 0.0f))) {
                if (!deck.empty()) {
                    revealedDeck.push_back(deck.back());
                    deck.pop_back();
                    playSound(cardSwitchSound);
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
            }
        }
    }
    else {
        deckMouseHold = false;
    }
    
    if (!revealedDeck.empty()) {
        if (revealedDeck.size() >= 2) {
            renderCard(revealedDeck[revealedDeck.size() - 2],revealedDeckPosition);
        }
        if(!draggingDeckCard) renderCard(revealedDeck.back(), revealedDeckPosition);
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
                        if (draggingStack.empty()) {
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
                }
            }
        }

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

    bool win[4];
    int x = 0;
    for (auto& base : bases) {
        win[x] = base.size() == 13;
        glm::vec3 pos = glm::vec3(cardNdcX + 0.75f + (x * 0.25f), cardNdcY + 0.5f, 0.0f);
        bool empty = base.empty();

        if (mousePressed) {
            if (hoverCard(ndcX, ndcY, pos) && !draggingStack.empty()) {
                validHover = true;
                int backRank = !empty ? base.back().rank : 0;
                backRank = backRank == 12 ? 0 : backRank + 1;
                currentMoveIsLegal = empty ? draggingStack[0].rank == 12 : draggingStack[0].suit == base[0].suit && draggingStack[0].rank + 1 - backRank == 1;
                if (!empty) printf("%i\n", draggingStack[0].rank - backRank);
                hoverOverBase = true;
                destinationBase = x;
                dragActive = true;
            }
            else {
                dragActive = false;
            }
        }

        CardObject obj;
        obj.rank = empty ? 2 : base.back().rank;
        obj.suit = empty ? 4 : base.back().suit;
        obj.revealed = true;
        renderCard(obj, pos);

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
        int cardIndex = 0;
        for (auto& card : draggingStack) {
            card.draggingPos = glm::vec3(ndcX, ndcY- (cardIndex * 0.1f), 0.0f);
            renderCard(card, card.draggingPos);
            cardIndex++;
        }

        if (!mousePressed) {
            if (currentMoveIsLegal && validHover) {
                if (!hoverOverBase) {
                    for (auto& card : draggingStack) {
                        columns[destinationColumn].push_back(card);
                    }
                    updateColumns = true;
                }
                else {
                    bases[destinationBase].push_back(draggingStack[0]);
                    hoverOverBase = false;
                    updateColumns = true;
                }
                playSound(cardPlaceSound);
            }
            else {
                for (auto& card : draggingStack) {
                    columns[homeColumn].push_back(card);
                }
            }
            draggingStack.clear();
        }
    }

    if (draggingDeckCard) {
        int cardIndex = 0;
        for (auto& card : draggingStack) {
            card.draggingPos = glm::vec3(ndcX, ndcY - (cardIndex * 0.1f), 0.0f);
            renderCard(card, card.draggingPos);
            cardIndex++;
        }

        if (!mousePressed) {
            if (currentMoveIsLegal) {
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
                playSound(cardPlaceSound);
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
        MessageBoxA(NULL, "You won!", "Congratulations!", MB_OK | MB_ICONINFORMATION);
        initCards();
    }

    if (mousePressed && !draggingStack.empty() && validHover) {
        SetCursor(currentMoveIsLegal ? arrow : illegal);
    }
    else {
        SetCursor(arrow);
    }

    glfwSwapBuffers(window);
    glfwWaitEvents();
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
}

void Engine::terminate() { glfwTerminate(); }
bool Engine::running() { return !glfwWindowShouldClose(window); }

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
    if(msg == WM_COMMAND) {
        int id = LOWORD(wparam);
        self->handleMenu(id);
        return 0;
    }
    return self->ogWndProc(hwnd, msg, wparam, lparam);
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

    HMENU hmenu = CreateMenu();
    HMENU gamePopup = CreatePopupMenu();
    AppendMenu(gamePopup, MF_STRING, 1001, TEXT("New Game"));
    HMENU gameThemePopup = CreatePopupMenu();
    AppendMenu(gameThemePopup, MF_STRING, 1002, TEXT("Green"));
    AppendMenu(gameThemePopup, MF_STRING, 1004, TEXT("Red"));
    AppendMenu(gameThemePopup, MF_STRING, 1003, TEXT("Nature"));
    AppendMenu(gamePopup, MF_POPUP, (UINT_PTR)gameThemePopup, TEXT("Theme"));
    AppendMenu(gamePopup, MF_SEPARATOR, 0, 0);
    AppendMenu(gamePopup, MF_STRING, 1005, TEXT("Exit"));
    AppendMenu(hmenu, MF_POPUP, (UINT_PTR)gamePopup, TEXT("Game"));
    AppendMenu(hmenu, MF_STRING, 1006, TEXT("About"));
    SetMenu(hwnd, hmenu);

    ogWndProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    SetWindowSubclass(hwnd, WndProc, 0, reinterpret_cast<DWORD_PTR>(this));

    arrow = LoadCursor(NULL, IDC_ARROW);
    illegal = LoadCursor(NULL, IDC_NO);
}

void Engine::handleMenu(int id) {
    switch (id) {
    case 1001:
        initCards();
        break;
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
        MessageBoxA(NULL, "Made by spikest3r\nolehsheremeta.com", "About Solitaire", MB_OK | MB_ICONINFORMATION);
        break;
    }
    }
}