#include "../Tabs.h"
#include "../../Hardware/Hardware.h"
#include <shellapi.h>

using namespace Gdiplus;

namespace {
class DriversTab : public Tab {
    static void Open(const wchar_t* what) { ShellExecuteW(nullptr, L"open", what, nullptr, nullptr, SW_SHOWNORMAL); }
public:
    const wchar_t* Title() const override { return L"Драйвери"; }
    const wchar_t* Subtitle() const override { return L"Версія драйвера відеокарти та офіційні джерела оновлень"; }
    const wchar_t* IconName() const override { return L"drivers"; }

    void Draw(Graphics& g, const RectF& a) override {
        Begin();
        const HwInfo& hw = Hardware::Get();
        RectF c(a.X, a.Y, a.Width, 190);
        ui::Card(g, c, L"Відеокарта");
        struct Row { const wchar_t* k; std::wstring v; };
        Row rows[] = {{L"Модель", hw.gpu},
                      {L"Виробник", hw.gpuVendor.empty() ? L"невідомо" : hw.gpuVendor},
                      {L"Версія драйвера", hw.gpuDriverVersion.empty() ? L"невідомо" : hw.gpuDriverVersion},
                      {L"Дата драйвера", hw.gpuDriverDate.empty() ? L"невідомо" : hw.gpuDriverDate}};
        for (int i = 0; i < 4; ++i) {
            float y = c.Y + 46 + i * 34;
            ui::Text(g, rows[i].k, RectF(c.X + 18, y, 170, 28), 14, ui::col::muted, false, StringAlignmentNear, StringAlignmentCenter);
            ui::Text(g, rows[i].v, RectF(c.X + 200, y, c.Width - 220, 28), 15, ui::col::text, i == 0, StringAlignmentNear, StringAlignmentCenter);
        }

        RectF d(a.X, a.Y + 206, a.Width, 250);
        ui::Card(g, d, L"Оновлення — лише з офіційних сайтів");
        float bw = (d.Width - 36 - 24) / 3;
        Btn(g, RectF(d.X + 18, d.Y + 50, bw, 44), L"NVIDIA", 1, hw.gpuVendor == L"NVIDIA");
        Btn(g, RectF(d.X + 30 + bw, d.Y + 50, bw, 44), L"AMD", 2, hw.gpuVendor == L"AMD");
        Btn(g, RectF(d.X + 42 + 2 * bw, d.Y + 50, bw, 44), L"Intel", 3, hw.gpuVendor == L"Intel");
        Btn(g, RectF(d.X + 18, d.Y + 106, (d.Width - 48) / 2, 44), L"Windows Update", 4, false);
        Btn(g, RectF(d.X + 30 + (d.Width - 48) / 2, d.Y + 106, (d.Width - 48) / 2, 44), L"Диспетчер пристроїв", 5, false);
        ui::Text(g, L"Завантажуйте оновлення лише з офіційних сайтів NVIDIA, AMD, Intel та виробників пристроїв. "
                    L"Boosterik нічого не завантажує і не встановлює драйвери сам — це безпечніше.",
                 RectF(d.X + 18, d.Y + 166, d.Width - 36, 70), 13, ui::col::muted, false, StringAlignmentNear, StringAlignmentNear, true);
    }

    void OnButton(int id) override {
        switch (id) {
        case 1: Open(L"https://www.nvidia.com/Download/index.aspx"); break;
        case 2: Open(L"https://www.amd.com/en/support/download/drivers.html"); break;
        case 3: Open(L"https://www.intel.com/content/www/us/en/download-center/home.html"); break;
        case 4: Open(L"ms-settings:windowsupdate"); break;
        case 5: Open(L"devmgmt.msc"); break;
        }
    }
};
}

std::unique_ptr<Tab> MakeDriversTab() { return std::make_unique<DriversTab>(); }
