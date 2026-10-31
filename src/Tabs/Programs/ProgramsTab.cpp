#include "../Tabs.h"
#include "../../Core/Util.h"
#include "../../Monitoring/Monitor.h"
#include <cstdio>
#include <cwchar>

using namespace Gdiplus;

namespace {
class ProgramsTab : public Tab {
    std::vector<ProcInfo> rows_;
    DWORD sel_ = 0;
    bool sortCpu_ = false;
    std::wstring status_ = L"Оберіть програму зі списку.";

    static bool Protected(const std::wstring& name) {
        static const wchar_t* p[] = {L"system", L"registry", L"smss.exe", L"csrss.exe", L"wininit.exe", L"winlogon.exe",
                                     L"services.exe", L"lsass.exe", L"dwm.exe", L"explorer.exe", L"svchost.exe",
                                     L"fontdrvhost.exe", L"memcompression"};
        std::wstring n = util::ToLower(name);
        for (auto s : p) if (n == s) return true;
        return false;
    }
    const ProcInfo* Selected() const {
        for (auto& r : rows_) if (r.pid == sel_) return &r;
        return nullptr;
    }
public:
    const wchar_t* Title() const override { return L"Програми"; }
    const wchar_t* Subtitle() const override { return L"Що навантажує систему: закрийте зайве перед грою"; }
    const wchar_t* IconName() const override { return L"programs"; }

    void Draw(Graphics& g, const RectF& a) override {
        Begin();
        rows_.clear();
        for (auto& p : Monitor::I().Processes(sortCpu_))
            if (p.pid != 0 && p.name != L"System Idle Process") rows_.push_back(p);

        RectF c(a.X, a.Y, a.Width, a.Height - 100);
        ui::Card(g, c);
        float x0 = c.X + 18;
        ui::Text(g, L"Програма", RectF(x0, c.Y + 12, 300, 22), 13, ui::col::muted, true);
        ui::Text(g, L"PID", RectF(c.X + 330, c.Y + 12, 80, 22), 13, ui::col::muted, true);
        ui::Text(g, L"ЦП", RectF(c.X + 430, c.Y + 12, 80, 22), 13, sortCpu_ ? ui::col::accent : ui::col::muted, true);
        ui::Text(g, L"Пам'ять", RectF(c.X + 540, c.Y + 12, 120, 22), 13, sortCpu_ ? ui::col::muted : ui::col::accent, true);

        int maxRows = (int)((c.Height - 48) / 28);
        for (int i = 0; i < maxRows && i < (int)rows_.size(); ++i) {
            const ProcInfo& p = rows_[i];
            RectF r(c.X + 8, c.Y + 40 + i * 28, c.Width - 16, 26);
            Area(r, 1000 + i);
            bool sel = p.pid == sel_;
            if (sel) { ui::Fill(g, r, 8, ui::col::card2); ui::Stroke(g, r, 8, ui::col::accent); }
            else if (Hover(r)) ui::Fill(g, r, 8, ui::col::card2);
            wchar_t b[48];
            ui::Text(g, p.name, RectF(x0, r.Y, 300, r.Height), 13, ui::col::text, false, StringAlignmentNear, StringAlignmentCenter);
            swprintf(b, 48, L"%lu", (unsigned long)p.pid);
            ui::Text(g, b, RectF(c.X + 330, r.Y, 80, r.Height), 13, ui::col::muted, false, StringAlignmentNear, StringAlignmentCenter);
            swprintf(b, 48, L"%.1f%%", p.cpu);
            ui::Text(g, b, RectF(c.X + 430, r.Y, 90, r.Height), 13, p.cpu > 10 ? ui::col::warn : ui::col::text, false, StringAlignmentNear, StringAlignmentCenter);
            ui::Text(g, util::FormatBytes(p.memBytes), RectF(c.X + 540, r.Y, 140, r.Height), 13, ui::col::text, false, StringAlignmentNear, StringAlignmentCenter);
        }

        const ProcInfo* s = Selected();
        ui::Text(g, s ? L"Вибрано: " + s->name + L"   —   " + status_ : status_,
                 RectF(a.X, a.Y + a.Height - 92, a.Width, 22), 13, ui::col::muted);
        float y = a.Y + a.Height - 56, bw = (a.Width - 36) / 4;
        Btn(g, RectF(a.X, y, bw, 44), L"Завершити", 1, false, s != nullptr);
        Btn(g, RectF(a.X + bw + 12, y, bw, 44), L"Пріоритет: високий", 2, false, s != nullptr);
        Btn(g, RectF(a.X + 2 * (bw + 12), y, bw, 44), L"Пріоритет: звичайний", 3, false, s != nullptr);
        Btn(g, RectF(a.X + 3 * (bw + 12), y, bw, 44), sortCpu_ ? L"Сорт.: за ЦП" : L"Сорт.: за пам'яттю", 4, true);
    }

    void OnButton(int id) override {
        if (id >= 1000) {
            size_t i = (size_t)(id - 1000);
            if (i < rows_.size()) { sel_ = rows_[i].pid; status_ = L"готово до дії"; }
            return;
        }
        if (id == 4) { sortCpu_ = !sortCpu_; return; }
        const ProcInfo* s = Selected();
        if (!s) return;
        if (s->pid == GetCurrentProcessId() || Protected(s->name)) {
            status_ = L"системний процес захищено";
            return;
        }
        if (id == 1) {
            if (MessageBoxW(nullptr, (L"Завершити " + s->name + L"? Незбережені дані буде втрачено.").c_str(), L"Boosterik",
                            MB_YESNO | MB_ICONWARNING) != IDYES) return;
            HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, s->pid);
            bool ok = h && TerminateProcess(h, 1);
            if (h) CloseHandle(h);
            status_ = ok ? L"процес завершено" : L"не вдалося завершити (немає доступу)";
        } else {
            HANDLE h = OpenProcess(PROCESS_SET_INFORMATION, FALSE, s->pid);
            bool ok = h && SetPriorityClass(h, id == 2 ? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
            if (h) CloseHandle(h);
            status_ = ok ? L"пріоритет змінено" : L"не вдалося змінити пріоритет";
        }
    }
};
}

std::unique_ptr<Tab> MakeProgramsTab() { return std::make_unique<ProgramsTab>(); }
