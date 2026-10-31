#include "../Tabs.h"
#include "../../Core/Util.h"
#include "../../Optimizers/Cleaner.h"
#include "../../Permissions/Permissions.h"

using namespace Gdiplus;

namespace {
class CleaningTab : public Tab {
    std::vector<CleanCategory> cats_ = Cleaner::Categories();
    std::wstring status_ = L"Натисніть «Сканувати», щоб побачити, що можна очистити.";
public:
    const wchar_t* Title() const override { return L"Очищення"; }
    const wchar_t* Subtitle() const override { return L"Тимчасові файли, дампи, кеш і кошик"; }
    const wchar_t* IconName() const override { return L"cleaning"; }

    void Draw(Graphics& g, const RectF& a) override {
        Begin();
        std::vector<CleanCategory> cats;
        std::wstring st;
        { std::lock_guard<std::mutex> lk(mu_); cats = cats_; st = status_; }
        RectF c(a.X, a.Y, a.Width, 380);
        ui::Card(g, c, L"Що очищати");
        unsigned long long total = 0;
        for (size_t i = 0; i < cats.size(); ++i) {
            float y = c.Y + 50 + i * 64;
            Check(g, RectF(c.X + 18, y, c.Width - 220, 28), cats[i].title, 100 + (int)i, cats[i].selected);
            std::wstring sub = cats[i].path.empty() ? L"Системний кошик" : cats[i].path;
            if (!cats[i].note.empty()) sub = cats[i].note;
            ui::Text(g, sub, RectF(c.X + 50, y + 28, c.Width - 260, 20), 12, ui::col::muted);
            ui::Text(g, cats[i].scanned ? util::FormatBytes(cats[i].size) : L"—",
                     RectF(c.X + c.Width - 190, y, 172, 28), 16, ui::col::accent, true, StringAlignmentFar,
                     StringAlignmentCenter);
            if (cats[i].selected) total += cats[i].size;
            if (i + 1 < cats.size()) ui::Fill(g, RectF(c.X + 18, y + 56, c.Width - 36, 1), 0, ui::col::line);
        }
        Btn(g, RectF(a.X, a.Y + 396, 190, 44), L"Сканувати", 1, false, !Busy());
        Btn(g, RectF(a.X + 202, a.Y + 396, 230, 44), L"Очистити вибране", 2, true, !Busy());
        ui::Text(g, L"Вибрано: " + util::FormatBytes(total), RectF(a.X + 450, a.Y + 396, 300, 44), 14, ui::col::muted,
                 false, StringAlignmentNear, StringAlignmentCenter);
        ui::Text(g, Busy() ? L"Працюю…" : st, RectF(a.X, a.Y + 452, a.Width, 48), 14, ui::col::text, false,
                 StringAlignmentNear, StringAlignmentNear, true);
        if (!Permissions::IsAdmin())
            ui::Text(g, L"Без прав адміністратора файли в Windows Temp можуть бути очищені частково.",
                     RectF(a.X, a.Y + 504, a.Width, 22), 12, ui::col::muted);
    }

    void OnButton(int id) override {
        if (id >= 100) {
            std::lock_guard<std::mutex> lk(mu_);
            size_t i = (size_t)(id - 100);
            if (i < cats_.size()) cats_[i].selected = !cats_[i].selected;
        } else if (id == 1) {
            RunAsync([this]() {
                std::vector<CleanCategory> copy;
                { std::lock_guard<std::mutex> lk(mu_); copy = cats_; }
                for (auto& c : copy) Cleaner::Scan(c);
                std::lock_guard<std::mutex> lk(mu_);
                for (size_t i = 0; i < copy.size() && i < cats_.size(); ++i) {
                    cats_[i].size = copy[i].size;
                    cats_[i].scanned = true;
                }
                status_ = L"Сканування завершено.";
            });
        } else if (id == 2) {
            RunAsync([this]() {
                std::vector<CleanCategory> copy;
                { std::lock_guard<std::mutex> lk(mu_); copy = cats_; }
                unsigned long long freed = 0;
                for (auto& c : copy) if (c.selected) freed += Cleaner::Clean(c);
                std::lock_guard<std::mutex> lk(mu_);
                for (size_t i = 0; i < copy.size() && i < cats_.size(); ++i)
                    if (cats_[i].selected) { cats_[i].size = copy[i].size; cats_[i].scanned = true; }
                status_ = L"Звільнено: " + util::FormatBytes(freed) + L". Файли, що зараз використовуються, пропущено.";
            });
        }
    }
};
}

std::unique_ptr<Tab> MakeCleaningTab() { return std::make_unique<CleaningTab>(); }
