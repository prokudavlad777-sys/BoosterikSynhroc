#include "Power.h"
#include "../Core/Util.h"
#include <regex>

namespace Power {

static const wchar_t* kHigh     = L"8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c";
static const wchar_t* kUltimate = L"e9a42b02-d5df-448d-aa00-03f14749eb61";
static const wchar_t* kUltCopy  = L"b0057e21-0000-4b00-8000-5ba3b00573e1";   // our own fixed copy
static const wchar_t* kBalanced = L"381b4222-f694-41f0-9685-ff5bb260df2e";
static const wchar_t* kSaver    = L"a1841308-3541-4fab-bc81-f71556f20b4a";

static std::wstring FindGuid(const std::string& s) {
    static const std::regex re("[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}");
    std::smatch m;
    if (std::regex_search(s, m, re)) return util::ToLower(util::Utf8ToWide(m.str()));
    return L"";
}

std::wstring GetActive() { return FindGuid(util::RunCapture(L"powercfg /getactivescheme")); }

bool SetActive(const std::wstring& guid) {
    if (guid.empty()) return false;
    DWORD code = 1;
    util::RunCapture(L"powercfg /setactive " + guid, 10000, &code);
    return code == 0;
}

bool EnableHighPerformance() {
    if (SetActive(kHigh)) return true;
    // Some laptops hide the High performance plan: create a copy of it.
    std::wstring g = FindGuid(util::RunCapture(std::wstring(L"powercfg -duplicatescheme ") + kHigh));
    return !g.empty() && SetActive(g);
}

bool EnableUltimate() {
    util::RunCapture(std::wstring(L"powercfg -duplicatescheme ") + kUltimate + L" " + kUltCopy);
    return SetActive(kUltCopy);
}

bool IsBalancedOrSaver(const std::wstring& guid) { return guid == kBalanced || guid == kSaver; }
}
