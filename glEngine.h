#pragma once
#include <glad/glad.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Windows.h>
#include "carduv.h"
#include "shader.h"
#include "glbuffer.h"
#include "cardobject.h"
#include "res.h"
#include <vector>
#include <algorithm>
#include <random>
#include <mmsystem.h>
#include <commctrl.h>
#include <fstream>

#pragma comment(lib, "winmm.lib") 
#pragma comment(lib, "Comctl32.lib")

struct GameState {
	Pile columns[7];
	std::vector<CardObject> deck;
	std::vector<CardObject> revealedDeck;
	Pile bases[4];
};

class Engine {
public:
	GLFWwindow* window;
	Engine(const char* name, int w, int h, bool* success);
	void loop();
	void terminate();
	bool running();
	WNDPROC ogWndProc;
	void handleMenu(int id);
	bool SaveGame();
	void LoadGame();
	void SetUserSavedGameFlag(bool flag) { userSavedGame = flag; }
	bool GetUserSavedGameFlag() { return userSavedGame; }
private:
	void update();
	void render();
	float ndcX, ndcY;

	unsigned int atlasTexture;
	unsigned int bgTexture[3];
	int selectedBackground = 0;
	float windowWidth, windowHeight;
	float cardAspect, cardScale, singleCardPixelWidth;
	void renderCard(CardObject object, glm::vec3 position);
	Shader textureShader;
	glm::mat4 proj;
	GLbuffer cardQuad;
	GLbuffer backgroundQuad;
	bool loadAtlas();
	bool loadBackgrounds();
	float cardW, cardH;
	CardUV getCardUV(int rank, int suit);

	CardObject cards[52];
	Pile columns[7];
	std::vector<CardObject> deck;
	std::vector<CardObject> revealedDeck;
	Pile bases[4];

	bool dontSaveMore = false;
	std::vector<GameState> previousMoves;

	bool legalMove(const CardObject& topCard, const CardObject& secondCard);
	bool currentMoveIsLegal = false;
	bool oldCursor = false; // for move legality cursor
	bool hoverCard(float ndcX, float ndcY, glm::vec3 cardPos);

	Resource* resources;

	std::vector<CardObject> draggingStack;
	int homeColumn;
	int destinationColumn;
	bool updateColumns = false;
	int deckIndex = 0;
	bool deckMouseHold = false;
	bool draggingDeckCard = false;
	bool mousePressed = false;
	int destinationBase = 0;
	bool hoverOverBase = false;
	bool dragActive = false;

	bool winning = true;

	int g_cardIndex = 0;

	GLuint texLoc;
	GLuint quadSizeLoc;
	GLuint uvLoc;

	float cardNdcX = -0.8f;
	float cardNdcY = 0.0f;
	float verticalSpacing;
	glm::vec3 revealedDeckPosition = glm::vec3(cardNdcX + 0.25f, cardNdcY + 0.5f, 0.0f);

	void initCards();
	bool loadSounds();
	void playSound(LPCWSTR sound);

	void SavePreviousState();
	void LoadPreviousState();

	LPCWSTR cardPlaceSound;
	LPCWSTR cardSwitchSound;

	void initWinapi();
	void registerHotkeys();
	HWND hwnd;

	HCURSOR arrow, illegal, legal;

	bool userSavedGame = false;

	bool SaveExists();
	void onCardMoved();
};