#include <Windows.h>

static const WCHAR *g_QuitDogEventName = L"Global\\MiniAppInstance_4673";

void WINAPI ISCloseAll()
{
	HANDLE hGlobalQuitEvent = OpenEvent(EVENT_MODIFY_STATE , FALSE, g_QuitDogEventName);
	if (hGlobalQuitEvent) {
		SetEvent(hGlobalQuitEvent);
		CloseHandle(hGlobalQuitEvent);

		for (;;) {
			hGlobalQuitEvent = OpenEvent(EVENT_MODIFY_STATE , FALSE, g_QuitDogEventName);
			if (hGlobalQuitEvent) {
				CloseHandle(hGlobalQuitEvent);
				Sleep(200);
			} else {
				break;
			}
		}
	}
}
