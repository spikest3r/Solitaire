#include "glEngine.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
	InitCommonControlsEx(&icc);
	BOOL themed = ::SetProcessDPIAware();

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