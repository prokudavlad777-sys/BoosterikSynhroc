#include "Core/Util.h"
#include <cstdio>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } } while (0)

int main() {
    CHECK(util::FormatBytes(0) == L"0 Б");
    CHECK(util::FormatBytes(1024) == L"1.0 КБ");
    CHECK(util::FormatBytes(5ull * 1024 * 1024) == L"5.0 МБ");
    CHECK(util::ToLower(L"AbC") == L"abc");
    CHECK(util::Utf8ToWide(util::WideToUtf8(L"Привіт")) == L"Привіт");
    CHECK(!util::ExeDir().empty());
    std::printf(fails ? "FAILED\n" : "OK\n");
    return fails;
}
