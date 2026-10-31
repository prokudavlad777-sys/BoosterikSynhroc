#include "Optimizer.h"
#include "Cleaner.h"
#include "Power.h"
#include "../Backup/Backup.h"
#include "../Permissions/Permissions.h"
#include "../Logger/Logger.h"
#include "../Core/Util.h"
#include <cstdio>
#include <cwchar>

namespace Optimizer {

static const wchar_t* kGameBar = L"Software\\Microsoft\\GameBar";
static const wchar_t* kGameCfg = L"System\\GameConfigStore";
static const wchar_t* kGameDvr = L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR";
static const wchar_t* kPersonalize = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
static const wchar_t* kSerialize = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Serialize";
static const wchar_t* kMmcss = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile";
static const wchar_t* kMmcssGames = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games";

const std::vector<RegTweak>& Tweaks() {
    static const std::vector<RegTweak> t = {
        {L"gm_auto",   Level::Easy,   HKEY_CURRENT_USER,  kGameBar, L"AutoGameModeEnabled", 1, false},
        {L"gm_allow",  Level::Easy,   HKEY_CURRENT_USER,  kGameBar, L"AllowAutoGameMode", 1, false},
        {L"dvr_cfg",   Level::Easy,   HKEY_CURRENT_USER,  kGameCfg, L"GameDVR_Enabled", 0, false},
        {L"appcap",    Level::Medium, HKEY_CURRENT_USER,  kGameDvr, L"AppCaptureEnabled", 0, false},
        {L"transp",    Level::Medium, HKEY_CURRENT_USER,  kPersonalize, L"EnableTransparency", 0, false},
        {L"startdly",  Level::Medium, HKEY_CURRENT_USER,  kSerialize, L"StartupDelayInMSec", 0, false},
        {L"sysresp",   Level::High,   HKEY_LOCAL_MACHINE, kMmcss, L"SystemResponsiveness", 10, true},
        {L"nothrottle",Level::High,   HKEY_LOCAL_MACHINE, kMmcss, L"NetworkThrottlingIndex", 0xFFFFFFFF, true},
        {L"games_gpu", Level::High,   HKEY_LOCAL_MACHINE, kMmcssGames, L"GPU Priority", 8, true},
        {L"games_prio",Level::High,   HKEY_LOCAL_MACHINE, kMmcssGames, L"Priority", 6, true},
    };
    return t;
}

const wchar_t* LevelName(Level l) {
    switch (l) {
        case Level::Easy: return L"Легкий";
        case Level::Medium: return L"Середній";
        default: return L"Високий";
    }
}

std::vector<std::wstring> Describe(Level l) {
    switch (l) {
        case Level::Easy:
            return {L"Схема живлення: висока продуктивність", L"Windows Game Mode: увімкнено",
                    L"Game DVR: фоновий запис вимкнено", L"Очищення тимчасових файлів користувача"};
        case Level::Medium:
            return {L"Xbox Game Bar не захоплює ігри", L"Прозорість та анімації вимкнено",
                    L"Швидший старт робочого столу", L"Очищення дампів збоїв"};
        default:
            return {L"План «Максимальна продуктивність»", L"MMCSS: пріоритет для ігор",
                    L"Вимкнення мережевого тротлінгу", L"Очищення Windows Temp"};
    }
}

static void ApplyAnimations(bool enable) {
    SystemParametersInfoW(SPI_SETCLIENTAREAANIMATION, 0, (PVOID)(INT_PTR)(enable ? TRUE : FALSE),
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    ANIMATIONINFO ai{sizeof(ai), enable ? 1 : 0};
    SystemParametersInfoW(SPI_SETANIMATION, sizeof(ai), &ai, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

void SetAnimations(bool enable) { ApplyAnimations(enable); }

OptResult Apply(Level level) {
    OptResult r;
    Backup::EnsureExists();                         // original state is saved once, before the first change
    const bool admin = Permissions::IsAdmin();
    const wchar_t* lv = LevelName(level);
    Log::Write(std::wstring(L"Оптимізація: рівень ") + lv);

    for (const RegTweak& t : Tweaks()) {
        if ((int)t.level > (int)level) continue;
        if (t.needsAdmin && !admin) {
            ++r.skipped;
            r.lines.push_back(std::wstring(L"Пропущено (потрібен адміністратор): ") + t.id);
            continue;
        }
        if (util::RegSetDword(t.root, t.path, t.name, t.value)) ++r.ok;
        else { ++r.failed; r.lines.push_back(std::wstring(L"Помилка запису: ") + t.id); }
    }

    bool power = false;
    if (level == Level::High && admin) power = Power::EnableUltimate();
    if (!power) power = Power::EnableHighPerformance();
    if (power) ++r.ok; else { ++r.failed; r.lines.push_back(L"Не вдалося змінити схему живлення"); }

    if ((int)level >= (int)Level::Medium) { ApplyAnimations(false); ++r.ok; }

    r.freed += Cleaner::CleanPath(Cleaner::UserTemp());
    if ((int)level >= (int)Level::Medium) r.freed += Cleaner::CleanPath(Cleaner::CrashDumps());
    if (level == Level::High) {
        if (admin) r.freed += Cleaner::CleanPath(Cleaner::WindowsTemp());
        else { ++r.skipped; r.lines.push_back(L"Windows Temp пропущено (потрібен адміністратор)"); }
    }

    wchar_t sum[256];
    swprintf(sum, 256, L"Готово: застосовано %d, пропущено %d, помилок %d, звільнено %ls", r.ok, r.skipped, r.failed,
             util::FormatBytes(r.freed).c_str());
    Log::Write(sum);
    r.lines.insert(r.lines.begin(), sum);
    return r;
}
}
