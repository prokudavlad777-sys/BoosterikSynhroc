#pragma once
#include <string>
namespace Power {
std::wstring GetActive();                       // active scheme GUID (lowercase, no braces) or empty
bool SetActive(const std::wstring& guid);
bool EnableHighPerformance();                   // built-in High performance (or a copy of it)
bool EnableUltimate();                          // Ultimate performance (fixed copy GUID, reused)
bool IsBalancedOrSaver(const std::wstring& guid);
}
