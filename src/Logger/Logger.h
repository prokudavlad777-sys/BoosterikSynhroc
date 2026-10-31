#pragma once
#include <string>
namespace Log {
void Init();
void Write(const std::wstring& line);
std::wstring FilePath();
}
