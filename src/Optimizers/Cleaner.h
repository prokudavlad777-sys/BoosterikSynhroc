#pragma once
#include <string>
#include <vector>

struct CleanCategory {
    std::wstring id, title, path, note;
    bool needsAdmin = false;
    bool selected = true;
    bool scanned = false;
    unsigned long long size = 0;
};

namespace Cleaner {
std::wstring UserTemp();
std::wstring WindowsTemp();
std::wstring CrashDumps();
std::wstring ShaderCache();

std::vector<CleanCategory> Categories();
unsigned long long Scan(CleanCategory& c);     // fills c.size, returns it
unsigned long long Clean(CleanCategory& c);    // returns freed bytes
unsigned long long CleanPath(const std::wstring& path);
}
