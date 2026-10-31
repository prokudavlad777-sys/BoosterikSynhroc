#include "../Tabs.h"
#include "../../Backup/Backup.h"
#include "../../Core/Util.h"
#include "../../Logger/Logger.h"
#include "../../Permissions/Permissions.h"
#include "../../Restore/Restore.h"
#include <shellapi.h>

using namespace Gdiplus;

namespace {
class SettingsTab : public Tab {
    std::wstring status_;
public:
    const wchar_t* Title() const override { return L"Налаштування"; }
    const wchar_t* Subtitle() const override { return L"Права, резервні копії та журнал"; }
    const wchar_t* IconName() const override { return L"settings"; }

    void Draw(Graphics& g, const RectF& a) override {
        Begin();
        bool admin = Permissions::IsAdmin();
        RectF c(a.X, a.Y, a.Width, 150);
        ui::Card(g, c, L"Права доступу");
        ui::Fill(g, RectF(c.X + 20, c.Y + 54, 12, 12), 6, admin ? ui::col::good : ui::col::warn);
        ui::Text(g, admin ? L"Запущено від імені адміністратора" : L"Звичайний режим (рівень «Високий» і Windows Temp потребують адміністратора)",
                 RectF(c.X + 42, c.Y + 46, c.Width - 62, 28), 14, ui::col::text, false, StringAlignmentNear, StringAlignmentCenter);
        Btn(g, RectF(c.X + 18, c.Y + 90, 280, 40), L"Перезапустити як адміністратор", 1, false, !admin);

        RectF b(a.X, a.Y + 166, a.Width, 210);
        ui::Card(g, b, L"Резервна копія та відновлення");
        ui::Text(g, L"Остання копія: " + Backup::LastWriteText(), RectF(b.X + 18, b.Y + 44, b.Width - 36, 24), 14, ui::col::muted);
        float bw = (b.Width - 36 - 12) / 2;
        Btn(g, RectF(b.X + 18, b.Y + 78, bw, 44), L"Створити резервну копію", 2, true, !Busy());
        Btn(g, RectF(b.X + 30 + bw, b.Y + 78, bw, 44), L"Відновити з копії", 3, false, !Busy());
        std::wstring st;
        { std::lock_guard<std::mutex> lk(mu_); st = status_; }
        ui::Text(g, st.empty() ? L"Копія створюється автоматично перед першою оптимізацією та містить схему живлення, "
                                 L"ключі реєстру й параметри анімацій." : st,
                 RectF(b.X + 18, b.Y + 134, b.Width - 36, 64), 13, ui::col::muted, false, StringAlignmentNear, StringAlignmentNear, true);

        RectF f(a.X, a.Y + 392, a.Width, 110);
        ui::Card(g, f, L"Файли");
        Btn(g, RectF(f.X + 18, f.Y + 50, 220, 40), L"Відкрити журнал", 4, false);
        Btn(g, RectF(f.X + 250, f.Y + 50, 220, 40), L"Папка програми", 5, false);
        ui::Text(g, L"Boosterik × SynhroBoost · v1.0.0", RectF(f.X + 490, f.Y + 50, f.Width - 508, 40), 13, ui::col::muted,
                 false, StringAlignmentFar, StringAlignmentCenter);
    }

    void OnButton(int id) override {
        if (id == 1) {
            if (Permissions::RelaunchAsAdmin()) PostMessageW(GetActiveWindow(), WM_CLOSE, 0, 0);
        } else if (id == 2) {
            bool ok = Backup::Create();
            std::lock_guard<std::mutex> lk(mu_);
            status_ = ok ? L"Резервну копію створено." : L"Не вдалося записати резервну копію (немає прав на запис у папку програми?).";
        } else if (id == 3) {
            if (MessageBoxW(nullptr, L"Відновити систему зі збереженої резервної копії?", L"Boosterik",
                            MB_YESNO | MB_ICONQUESTION) != IDYES) return;
            RunAsync([this]() {
                Restore::Result r = Restore::Run();
                std::wstring s;
                for (size_t i = 0; i < r.lines.size() && i < 3; ++i) s += r.lines[i] + L"\n";
                std::lock_guard<std::mutex> lk(mu_);
                status_ = s;
            });
        } else if (id == 4) {
            ShellExecuteW(nullptr, L"open", Log::FilePath().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        } else if (id == 5) {
            ShellExecuteW(nullptr, L"open", util::ExeDir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }
};
}

std::unique_ptr<Tab> MakeSettingsTab() { return std::make_unique<SettingsTab>(); }
