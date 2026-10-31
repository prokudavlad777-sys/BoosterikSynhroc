#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>

namespace util {

std::wstring ExeDir();                         // folder of the exe, no trailing slash
std::wstring Path(const std::wstring& rel);    // ExeDir() + L"\\" + rel
std::wstring FormatBytes(unsigned long long b);
bool FileExists(const std::wstring& p);
void EnsureDir(const std::wstring& dir);
std::wstring Utf8ToWide(const std::string& s);
std::string  WideToUtf8(const std::wstring& s);
std::wstring ToLower(std::wstring s);

// Runs a hidden console process and returns its stdout.
std::string RunCapture(const std::wstring& cmdline, DWORD timeoutMs = 10000, DWORD* exitCode = nullptr);

bool RegGetDword(HKEY root, const wchar_t* path, const wchar_t* name, DWORD& out);
bool RegSetDword(HKEY root, const wchar_t* path, const wchar_t* name, DWORD value);
bool RegDelValue(HKEY root, const wchar_t* path, const wchar_t* name);
std::wstring RegGetString(HKEY root, const wchar_t* path, const wchar_t* name);

} // namespace util
