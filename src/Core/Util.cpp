#include "Util.h"
#include <vector>
#include <cstdio>
#include <cwctype>
#include <algorithm>

namespace util {

std::wstring ExeDir() {
    wchar_t buf[MAX_PATH * 2] = {};
    GetModuleFileNameW(nullptr, buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
    std::wstring s(buf);
    size_t p = s.find_last_of(L"\\/");
    return p == std::wstring::npos ? L"." : s.substr(0, p);
}

std::wstring Path(const std::wstring& rel) { return ExeDir() + L"\\" + rel; }

std::wstring FormatBytes(unsigned long long b) {
    const wchar_t* u[] = {L"Б", L"КБ", L"МБ", L"ГБ", L"ТБ"};
    double v = (double)b;
    int i = 0;
    while (v >= 1024.0 && i < 4) { v /= 1024.0; ++i; }
    wchar_t out[64];
    if (i == 0) swprintf(out, 64, L"%llu %ls", b, u[0]);
    else        swprintf(out, 64, L"%.1f %ls", v, u[i]);
    return out;
}

bool FileExists(const std::wstring& p) { return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES; }

void EnsureDir(const std::wstring& dir) { CreateDirectoryW(dir.c_str(), nullptr); }

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}

std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string o(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), &o[0], n, nullptr, nullptr);
    return o;
}

std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return (wchar_t)towlower(c); });
    return s;
}

std::string RunCapture(const std::wstring& cmdline, DWORD timeoutMs, DWORD* exitCode) {
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE rd = nullptr, wr = nullptr;
    if (!CreatePipe(&rd, &wr, &sa, 1 << 16)) return "";
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = wr;
    si.hStdError = wr;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmd(cmdline.begin(), cmdline.end());
    cmd.push_back(L'\0');
    BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                             nullptr, nullptr, &si, &pi);
    CloseHandle(wr);
    std::string out;
    if (!ok) { CloseHandle(rd); return ""; }

    if (WaitForSingleObject(pi.hProcess, timeoutMs) == WAIT_TIMEOUT) TerminateProcess(pi.hProcess, 1);
    DWORD avail = 0;
    char buf[4096];
    while (PeekNamedPipe(rd, nullptr, 0, nullptr, &avail, nullptr) && avail > 0) {
        DWORD got = 0;
        if (!ReadFile(rd, buf, (DWORD)sizeof(buf), &got, nullptr) || got == 0) break;
        out.append(buf, got);
    }
    if (exitCode) GetExitCodeProcess(pi.hProcess, exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(rd);
    return out;
}

bool RegGetDword(HKEY root, const wchar_t* path, const wchar_t* name, DWORD& out) {
    HKEY k;
    if (RegOpenKeyExW(root, path, 0, KEY_READ, &k) != ERROR_SUCCESS) return false;
    DWORD type = 0, size = sizeof(DWORD), v = 0;
    LONG r = RegQueryValueExW(k, name, nullptr, &type, (LPBYTE)&v, &size);
    RegCloseKey(k);
    if (r != ERROR_SUCCESS || type != REG_DWORD) return false;
    out = v;
    return true;
}

bool RegSetDword(HKEY root, const wchar_t* path, const wchar_t* name, DWORD value) {
    HKEY k;
    if (RegCreateKeyExW(root, path, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS) return false;
    LONG r = RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
    RegCloseKey(k);
    return r == ERROR_SUCCESS;
}

bool RegDelValue(HKEY root, const wchar_t* path, const wchar_t* name) {
    HKEY k;
    if (RegOpenKeyExW(root, path, 0, KEY_SET_VALUE, &k) != ERROR_SUCCESS) return false;
    LONG r = RegDeleteValueW(k, name);
    RegCloseKey(k);
    return r == ERROR_SUCCESS;
}

std::wstring RegGetString(HKEY root, const wchar_t* path, const wchar_t* name) {
    HKEY k;
    if (RegOpenKeyExW(root, path, 0, KEY_READ, &k) != ERROR_SUCCESS) return L"";
    wchar_t buf[512] = {};
    DWORD type = 0, size = sizeof(buf) - sizeof(wchar_t);
    LONG r = RegQueryValueExW(k, name, nullptr, &type, (LPBYTE)buf, &size);
    RegCloseKey(k);
    if (r != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return L"";
    return buf;
}

} // namespace util
