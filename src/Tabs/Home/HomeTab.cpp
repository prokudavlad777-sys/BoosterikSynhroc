#include "../Tabs.h"
#include "../GameMode/GameMode.h"
#include "../../Core/App.h"
#include "../../Core/Util.h"
#include "../../Hardware/Hardware.h"
#include "../../Monitoring/Monitor.h"
#include "../../Optimizers/Optimizer.h"
#include <cstdio>
#include <cwchar>

using namespace Gdiplus;

namespace {
class HomeTab : public Tab {
    std::wstring status_ = L"Готово до роботи. Почніть зі «Швидкої оптимізації».";
public:
    const wchar_t* Title() const override { return L"Головна"; }
    const wchar_t* Subtitle() const override { return L"Огляд системи та швидкі дії"; }
    const wchar_t* IconName() const override { return L"home"; }

    void Draw(Graphics& g, const RectF& a) override {
        Begin();
        const HwInfo& hw = Hardware::Get();
        Monitor& mon = Monitor::I();

        RectF c(a.X, a.Y, a.Width, 152);
        ui::Card(g, c, L"Система");
        wchar_t ram[96];
        swprintf(ram, 96, L"%ls · потоків ЦП: %u", util::FormatBytes(hw.ramTotal).c_str(), hw.cores);
        struct Row { const wchar_t* k; std::wstring v; };
        Row rows[] = {{L"Процесор", hw.cpu}, {L"Відеокарта", hw.gpu}, {L"Пам'ять", ram}, {L"Система", hw.os}};
        for (int i = 0; i < 4; ++i) {
            float y = c.Y + 44 + i * 25;
            ui::Text(g, rows[i].k, RectF(c.X + 18, y, 110, 22), 14, ui::col::muted);
            ui::Text(g, rows[i].v, RectF(c.X + 140, y, c.Width - 160, 22), 14, ui::col::text);
        }

        float hw2 = (a.Width - 16) / 2;
        RectF cc(a.X, a.Y + 168, hw2, 176), mc(a.X + hw2 + 16, a.Y + 168, hw2, 176);
        ui::Card(g, cc, L"Навантаження ЦП");
        wchar_t t[48];
        swprintf(t, 48, L"%.0f%%", mon.Cpu());
        ui::Text(g, t, RectF(cc.X + 18, cc.Y + 38, 160, 48), 36, ui::col::accent, true);
        RectF gr(cc.X + 18, cc.Y + 92, cc.Width - 36, 70);
        ui::Fill(g, gr, 8, ui::col::card2);
        const auto& h = mon.CpuHistory();
        std::vector<PointF> pts;
        for (size_t i = 0; i < h.size(); ++i) {
            float x = gr.X + gr.Width - (float)(h.size() - 1 - i) * gr.Width / 59.f;
            float y = gr.Y + gr.Height - 4 - (float)(h[i] / 100.0) * (gr.Height - 8);
            pts.push_back(PointF(x, y));
        }
        if (pts.size() >= 2) {
            Pen pen(Color(ui::col::accent), 2.f);
            g.DrawLines(&pen, pts.data(), (INT)pts.size());
        }

        ui::Card(g, mc, L"Оперативна пам'ять");
        swprintf(t, 48, L"%.0f%%", mon.RamPercent());
        ui::Text(g, t, RectF(mc.X + 18, mc.Y + 38, 160, 48), 36, ui::col::accent, true);
        ui::Bar(g, RectF(mc.X + 18, mc.Y + 100, mc.Width - 36, 12), mon.RamPercent() / 100.0,
                mon.RamPercent() > 85 ? ui::col::danger : ui::col::accent);
        ui::Text(g, util::FormatBytes(mon.RamUsed()) + L" з " + util::FormatBytes(mon.RamTotal()),
                 RectF(mc.X + 18, mc.Y + 124, mc.Width - 36, 22), 14, ui::col::muted);

        RectF q(a.X, a.Y + 360, a.Width, 150);
        ui::Card(g, q, L"Швидкі дії");
        float bw = (q.Width - 36 - 24) / 3;
        Btn(g, RectF(q.X + 18, q.Y + 48, bw, 44), L"Швидка оптимізація", 1, true, !Busy());
        Btn(g, RectF(q.X + 18 + bw + 12, q.Y + 48, bw, 44),
            GameMode::IsOn() ? L"Game Mode: вимкнути" : L"Game Mode: увімкнути", 2, false);
        Btn(g, RectF(q.X + 18 + 2 * (bw + 12), q.Y + 48, bw, 44), L"Знайти втрати FPS", 3, false);
        std::wstring st;
        { std::lock_guard<std::mutex> lk(mu_); st = Busy() ? L"Виконую оптимізацію…" : status_; }
        ui::Text(g, st, RectF(q.X + 18, q.Y + 104, q.Width - 36, 36), 13, ui::col::muted, false,
                 StringAlignmentNear, StringAlignmentNear, true);
    }

    void OnButton(int id) override {
        if (id == 1) {
            RunAsync([this]() {
                OptResult r = Optimizer::Apply(Level::Easy);
                std::lock_guard<std::mutex> lk(mu_);
                status_ = L"Легкий рівень: " + (r.lines.empty() ? std::wstring(L"готово") : r.lines.front());
            });
        } else if (id == 2) {
            if (GameMode::IsOn()) GameMode::Disable(); else GameMode::Enable();
            std::lock_guard<std::mutex> lk(mu_);
            status_ = L"Game Mode: " + GameMode::Status();
        } else if (id == 3) {
            app::scanRequested = true;
            if (app::goTab) app::goTab(app::Acceleration);
        }
    }
};
}

std::unique_ptr<Tab> MakeHomeTab() { return std::make_unique<HomeTab>(); }
