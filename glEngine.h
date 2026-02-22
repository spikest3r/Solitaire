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
#include <thread>

#pragma comment(lib, "winmm.lib") 
#pragma comment(lib, "Comctl32.lib")



class Engine {
public:
	GLFWwindow* window;
	Engine(const char* name, int w, int h, bool* success);
	void loop();
	void terminate();
	bool running();
	WNDPROC ogWndProc;
	void handleMenu(int id);
	bool SaveGame(bool showMessage = true);
	bool LoadGame(std::wstring presetPath = L"");
	void SetUserSavedGameFlag(bool flag) { userSavedGame = flag; }
	bool GetUserSavedGameFlag() { return userSavedGame; }
	HWND Get_hStatus() { return hStatus; }
	HMENU GetHMENU() { return hmenu; }

	bool pauseTimer = false;
	bool pauseGame = false;
	void KillGameTimer();
	void ResetGameTimer(int time);
	int GetTime() { return time; }

	void registerHotkeys();
	void killHotkeys();

	int ShowUserMessage(const char* message, const char* caption, UINT type, bool ensureNoGamePause = false);
	bool LoadStatistics(int& won, int& lost);
	void GetCurrentGameStats(int& movesExt, int& timeExt, int& undidExt) {
		movesExt = moves;
		timeExt = time;
		undidExt = undidMovesCount;
	}
private:
	const char* ogTitle;

	bool isCursorArrow = true;
	bool isRunning = true;

	bool autoFinishAvail = false;

	// status messages variable
	void PushStatusMessage(LPCWSTR statusMessage);
	std::thread statusMessageThread;
	int statusMessageTime = 0;
	bool statusMessageDirty = false;
	const int statusMessageTimeout = 3000; // 3 seconds
	void StatusThreadFunc();

	// timer variables and game stats
	int moves = 0;
	int time;

	std::thread timerThread;
	bool timerRunning = false;
	void TimerThreadFunc();

	void UpdateMovesStatusText();

	void update();
	void render(bool playAnim = false);
	float ndcX, ndcY;

	unsigned int atlasTexture;
	unsigned int bgTexture[3];
	unsigned int customBgTexture = -1;
	int selectedBackground = 0;

	unsigned int framebuffer;
	unsigned int texColorBuffer;
	GLuint quadVAO;
	bool createFramebuffer();

	float windowWidth, windowHeight; // window size

	float cardAspect, cardScale, singleCardPixelWidth;
	void renderCard(CardObject object, glm::vec3 position, float yRot = 0.0f, bool legal = false);
	Shader textureShader;
	Shader quadShader;
	Shader wipeShader;
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
	bool legalAutoFinishMove(const CardObject& topCard, const CardObject& secondCard);
	bool currentMoveIsLegal = false;
	bool oldCursor = false; // for move legality cursor
	bool hoverCard(float ndcX, float ndcY, glm::vec3 cardPos);

	Resource* resources;

	std::vector<CardObject> draggingStack;
	int homeColumn;
	int homeBase;
	int destinationColumn;
	bool updateColumns = false;
	int deckIndex = 0;
	bool deckMouseHold = false;
	bool draggingDeckCard = false;
	bool mousePressed = false;
	int destinationBase = 0;
	bool hoverOverBase = false;
	bool dragActive = false;

	bool winning = false;
	bool isWinningAnimationPlaying = false;
	float winningAnimationTime = 0;
	float popTimer = 0.0f;
	const float popInterval = 1.7f;

	float r = 0.0f;

	int g_cardIndex = 0;

	GLuint texLoc;
	GLuint quadSizeLoc;
	GLuint uvLoc;

	float cardNdcX = -0.8f;
	float cardNdcY = 0.0f;
	float verticalSpacing;
	glm::vec3 revealedDeckPosition = glm::vec3(cardNdcX + 0.25f, cardNdcY + 0.5f, 0.0f);

	void initCards(int userSeed = 0, bool won = false);
	bool loadSounds();
	void playSound(LPCWSTR sound);

	void ProposeAutoFinish();
	void AutoFinish();
	bool autoFinishRunning = false;
	float autoFinishTime = 0.0f;
	const float autoFinishStep = 0.5f;
	CardObject lastBaseCards[4]; // cache last card from each base

	void SavePreviousState();
	bool LoadPreviousState(bool* isLast);
	GameState* PeekPreviousState(bool* isLast);
	void SetOptionAvailability(UINT option, bool flag);
	void SetMenuItemText(HMENU hMenu, UINT itemId, const wchar_t* newText);

	LPCWSTR cardPlaceSound;
	LPCWSTR cardSwitchSound;

	void initWinapi();
	void createStatusBar();
	HWND hwnd;
	HWND hStatus; // bottom status bar

	// window menus
	HMENU hmenu, gamePopup;

	HCURSOR arrow, illegal, legal;

	bool userSavedGame = false;

	bool SaveExists();
	void onCardMoved();

	float previousTime;
	float deltaTime;
	void UpdateDeltaTime();

	std::wstring ShowSaveDialog(HWND);
	std::wstring ShowOpenDialog(HWND, bool allFiles = false);
	std::string WideToString(LPCWSTR wide);

	bool selectedSaveFile = false;
	std::string saveFilePath;

	bool SaveLastSavePath(const std::wstring& path);
	std::wstring LoadLastSavePath();
	bool SaveStatistics(int won, int lost);

	float autoSaveCountDown = 60.0f; // every 1 minute

	void initCardsWithSeed();

	unsigned int currentSeed;

	void PushDefaultStatusMessage(LPCWSTR statusMessage);
	void ClearDefaultStatusMessage();
	bool isCustomDefLabel = false;
	LPCWSTR customDefLabel;

	void SetWindowTitle(const char* additionalText);

	void UpdateAnimations();
	int undidMovesCount = 0;
	void ShowStatisticsDialog();
	void ShowAboutDialog();
};