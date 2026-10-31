#include "Hardware.h"
#include "../Core/Util.h"
#include <cwchar>
#include <cwctype>
#include <cstdio>

namespace Hardware {

static std::wstring Trim(std::wstring s) {
    while (!s.empty() && iswspace(s.front())) s.erase(s.begin());
    while (!s.empty() && iswspace(s.back())) s.pop_back();
    return s;
}

static HwInfo Collect() {
    HwInfo h;
    h.cpu = Trim(util::RegGetString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString"));
    if (h.cpu.empty()) h.cpu = L"Невідомий процесор";

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    h.cores = si.dwNumberOfProcessors;

    MEMORYSTATUSEX ms{sizeof(ms)};
    GlobalMemoryStatusEx(&ms);
    h.ramTotal = ms.ullTotalPhys;

    const wchar_t* cv = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    std::wstring prod = util::RegGetString(HKEY_LOCAL_MACHINE, cv, L"ProductName");
    std::wstring ver = util::RegGetString(HKEY_LOCAL_MACHINE, cv, L"DisplayVersion");
    std::wstring build = util::RegGetString(HKEY_LOCAL_MACHINE, cv, L"CurrentBuild");
    h.os = prod + (ver.empty() ? L"" : L" " + ver) + (build.empty() ? L"" : L" (build " + build + L")");

    DISPLAY_DEVICEW dd{};
    dd.cb = sizeof(dd);
    for (DWORD i = 0; EnumDisplayDevicesW(nullptr, i, &dd, 0); ++i) {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) { h.gpu = dd.DeviceString; break; }
        if (h.gpu.empty()) h.gpu = dd.DeviceString;
        dd.cb = sizeof(dd);
    }
    if (h.gpu.empty()) h.gpu = L"Невідома відеокарта";

    std::wstring g = util::ToLower(h.gpu);
    if (g.find(L"nvidia") != std::wstring::npos || g.find(L"geforce") != std::wstring::npos) h.gpuVendor = L"NVIDIA";
    else if (g.find(L"amd") != std::wstring::npos || g.find(L"radeon") != std::wstring::npos) h.gpuVendor = L"AMD";
    else if (g.find(L"intel") != std::wstring::npos) h.gpuVendor = L"Intel";

    // Driver version/date from the display adapter class key
    const wchar_t* cls = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}";
    for (int i = 0; i < 10; ++i) {
        wchar_t sub[256];
        swprintf(sub, 256, L"%ls\\%04d", cls, i);
        std::wstring desc = util::RegGetString(HKEY_LOCAL_MACHINE, sub, L"DriverDesc");
        if (desc.empty()) continue;
        if (desc == h.gpu || h.gpuDriverVersion.empty()) {
            h.gpuDriverVersion = util::RegGetString(HKEY_LOCAL_MACHINE, sub, L"DriverVersion");
            h.gpuDriverDate = util::RegGetString(HKEY_LOCAL_MACHINE, sub, L"DriverDate");
            if (desc == h.gpu) break;
        }
    }
    return h;
}

const HwInfo& Get() {
    static HwInfo info = Collect();
    return info;
}
}
