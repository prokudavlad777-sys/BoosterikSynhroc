#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "UI/Ui.h"
#include "UI/MainWindow.h"
#include "Logger/Logger.h"
#include "Monitoring/Monitor.h"

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    HANDLE once = CreateMutexW(nullptr, TRUE, L"Boosterik.SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // the previous copy may be closing right now (e.g. relaunch as administrator)
        DWORD w = WaitForSingleObject(once, 3000);
        if (w != WAIT_OBJECT_0 && w != WAIT_ABANDONED) {
            MessageBoxW(nullptr, L"Boosterik уже запущено.", L"Boosterik", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
    }
    SetProcessDPIAware();

    Gdiplus::GdiplusStartupInput gsi;
    ULONG_PTR token = 0;
    Gdiplus::GdiplusStartup(&token, &gsi, nullptr);

    Log::Init();
    Monitor::I().Update();

    int code = 0;
    {
        MainWindow win;
        if (win.Create(inst)) {
            MSG msg;
            while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            code = (int)msg.wParam;
        } else {
            MessageBoxW(nullptr, L"Не вдалося створити вікно.", L"Boosterik", MB_OK | MB_ICONERROR);
            code = 1;
        }
    }
    ui::Shutdown();
    Gdiplus::GdiplusShutdown(token);
    if (once) CloseHandle(once);
    return code;
}
