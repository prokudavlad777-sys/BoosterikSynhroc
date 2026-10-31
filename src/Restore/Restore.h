#pragma once
#include <string>
#include <vector>
namespace Restore {
struct Result {
    int restored = 0, skipped = 0, failed = 0;
    std::vector<std::wstring> lines;
};
Result Run();   // puts everything back from configs/backup.ini
}
