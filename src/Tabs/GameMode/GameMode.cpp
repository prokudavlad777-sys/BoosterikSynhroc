#include "GameMode.h"
#include "../../Optimizers/Power.h"
#include "../../Monitoring/Monitor.h"
#include "../../Core/Util.h"
#include "../../Logger/Logger.h"
#include <psapi.h>
#include <map>
#include <set>
#include <cstdio>
#include <cwchar>

namespace GameMode {

static bool g_on = false;
static std::wstring g_plan;
static bool g_hadGm = false;
static DWORD g_oldGm = 0;
static int g_trimmed = 0;
static std::wstring g_game;
static std::map<DWORD, DWORD> g_raised;   // pid -> original priority class

static const wchar_t* kGameBar = L"Software\\Microsoft\\GameBar";

static bool IsFullscreen(HWND h) {
    if (!h || !IsWindowVisible(h)) return false;
    wchar_t cls[64] = {};
    GetClassNameW(h, cls, 64);
    if (!wcscmp(cls, L"Progman") || !wcscmp(cls, L"WorkerW") || !wcscmp(cls, L"Shell_TrayWnd")) return false;
    RECT r;
    GetWindowRect(h, &r);
    MONITORINFO mi{sizeof(mi)};
    if (!GetMonitorInfoW(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &mi)) return false;
    return r.left <= mi.rcMonitor.left && r.top <= mi.rcMonitor.top &&
           r.right >= mi.rcMonitor.right && r.bottom >= mi.rcMonitor.bottom;
}

static std::wstring ProcName(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return L"";
    wchar_t b[MAX_PATH] = {};
    DWORD n = MAX_PATH;
    std::wstring name;
    if (QueryFullProcessImageNameW(h, 0, b, &n)) {
        name = b;
        size_t p = name.find_last_of(L"\\/");
        if (p != std::wstring::npos) name = name.substr(p + 1);
    }
    CloseHandle(h);
    return name;
}

static int TrimBackground(DWORD keepPid) {
    static const wchar_t* skip[] = {L"system", L"registry", L"csrss.exe", L"wininit.exe", L"winlogon.exe",
                                    L"services.exe", L"lsass.exe", L"smss.exe", L"dwm.exe", L"explorer.exe",
                                    L"audiodg.exe", L"memcompression"};
    int count = 0;
    for (const ProcInfo& p : Monitor::I().Processes(false)) {
        if (count >= 40 || p.memBytes < (60ull << 20)) break;
        if (p.pid == GetCurrentProcessId() || p.pid == keepPid) continue;
        bool protectedProc = false;
        std::wstring n = util::ToLower(p.name);
        for (auto s : skip) if (n == s) protectedProc = true;
        if (protectedProc) continue;
        HANDLE h = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION, FALSE, p.pid);
        if (!h) continue;
        if (EmptyWorkingSet(h)) ++count;     // memory is paged back in on demand; nothing is closed
        CloseHandle(h);
    }
    return count;
}

bool IsOn() { return g_on; }

void Enable() {
    if (g_on) return;
    g_plan = Power::GetActive();
    Power::EnableHighPerformance();
    g_hadGm = util::RegGetDword(HKEY_CURRENT_USER, kGameBar, L"AutoGameModeEnabled", g_oldGm);
    util::RegSetDword(HKEY_CURRENT_USER, kGameBar, L"AutoGameModeEnabled", 1);
    DWORD fg = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &fg);
    g_trimmed = TrimBackground(fg);
    g_on = true;
    Log::Write(L"Game Mode: увімкнено");
}

void Disable() {
    if (!g_on) return;
    for (auto& kv : g_raised) {
        HANDLE h = OpenProcess(PROCESS_SET_INFORMATION, FALSE, kv.first);
        if (h) { SetPriorityClass(h, kv.second); CloseHandle(h); }
    }
    g_raised.clear();
    g_game.clear();
    if (!g_plan.empty()) Power::SetActive(g_plan);
    if (g_hadGm) util::RegSetDword(HKEY_CURRENT_USER, kGameBar, L"AutoGameModeEnabled", g_oldGm);
    else util::RegDelValue(HKEY_CURRENT_USER, kGameBar, L"AutoGameModeEnabled");
    g_on = false;
    Log::Write(L"Game Mode: вимкнено");
}

void Tick() {
    if (!g_on) return;
    HWND fg = GetForegroundWindow();
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (!pid || pid == GetCurrentProcessId() || g_raised.count(pid) || !IsFullscreen(fg)) return;
    HANDLE h = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return;
    DWORD old = GetPriorityClass(h);
    if (old && old != HIGH_PRIORITY_CLASS && SetPriorityClass(h, HIGH_PRIORITY_CLASS)) {
        g_raised[pid] = old;
        g_game = ProcName(pid);
        Log::Write(L"Game Mode: високий пріоритет для " + g_game);
    }
    CloseHandle(h);
}

std::wstring Status() {
    if (!g_on) return L"Вимкнено";
    wchar_t b[200];
    swprintf(b, 200, L"Увімкнено · звільнено пам'ять у %d фонових програм", g_trimmed);
    std::wstring s = b;
    if (!g_game.empty()) s += L" · гра: " + g_game;
    else s += L" · очікую повноекранну гру";
    return s;
}
}
