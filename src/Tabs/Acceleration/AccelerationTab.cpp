#include "../Tabs.h"
#include "../GameMode/GameMode.h"
#include "../../Core/App.h"
#include "../../Monitoring/Monitor.h"
#include "../../Optimizers/Optimizer.h"
#include "../../Permissions/Permissions.h"
#include "../../Restore/Restore.h"

using namespace Gdiplus;

namespace {
class AccelerationTab : public Tab {
    std::wstring status_ = L"Оберіть рівень. Перед першою зміною автоматично зберігається резервна копія.";
    std::vector<Finding> findings_;
public:
    const wchar_t* Title() const override { return L"Прискорення"; }
    const wchar_t* Subtitle() const override { return L"3 рівні оптимізації, Game Mode і пошук втрат FPS"; }
    const wchar_t* IconName() const override { return L"acceleration"; }

    void OnEnter() override {
        if (app::scanRequested) { app::scanRequested = false; findings_ = Monitor::I().FindLosses(); }
    }

    void Draw(Graphics& g, const RectF& a) override {
        Begin();
        const Level levels[3] = {Level::Easy, Level::Medium, Level::High};
        const wchar_t* tags[3] = {L"Безпечно для всіх", L"Легкий + додаткові", L"Максимум · потрібен адмін"};
        float cw = (a.Width - 32) / 3;
        for (int i = 0; i < 3; ++i) {
            RectF c(a.X + i * (cw + 16), a.Y, cw, 250);
            ui::Card(g, c, Optimizer::LevelName(levels[i]));
            ui::Text(g, tags[i], RectF(c.X + 18, c.Y + 40, c.Width - 36, 20), 12,
                     i == 2 ? ui::col::warn : ui::col::accent);
            std::wstring list;
            for (auto& s : Optimizer::Describe(levels[i])) list += L"•  " + s + L"\n";
            ui::Text(g, list, RectF(c.X + 18, c.Y + 68, c.Width - 36, 120), 13, ui::col::text, false,
                     StringAlignmentNear, StringAlignmentNear, true);
            Btn(g, RectF(c.X + 18, c.Y + c.Height - 56, c.Width - 36, 40), L"Застосувати", 10 + i, true, !Busy());
        }

        std::wstring st;
        { std::lock_guard<std::mutex> lk(mu_); st = Busy() ? L"Виконую… зачекайте кілька секунд." : status_; }
        ui::Text(g, st, RectF(a.X, a.Y + 256, a.Width, 22), 13, ui::col::muted);

        RectF gm(a.X, a.Y + 284, a.Width, 88);
        ui::Card(g, gm, L"Game Mode");
        ui::Text(g, GameMode::Status(), RectF(gm.X + 18, gm.Y + 46, gm.Width - 230, 30), 13, ui::col::muted);
        Btn(g, RectF(gm.X + gm.Width - 200, gm.Y + 22, 182, 44), GameMode::IsOn() ? L"Вимкнути" : L"Увімкнути", 20,
            !GameMode::IsOn());

        RectF fl(a.X, a.Y + 384, a.Width, 192);
        ui::Card(g, fl, L"Find FPS Losses");
        Btn(g, RectF(fl.X + fl.Width - 392, fl.Y + 8, 190, 34), L"Знайти втрати FPS", 30, true);
        Btn(g, RectF(fl.X + fl.Width - 194, fl.Y + 8, 176, 34), L"Відкотити зміни", 31, false, !Busy());
        if (findings_.empty())
            ui::Text(g, L"Натисніть «Знайти втрати FPS» — я перевірю схему живлення, фонові програми та пам'ять.",
                     RectF(fl.X + 18, fl.Y + 58, fl.Width - 36, 40), 13, ui::col::muted, false,
                     StringAlignmentNear, StringAlignmentNear, true);
        for (size_t i = 0; i < findings_.size() && i < 5; ++i) {
            float y = fl.Y + 54 + i * 26;
            ARGB c = findings_[i].severity == 2 ? ui::col::danger : (findings_[i].severity == 1 ? ui::col::warn : ui::col::good);
            ui::Fill(g, RectF(fl.X + 20, y + 8, 10, 10), 5, c);
            ui::Text(g, findings_[i].text, RectF(fl.X + 40, y, fl.Width - 58, 26), 13, ui::col::text, false,
                     StringAlignmentNear, StringAlignmentCenter);
        }
    }

    void OnButton(int id) override {
        if (id >= 10 && id <= 12) {
            Level lv = (Level)(id - 10);
            RunAsync([this, lv]() {
                OptResult r = Optimizer::Apply(lv);
                std::wstring s = std::wstring(Optimizer::LevelName(lv)) + L" рівень. " + (r.lines.empty() ? L"" : r.lines.front());
                if (r.lines.size() > 1) s += L"  ·  " + r.lines[1];
                std::lock_guard<std::mutex> lk(mu_);
                status_ = s;
            });
        } else if (id == 20) {
            if (GameMode::IsOn()) GameMode::Disable(); else GameMode::Enable();
        } else if (id == 30) {
            findings_ = Monitor::I().FindLosses();
        } else if (id == 31) {
            if (MessageBoxW(nullptr, L"Повернути налаштування, збережені в резервній копії?", L"Boosterik",
                            MB_YESNO | MB_ICONQUESTION) != IDYES) return;
            RunAsync([this]() {
                Restore::Result r = Restore::Run();
                std::lock_guard<std::mutex> lk(mu_);
                status_ = r.lines.empty() ? L"Готово" : r.lines.front();
            });
        }
    }
};
}

std::unique_ptr<Tab> MakeAccelerationTab() { return std::make_unique<AccelerationTab>(); }
