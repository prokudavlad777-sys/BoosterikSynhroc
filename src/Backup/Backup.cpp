#include "Backup.h"
#include "../Optimizers/Optimizer.h"
#include "../Optimizers/Power.h"
#include "../Core/Util.h"
#include "../Logger/Logger.h"
#include <cstdio>
#include <cwchar>

namespace Backup {

std::wstring FilePath() { return util::Path(L"configs\\backup.ini"); }
bool Exists() { return util::FileExists(FilePath()); }

bool Create() {
    util::EnsureDir(util::Path(L"configs"));
    const std::wstring f = FilePath();
    bool ok = true;
    for (const RegTweak& t : Optimizer::Tweaks()) {
        DWORD v = 0;
        bool ex = util::RegGetDword(t.root, t.path, t.name, v);
        wchar_t val[48];
        swprintf(val, 48, L"%d|%lu", ex ? 1 : 0, (unsigned long)(ex ? v : 0));
        ok &= WritePrivateProfileStringW(L"reg", t.id, val, f.c_str()) != FALSE;
    }
    ok &= WritePrivateProfileStringW(L"power", L"scheme", Power::GetActive().c_str(), f.c_str()) != FALSE;

    BOOL clientAnim = TRUE;
    SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &clientAnim, 0);
    ANIMATIONINFO ai{sizeof(ai), 1};
    SystemParametersInfoW(SPI_GETANIMATION, sizeof(ai), &ai, 0);
    WritePrivateProfileStringW(L"spi", L"clientanim", clientAnim ? L"1" : L"0", f.c_str());
    WritePrivateProfileStringW(L"spi", L"minanim", ai.iMinAnimate ? L"1" : L"0", f.c_str());

    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t when[48];
    swprintf(when, 48, L"%04d-%02d-%02d %02d:%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute);
    WritePrivateProfileStringW(L"meta", L"created", when, f.c_str());

    Log::Write(ok ? L"Резервну копію створено" : L"Резервну копію створено з помилками");
    return ok;
}

bool EnsureExists() { return Exists() || Create(); }

std::wstring LastWriteText() {
    WIN32_FILE_ATTRIBUTE_DATA d;
    if (!GetFileAttributesExW(FilePath().c_str(), GetFileExInfoStandard, &d)) return L"немає";
    FILETIME lf;
    SYSTEMTIME st;
    FileTimeToLocalFileTime(&d.ftLastWriteTime, &lf);
    FileTimeToSystemTime(&lf, &st);
    wchar_t b[48];
    swprintf(b, 48, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return b;
}
}
