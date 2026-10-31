#include "Monitor.h"
#include "../Core/Util.h"
#include "../Optimizers/Power.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>
#include <cstdio>
#include <cwchar>

static unsigned long long ToU64(const FILETIME& f) {
    return ((unsigned long long)f.dwHighDateTime << 32) | f.dwLowDateTime;
}

Monitor& Monitor::I() {
    static Monitor m;
    return m;
}

void Monitor::Update() {
    FILETIME idle, kernel, user;
    GetSystemTimes(&idle, &kernel, &user);
    unsigned long long i = ToU64(idle), k = ToU64(kernel), u = ToU64(user);
    unsigned long long dTotal = 0;
    if (havePrev_) {
        unsigned long long dIdle = i - prevIdle_;
        dTotal = (k - prevKernel_) + (u - prevUser_);   // kernel time already includes idle
        cpu_ = dTotal ? 100.0 * (double)(dTotal - dIdle) / (double)dTotal : 0.0;
    }
    prevIdle_ = i; prevKernel_ = k; prevUser_ = u;
    havePrev_ = true;
    hist_.push_back(cpu_);
    while (hist_.size() > 60) hist_.pop_front();

    MEMORYSTATUSEX ms{sizeof(ms)};
    GlobalMemoryStatusEx(&ms);
    ramTotal_ = ms.ullTotalPhys;
    ramUsed_ = ms.ullTotalPhys - ms.ullAvailPhys;
    ramPct_ = (double)ms.dwMemoryLoad;

    // per-process snapshot
    std::vector<ProcInfo> out;
    std::map<DWORD, unsigned long long> now;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe{sizeof(pe)};
        for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
            if (pe.th32ProcessID == 0) continue;
            ProcInfo p;
            p.pid = pe.th32ProcessID;
            p.name = pe.szExeFile;
            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, p.pid);
            if (h) {
                FILETIME c, e, kt, ut;
                if (GetProcessTimes(h, &c, &e, &kt, &ut)) {
                    unsigned long long t = ToU64(kt) + ToU64(ut);
                    now[p.pid] = t;
                    auto it = prevProc_.find(p.pid);
                    if (it != prevProc_.end() && dTotal > 0 && t >= it->second)
                        p.cpu = 100.0 * (double)(t - it->second) / (double)dTotal;
                }
                PROCESS_MEMORY_COUNTERS pmc{sizeof(pmc)};
                if (GetProcessMemoryInfo(h, &pmc, sizeof(pmc))) p.memBytes = pmc.WorkingSetSize;
                CloseHandle(h);
            }
            out.push_back(std::move(p));
        }
        CloseHandle(snap);
    }
    {
        std::lock_guard<std::mutex> lock(mu_);
        procs_ = std::move(out);
        prevProc_ = std::move(now);
    }
}

std::vector<ProcInfo> Monitor::Processes(bool sortByCpu) const {
    std::vector<ProcInfo> v;
    {
        std::lock_guard<std::mutex> lock(mu_);
        v = procs_;
    }
    std::sort(v.begin(), v.end(), [&](const ProcInfo& a, const ProcInfo& b) {
        return sortByCpu ? a.cpu > b.cpu : a.memBytes > b.memBytes;
    });
    return v;
}

std::vector<Finding> Monitor::FindLosses() const {
    std::vector<Finding> f;
    wchar_t buf[256];

    std::wstring plan = Power::GetActive();
    if (Power::IsBalancedOrSaver(plan))
        f.push_back({2, L"Схема живлення не на максимум: ЦП може знижувати частоти в іграх."});

    DWORD v = 1;
    if (util::RegGetDword(HKEY_CURRENT_USER, L"System\\GameConfigStore", L"GameDVR_Enabled", v) && v != 0)
        f.push_back({1, L"Game DVR увімкнено: фоновий запис ігор забирає частину FPS."});
    v = 0;
    if (util::RegGetDword(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR", L"AppCaptureEnabled", v) && v != 0)
        f.push_back({1, L"Xbox Game Bar захоплює ігри у фоні."});
    v = 1;
    if (!util::RegGetDword(HKEY_CURRENT_USER, L"Software\\Microsoft\\GameBar", L"AutoGameModeEnabled", v) || v == 0)
        f.push_back({1, L"Windows Game Mode вимкнено."});

    if (ramPct_ >= 85) {
        swprintf(buf, 256, L"Пам'ять заповнена на %.0f%%: закрийте зайві програми.", ramPct_);
        f.push_back({2, buf});
    } else if (ramPct_ >= 70) {
        swprintf(buf, 256, L"Пам'ять заповнена на %.0f%%.", ramPct_);
        f.push_back({1, buf});
    }

    auto byCpu = Processes(true);
    int shown = 0;
    for (auto& p : byCpu) {
        if (p.cpu < 8.0 || shown >= 2) break;
        if (p.name == L"System Idle Process") continue;
        swprintf(buf, 256, L"%ls завантажує ЦП на %.0f%%.", p.name.c_str(), p.cpu);
        f.push_back({1, buf});
        ++shown;
    }
    auto byMem = Processes(false);
    shown = 0;
    for (auto& p : byMem) {
        if (p.memBytes < (800ull << 20) || shown >= 2) break;
        swprintf(buf, 256, L"%ls займає %ls пам'яті.", p.name.c_str(), util::FormatBytes(p.memBytes).c_str());
        f.push_back({1, buf});
        ++shown;
    }
    if (f.empty()) f.push_back({0, L"Явних втрат FPS не знайдено. Система готова до ігор."});
    return f;
}
