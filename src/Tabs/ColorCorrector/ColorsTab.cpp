#include "../Tabs.h"
#include "../../Core/Util.h"
#include <magnification.h>
#include <shellapi.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cwchar>

using namespace Gdiplus;

namespace {
struct ColorParams {
    float exposure = 0, gamma = 1, shadows = 0, contrast = 1, saturation = 1;
};

static bool g_magInit = false;
static WORD g_orig[3][256];
static bool g_haveOrig = false;
static bool g_touched = false;

static void SaveOriginal() {
    if (g_haveOrig) return;
    HDC dc = GetDC(nullptr);
    g_haveOrig = GetDeviceGammaRamp(dc, g_orig) != FALSE;
    ReleaseDC(nullptr, dc);
}

static bool ApplyColors(const ColorParams& p) {
    SaveOriginal();
    WORD ramp[3][256];
    for (int i = 0; i < 256; ++i) {
        double x = i / 255.0;
        x *= std::pow(2.0, p.exposure);
        x = std::pow(std::min(1.0, std::max(0.0, x)), 1.0 / p.gamma);
        x += p.shadows * (1 - x) * (1 - x);
        x = (x - 0.5) * p.contrast + 0.5;
        x = std::min(1.0, std::max(0.0, x));
        WORD v = (WORD)(x * 65535.0 + 0.5);
        ramp[0][i] = ramp[1][i] = ramp[2][i] = v;
    }
    HDC dc = GetDC(nullptr);
    bool ok = SetDeviceGammaRamp(dc, ramp) != FALSE;
    ReleaseDC(nullptr, dc);

    // saturation through the Magnification API colour matrix
    if (!g_magInit) g_magInit = MagInitialize() != FALSE;
    if (g_magInit) {
        const float lr = 0.3086f, lg = 0.6094f, lb = 0.0820f, s = p.saturation, t = 1 - s;
        MAGCOLOREFFECT m = {{{lr * t + s, lr * t, lr * t, 0, 0},
                             {lg * t, lg * t + s, lg * t, 0, 0},
                             {lb * t, lb * t, lb * t + s, 0, 0},
                             {0, 0, 0, 1, 0},
                             {0, 0, 0, 0, 1}}};
        MagSetFullscreenColorEffect(&m);
    }
    g_touched = true;
    return ok;
}

static void ResetColors() {
    if (g_haveOrig) {
        HDC dc = GetDC(nullptr);
        SetDeviceGammaRamp(dc, g_orig);
        ReleaseDC(nullptr, dc);
    }
    if (g_magInit) {
        MAGCOLOREFFECT id = {{{1, 0, 0, 0, 0}, {0, 1, 0, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 1, 0}, {0, 0, 0, 0, 1}}};
        MagSetFullscreenColorEffect(&id);
        MagUninitialize();
        g_magInit = false;
    }
    g_touched = false;
}

class ColorsTab : public Tab {
    ColorParams p_;
    std::vector<std::wstring> profiles_;
    std::wstring status_ = L"Рухайте повзунки — зміни видно одразу.";

    static std::wstring Dir() { return util::Path(L"custom\\ColorProfiles"); }
    static std::wstring Stem(const std::wstring& f) {
        std::wstring n = f.substr(f.find_last_of(L"\\/") + 1);
        size_t d = n.find_last_of(L'.');
        return d == std::wstring::npos ? n : n.substr(0, d);
    }
    void Scan() {
        profiles_.clear();
        util::EnsureDir(util::Path(L"custom"));
        util::EnsureDir(Dir());
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW((Dir() + L"\\*.ini").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do { profiles_.push_back(Dir() + L"\\" + fd.cFileName); } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
        std::sort(profiles_.begin(), profiles_.end());
    }
    static float Get(const std::wstring& f, const wchar_t* key, float def) {
        wchar_t b[64] = {};
        GetPrivateProfileStringW(L"color", key, L"", b, 64, f.c_str());
        return b[0] ? (float)_wtof(b) : def;
    }
    static void Put(const std::wstring& f, const wchar_t* key, float v) {
        wchar_t b[64];
        swprintf(b, 64, L"%.3f", v);
        WritePrivateProfileStringW(L"color", key, b, f.c_str());
    }
    void Apply() { status_ = ApplyColors(p_) ? L"Застосовано." : L"Windows або драйвер відхилили ці значення — зменште зміни."; }
public:
    ColorsTab() { Scan(); }
    const wchar_t* Title() const override { return L"Кольори"; }
    const wchar_t* Subtitle() const override { return L"Корекція кольору: експозиція, гамма, тіні, насиченість"; }
    const wchar_t* IconName() const override { return L"colors"; }

    void Draw(Graphics& g, const RectF& a) override {
        Begin();
        float rw = 250;
        RectF lc(a.X, a.Y, a.Width - rw - 16, 430), rc(a.X + lc.Width + 16, a.Y, rw, 430);
        ui::Card(g, lc, L"Параметри");
        struct Sl { const wchar_t* name; int id; float v, lo, hi; const wchar_t* fmt; };
        Sl sl[5] = {{L"Експозиція", 1, p_.exposure, -0.5f, 0.5f, L"%+.2f EV"},
                    {L"Гамма", 2, p_.gamma, 0.7f, 1.6f, L"%.2f"},
                    {L"Тіні", 3, p_.shadows, -0.05f, 0.15f, L"%+.2f"},
                    {L"Контраст", 4, p_.contrast, 0.8f, 1.3f, L"%.2f"},
                    {L"Насиченість", 5, p_.saturation, 0.0f, 2.0f, L"%.2f"}};
        for (int i = 0; i < 5; ++i) {
            float y = lc.Y + 54 + i * 62;
            ui::Text(g, sl[i].name, RectF(lc.X + 18, y, 130, 30), 14, ui::col::text, false, StringAlignmentNear, StringAlignmentCenter);
            Slide(g, RectF(lc.X + 150, y + 3, lc.Width - 150 - 100, 24), sl[i].id, (sl[i].v - sl[i].lo) / (sl[i].hi - sl[i].lo));
            wchar_t b[32];
            swprintf(b, 32, sl[i].fmt, sl[i].v);
            ui::Text(g, b, RectF(lc.X + lc.Width - 90, y, 72, 30), 13, ui::col::accent, true, StringAlignmentFar, StringAlignmentCenter);
        }
        ui::Text(g, status_, RectF(lc.X + 18, lc.Y + 368, lc.Width - 36, 50), 12, ui::col::muted, false,
                 StringAlignmentNear, StringAlignmentNear, true);

        ui::Card(g, rc, L"Профілі");
        for (int i = 0; i < (int)profiles_.size() && i < 6; ++i)
            Btn(g, RectF(rc.X + 18, rc.Y + 46 + i * 44, rc.Width - 36, 36), Stem(profiles_[i]), 10 + i, false);
        Btn(g, RectF(rc.X + 18, rc.Y + 318, rc.Width - 36, 40), L"Зберегти поточний", 20, true);
        Btn(g, RectF(rc.X + 18, rc.Y + 368, rc.Width - 36, 40), L"Скинути", 21, false);
        Btn(g, RectF(a.X, a.Y + 446, 220, 36), L"Папка профілів", 22, false);
    }

    void OnSlider(int id, float v) override {
        auto lerp = [&](float lo, float hi) { return lo + v * (hi - lo); };
        auto snap = [](float x) { return std::floor(x * 100 + 0.5f) / 100; };
        if (id == 1) p_.exposure = snap(lerp(-0.5f, 0.5f));
        else if (id == 2) p_.gamma = snap(lerp(0.7f, 1.6f));
        else if (id == 3) p_.shadows = snap(lerp(-0.05f, 0.15f));
        else if (id == 4) p_.contrast = snap(lerp(0.8f, 1.3f));
        else if (id == 5) p_.saturation = snap(lerp(0.0f, 2.0f));
        Apply();
    }

    void OnButton(int id) override {
        if (id >= 10 && id < 16) {
            size_t i = (size_t)(id - 10);
            if (i >= profiles_.size()) return;
            const std::wstring& f = profiles_[i];
            p_.exposure = Get(f, L"exposure", 0); p_.gamma = Get(f, L"gamma", 1);
            p_.shadows = Get(f, L"shadows", 0); p_.contrast = Get(f, L"contrast", 1);
            p_.saturation = Get(f, L"saturation", 1);
            Apply();
        } else if (id == 20) {
            std::wstring f;
            for (int n = 1; n < 100; ++n) {
                f = Dir() + L"\\Custom_" + std::to_wstring(n) + L".ini";
                if (!util::FileExists(f)) break;
            }
            Put(f, L"exposure", p_.exposure); Put(f, L"gamma", p_.gamma); Put(f, L"shadows", p_.shadows);
            Put(f, L"contrast", p_.contrast); Put(f, L"saturation", p_.saturation);
            Scan();
            status_ = L"Збережено: " + Stem(f);
        } else if (id == 21) {
            p_ = ColorParams();
            ResetColors();
            status_ = L"Кольори скинуто.";
        } else if (id == 22) {
            ShellExecuteW(nullptr, L"open", Dir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }
};
}

void ShutdownColors() { if (g_touched) ResetColors(); }
std::unique_ptr<Tab> MakeColorsTab() { return std::make_unique<ColorsTab>(); }
