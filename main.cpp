#include "glEngine.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	bool success;
	Engine engine("Solitaire", 1100, 768, &success);
	if (success) {
		while (engine.running()) {
			engine.loop();
		}
	}
	engine.terminate();
	return 0;
}