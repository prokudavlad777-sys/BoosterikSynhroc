#pragma once
#include <string>
namespace Backup {
std::wstring FilePath();
bool Exists();
bool Create();           // (over)writes configs/backup.ini with the current system state
bool EnsureExists();     // creates the backup only if none exists yet
std::wstring LastWriteText();
}
