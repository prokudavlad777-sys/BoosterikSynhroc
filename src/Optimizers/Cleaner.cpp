#include "Cleaner.h"
#include "../Core/Util.h"
#include "../Logger/Logger.h"
#include <shellapi.h>
#include <shlobj.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace Cleaner {

static std::wstring StripSlash(std::wstring s) {
    while (s.size() > 3 && (s.back() == L'\\' || s.back() == L'/')) s.pop_back();
    return s;
}

std::wstring UserTemp() {
    wchar_t b[MAX_PATH + 2] = {};
    DWORD n = GetTempPathW(MAX_PATH, b);
    return n ? StripSlash(b) : L"";
}

std::wstring WindowsTemp() {
    wchar_t b[MAX_PATH] = {};
    UINT n = GetWindowsDirectoryW(b, MAX_PATH);
    return n ? std::wstring(b) + L"\\Temp" : L"";
}

static std::wstring LocalAppData(const wchar_t* sub) {
    wchar_t b[MAX_PATH] = {};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", b, MAX_PATH);
    return n ? std::wstring(b) + L"\\" + sub : L"";
}

std::wstring CrashDumps()  { return LocalAppData(L"CrashDumps"); }
std::wstring ShaderCache() { return LocalAppData(L"D3DSCache"); }

static unsigned long long DirSize(const fs::path& p) {
    unsigned long long total = 0;
    std::error_code ec;
    fs::recursive_directory_iterator it(p, fs::directory_options::skip_permission_denied, ec), end;
    for (; !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        if (it->is_regular_file(e2) && !e2) {
            auto sz = it->file_size(e2);
            if (!e2) total += sz;
        }
    }
    return total;
}

static bool SafeTarget(const std::wstring& path) {
    std::wstring l = util::ToLower(path);
    if (l.size() < 8) return false;
    return l.find(L"temp") != std::wstring::npos || l.find(L"crashdumps") != std::wstring::npos ||
           l.find(L"d3dscache") != std::wstring::npos;
}

unsigned long long CleanPath(const std::wstring& path) {
    if (path.empty() || !SafeTarget(path)) return 0;
    std::error_code ec;
    if (!fs::exists(path, ec)) return 0;
    unsigned long long before = DirSize(path);
    fs::directory_iterator it(path, fs::directory_options::skip_permission_denied, ec), end;
    for (; !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        fs::remove_all(it->path(), e2);      // locked files are simply skipped
    }
    unsigned long long after = DirSize(path);
    return before > after ? before - after : 0;
}

std::vector<CleanCategory> Categories() {
    std::vector<CleanCategory> v;
    v.push_back({L"utemp", L"Тимчасові файли користувача", UserTemp(), L"", false, true});
    v.push_back({L"wtemp", L"Тимчасові файли Windows", WindowsTemp(), L"Потрібні права адміністратора", true, true});
    v.push_back({L"dumps", L"Дампи збоїв", CrashDumps(), L"", false, true});
    v.push_back({L"shader", L"Кеш шейдерів DirectX", ShaderCache(), L"Після очищення перший запуск ігор буде трохи повільнішим", false, false});
    v.push_back({L"recycle", L"Кошик", L"", L"", false, false});
    return v;
}

unsigned long long Scan(CleanCategory& c) {
    if (c.id == L"recycle") {
        SHQUERYRBINFO info{sizeof(info)};
        c.size = SUCCEEDED(SHQueryRecycleBinW(nullptr, &info)) ? (unsigned long long)info.i64Size : 0;
    } else {
        std::error_code ec;
        c.size = (!c.path.empty() && fs::exists(c.path, ec)) ? DirSize(c.path) : 0;
    }
    c.scanned = true;
    return c.size;
}

unsigned long long Clean(CleanCategory& c) {
    unsigned long long freed = 0;
    if (c.id == L"recycle") {
        SHQUERYRBINFO info{sizeof(info)};
        unsigned long long before = SUCCEEDED(SHQueryRecycleBinW(nullptr, &info)) ? (unsigned long long)info.i64Size : 0;
        if (SUCCEEDED(SHEmptyRecycleBinW(nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND)))
            freed = before;
    } else {
        freed = CleanPath(c.path);
    }
    c.size = c.size > freed ? c.size - freed : 0;
    Log::Write(L"Очищення [" + c.id + L"]: " + util::FormatBytes(freed));
    return freed;
}
}
