#include "Logger.h"
#include "../Core/Util.h"
#include <mutex>
#include <cstdio>
#include <cwchar>

namespace Log {
static std::mutex g_mu;

std::wstring FilePath() { return util::Path(L"configs\\boosterik.log"); }

void Init() {
    util::EnsureDir(util::Path(L"configs"));
    Write(L"=== Boosterik started ===");
}

void Write(const std::wstring& line) {
    std::lock_guard<std::mutex> lock(g_mu);
    HANDLE f = CreateFileW(FilePath().c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t head[64];
    swprintf(head, 64, L"[%04d-%02d-%02d %02d:%02d:%02d] ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    std::string s = util::WideToUtf8(std::wstring(head) + line + L"\r\n");
    DWORD w = 0;
    WriteFile(f, s.data(), (DWORD)s.size(), &w, nullptr);
    CloseHandle(f);
}
}
