#pragma once
#include <windows.h>
#include <string>
#include <vector>

enum class Level { Easy = 0, Medium = 1, High = 2 };

struct RegTweak {
    const wchar_t* id;
    Level level;
    HKEY root;
    const wchar_t* path;
    const wchar_t* name;
    DWORD value;
    bool needsAdmin;
};

struct OptResult {
    int ok = 0, skipped = 0, failed = 0;
    unsigned long long freed = 0;
    std::vector<std::wstring> lines;
};

namespace Optimizer {
const std::vector<RegTweak>& Tweaks();
void SetAnimations(bool enable);
const wchar_t* LevelName(Level l);
std::vector<std::wstring> Describe(Level l);   // what this level adds (for the UI cards)
OptResult Apply(Level l);                      // cumulative: High includes Medium and Easy
}
