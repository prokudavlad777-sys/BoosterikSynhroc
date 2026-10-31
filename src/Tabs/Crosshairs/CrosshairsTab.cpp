#include "../Tabs.h"
#include "Overlay.h"
#include "../../Core/Util.h"
#include <shellapi.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cwchar>

using namespace Gdiplus;

static Crosshair::Overlay* g_overlay = nullptr;
void ShutdownCrosshairs() { if (g_overlay) g_overlay->Destroy(); }

namespace {
class CrosshairsTab : public Tab {
    CrosshairSettings s_;
    Crosshair::Overlay overlay_;
    std::vector<std::wstring> customs_;
    std::unique_ptr<Bitmap> customBmp_;

    static std::wstring Dir() { return util::Path(L"custom\\Crosshairs"); }

    void Scan() {
        customs_.clear();
        util::EnsureDir(util::Path(L"custom"));
        util::EnsureDir(Dir());
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW((Dir() + L"\\*.png").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do { customs_.push_back(Dir() + L"\\" + fd.cFileName); } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
        std::sort(customs_.begin(), customs_.end());
    }
    void Refresh() { if (overlay_.Visible()) overlay_.Show(s_, customBmp_.get()); }
public:
    CrosshairsTab() { g_overlay = &overlay_; Scan(); }
    ~CrosshairsTab() override { g_overlay = nullptr; }
    const wchar_t* Title() const override { return L"Приціли"; }
    const wchar_t* Subtitle() const override { return L"Накладка-приціл по центру екрана"; }
    const wchar_t* IconName() const override { return L"crosshairs"; }

    void Draw(Graphics& g, const RectF& a) override {
        Begin();
        float lw = 470;
        RectF lc(a.X, a.Y, lw, 480), rc(a.X + lw + 16, a.Y, a.Width - lw - 16, 250);
        ui::Card(g, lc, L"Налаштування");
        const wchar_t* names[4] = {L"Хрест", L"Крапка", L"Коло", L"Хрест + крапка"};
        float bw = (lc.Width - 36 - 10) / 2;
        for (int i = 0; i < 4; ++i)
            Btn(g, RectF(lc.X + 18 + (i % 2) * (bw + 10), lc.Y + 46 + (i / 2) * 44, bw, 36), names[i], 10 + i, s_.style != i);

        ui::Text(g, L"Колір", RectF(lc.X + 18, lc.Y + 142, 100, 22), 13, ui::col::muted);
        for (int i = 0; i < 5; ++i) {
            RectF sw(lc.X + 18 + i * 46, lc.Y + 168, 34, 34);
            Area(sw, 20 + i);
            ui::Fill(g, sw, 17, Crosshair::kColors[i]);
            if (s_.colorIdx == i) ui::Stroke(g, RectF(sw.X - 4, sw.Y - 4, 42, 42), 21, ui::col::text, 2);
        }

        struct Sl { const wchar_t* name; int id; float v01; float val; const wchar_t* unit; };
        Sl sl[3] = {{L"Розмір", 30, (s_.size - 6) / 34, s_.size, L"px"},
                    {L"Товщина", 31, (s_.thickness - 1) / 5, s_.thickness, L"px"},
                    {L"Проміжок", 32, s_.gap / 14, s_.gap, L"px"}};
        for (int i = 0; i < 3; ++i) {
            float y = lc.Y + 222 + i * 44;
            ui::Text(g, sl[i].name, RectF(lc.X + 18, y, 100, 28), 14, ui::col::text, false, StringAlignmentNear, StringAlignmentCenter);
            Slide(g, RectF(lc.X + 124, y + 2, lc.Width - 124 - 80, 24), sl[i].id, sl[i].v01);
            wchar_t b[32];
            swprintf(b, 32, L"%.0f %ls", sl[i].val, sl[i].unit);
            ui::Text(g, b, RectF(lc.X + lc.Width - 70, y, 56, 28), 13, ui::col::muted, false, StringAlignmentFar, StringAlignmentCenter);
        }

        ui::Text(g, L"Свої приціли (PNG у custom\\Crosshairs)", RectF(lc.X + 18, lc.Y + 362, lc.Width - 36, 22), 13, ui::col::muted);
        if (customs_.empty())
            ui::Text(g, L"Файлів немає. Додайте PNG з прозорим фоном і натисніть «Оновити».",
                     RectF(lc.X + 18, lc.Y + 388, lc.Width - 36, 40), 12, ui::col::muted, false, StringAlignmentNear, StringAlignmentNear, true);
        for (int i = 0; i < (int)customs_.size() && i < 3; ++i) {
            std::wstring n = customs_[i].substr(customs_[i].find_last_of(L"\\/") + 1);
            Btn(g, RectF(lc.X + 18 + i * 144, lc.Y + 388, 136, 34), n, 40 + i, s_.style != 100 + i);
        }
        Btn(g, RectF(lc.X + 18, lc.Y + 432, 140, 34), L"Оновити", 50, false);
        Btn(g, RectF(lc.X + 168, lc.Y + 432, 180, 34), L"Відкрити папку", 51, false);

        ui::Card(g, rc, L"Перегляд");
        RectF box(rc.X + 18, rc.Y + 44, rc.Width - 36, 140);
        ui::Fill(g, box, 10, 0xFF22252E);
        GraphicsState st = g.Save();
        g.SetClip(box);
        Crosshair::Render(g, box.X + box.Width / 2, box.Y + box.Height / 2, s_, customBmp_.get());
        g.Restore(st);
        Btn(g, RectF(rc.X + 18, rc.Y + 196, rc.Width - 36, 40), overlay_.Visible() ? L"Сховати з екрана" : L"Показати на екрані",
            60, !overlay_.Visible());

        RectF n(a.X + lw + 16, a.Y + 266, a.Width - lw - 16, 120);
        ui::Card(g, n);
        ui::Text(g, L"Накладка не перехоплює кліки. Вона працює над іграми у віконному та безрамковому режимі; "
                    L"в ексклюзивному повноекранному режимі деякі ігри її ховають.",
                 RectF(n.X + 18, n.Y + 14, n.Width - 36, n.Height - 28), 12, ui::col::muted, false,
                 StringAlignmentNear, StringAlignmentNear, true);
    }

    void OnButton(int id) override {
        if (id >= 10 && id < 14) { s_.style = id - 10; customBmp_.reset(); }
        else if (id >= 20 && id < 25) s_.colorIdx = id - 20;
        else if (id >= 40 && id < 43) {
            size_t i = (size_t)(id - 40);
            if (i < customs_.size()) {
                std::unique_ptr<Bitmap> b(Bitmap::FromFile(customs_[i].c_str()));
                if (b && b->GetLastStatus() == Ok) { customBmp_ = std::move(b); s_.style = 100 + (int)i; }
            }
        } else if (id == 50) { Scan(); }
        else if (id == 51) { ShellExecuteW(nullptr, L"open", Dir().c_str(), nullptr, nullptr, SW_SHOWNORMAL); }
        else if (id == 60) {
            if (overlay_.Visible()) overlay_.Hide(); else overlay_.Show(s_, customBmp_.get());
        }
        Refresh();
    }

    void OnSlider(int id, float v) override {
        if (id == 30) s_.size = std::floor(6 + v * 34 + 0.5f);
        else if (id == 31) s_.thickness = std::floor(1 + v * 5 + 0.5f);
        else if (id == 32) s_.gap = std::floor(v * 14 + 0.5f);
        Refresh();
    }
};
}

std::unique_ptr<Tab> MakeCrosshairsTab() { return std::make_unique<CrosshairsTab>(); }
