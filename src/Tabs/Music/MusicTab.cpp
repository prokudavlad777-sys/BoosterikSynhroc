#include "../Tabs.h"
#include "../../Core/Util.h"
#include <mmsystem.h>
#include <shellapi.h>
#include <algorithm>
#include <cstdio>
#include <cwchar>

using namespace Gdiplus;

static void Mci(const std::wstring& cmd) { mciSendStringW(cmd.c_str(), nullptr, 0, nullptr); }
void ShutdownMusic() { Mci(L"close bm"); }

namespace {
class MusicTab : public Tab {
    std::vector<std::wstring> files_;
    int idx_ = -1, scroll_ = 0;
    bool playing_ = false, paused_ = false;
    float volume_ = 0.7f;
    std::wstring status_ = L"Покладіть .mp3 / .wav / .wma у папку custom\\Music";

    static std::wstring Dir() { return util::Path(L"custom\\Music"); }
    static std::wstring NameOf(const std::wstring& p) {
        size_t s = p.find_last_of(L"\\/"), d = p.find_last_of(L'.');
        return p.substr(s + 1, (d == std::wstring::npos || d < s) ? std::wstring::npos : d - s - 1);
    }

    void Scan() {
        files_.clear();
        util::EnsureDir(util::Path(L"custom"));
        util::EnsureDir(Dir());
        for (const wchar_t* ext : {L"*.mp3", L"*.wav", L"*.wma"}) {
            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW((Dir() + L"\\" + ext).c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) continue;
            do { files_.push_back(Dir() + L"\\" + fd.cFileName); } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
        std::sort(files_.begin(), files_.end());
    }
    void SetVolume() {
        wchar_t b[64];
        swprintf(b, 64, L"setaudio bm volume to %d", (int)(volume_ * 1000));
        Mci(b);
    }
    void Play(int i) {
        if (i < 0 || i >= (int)files_.size()) return;
        Mci(L"close bm");
        MCIERROR err = mciSendStringW((L"open \"" + files_[i] + L"\" alias bm").c_str(), nullptr, 0, nullptr);
        if (err) { status_ = L"Не вдалося відкрити файл"; playing_ = false; return; }
        SetVolume();
        Mci(L"play bm");
        idx_ = i; playing_ = true; paused_ = false;
        status_ = L"Грає";
    }
    void Next(int step) {
        if (files_.empty()) return;
        int n = (int)files_.size();
        Play(((idx_ < 0 ? 0 : idx_) + step + n) % n);
    }
public:
    MusicTab() { Scan(); }
    const wchar_t* Title() const override { return L"Музика"; }
    const wchar_t* Subtitle() const override { return L"Локальні треки та Spotify"; }
    const wchar_t* IconName() const override { return L"music"; }

    void OnTick() override {
        if (!playing_ || paused_) return;
        wchar_t mode[64] = {};
        mciSendStringW(L"status bm mode", mode, 64, nullptr);
        if (!wcscmp(mode, L"stopped")) Next(1);       // track finished -> next one
    }

    void Draw(Graphics& g, const RectF& a) override {
        Begin();
        float lw = a.Width * 0.58f;
        RectF pl(a.X, a.Y, lw, 470), ct(a.X + lw + 16, a.Y, a.Width - lw - 16, 470);
        ui::Card(g, pl, L"Плейлист (custom\\Music)");
        const int rows = 10;
        if (files_.empty())
            ui::Text(g, L"Тут поки порожньо. Додайте музику в папку і натисніть «Оновити список».",
                     RectF(pl.X + 18, pl.Y + 56, pl.Width - 36, 60), 13, ui::col::muted, false, StringAlignmentNear, StringAlignmentNear, true);
        for (int r = 0; r < rows; ++r) {
            int i = scroll_ + r;
            if (i >= (int)files_.size()) break;
            RectF row(pl.X + 8, pl.Y + 46 + r * 36, pl.Width - 16, 34);
            Area(row, 100 + i);
            bool cur = i == idx_;
            if (cur) { ui::Fill(g, row, 8, ui::col::card2); ui::Stroke(g, row, 8, ui::col::accent); }
            else if (Hover(row)) ui::Fill(g, row, 8, ui::col::card2);
            ui::Text(g, (cur && playing_ ? L"▶  " : L"    ") + NameOf(files_[i]), RectF(row.X + 10, row.Y, row.Width - 20, row.Height), 14,
                     cur ? ui::col::accent : ui::col::text, false, StringAlignmentNear, StringAlignmentCenter);
        }
        Btn(g, RectF(pl.X + 18, pl.Y + pl.Height - 52, 90, 36), L"Вгору", 20, false, scroll_ > 0);
        Btn(g, RectF(pl.X + 118, pl.Y + pl.Height - 52, 90, 36), L"Вниз", 21, false, scroll_ + rows < (int)files_.size());
        wchar_t cnt[48];
        swprintf(cnt, 48, L"Треків: %d", (int)files_.size());
        ui::Text(g, cnt, RectF(pl.X + pl.Width - 150, pl.Y + pl.Height - 52, 132, 36), 13, ui::col::muted, false, StringAlignmentFar, StringAlignmentCenter);

        ui::Card(g, ct, L"Керування");
        ui::Text(g, idx_ >= 0 ? NameOf(files_[idx_]) : L"Нічого не грає", RectF(ct.X + 18, ct.Y + 46, ct.Width - 36, 44), 15,
                 ui::col::text, true, StringAlignmentNear, StringAlignmentNear, true);
        ui::Text(g, status_, RectF(ct.X + 18, ct.Y + 92, ct.Width - 36, 36), 12, ui::col::muted, false, StringAlignmentNear, StringAlignmentNear, true);
        float bw = (ct.Width - 36 - 10) / 2;
        Btn(g, RectF(ct.X + 18, ct.Y + 134, bw, 40), L"Назад", 1, false, !files_.empty());
        Btn(g, RectF(ct.X + 28 + bw, ct.Y + 134, bw, 40), L"Далі", 2, false, !files_.empty());
        Btn(g, RectF(ct.X + 18, ct.Y + 182, bw, 40), playing_ && !paused_ ? L"Пауза" : L"Грати", 3, true, !files_.empty());
        Btn(g, RectF(ct.X + 28 + bw, ct.Y + 182, bw, 40), L"Стоп", 4, false, playing_);
        wchar_t v[48];
        swprintf(v, 48, L"Гучність: %d%%", (int)(volume_ * 100));
        ui::Text(g, v, RectF(ct.X + 18, ct.Y + 236, ct.Width - 36, 22), 13, ui::col::muted);
        Slide(g, RectF(ct.X + 22, ct.Y + 262, ct.Width - 44, 24), 30, volume_);
        Btn(g, RectF(ct.X + 18, ct.Y + 306, ct.Width - 36, 40), L"Оновити список", 5, false);
        Btn(g, RectF(ct.X + 18, ct.Y + 354, ct.Width - 36, 40), L"Відкрити папку Music", 6, false);
        Btn(g, RectF(ct.X + 18, ct.Y + 410, ct.Width - 36, 44), L"Відкрити Spotify", 7, true);
    }

    void OnButton(int id) override {
        if (id >= 100) { Play(id - 100); return; }
        switch (id) {
        case 1: Next(-1); break;
        case 2: Next(1); break;
        case 3:
            if (!playing_) Play(idx_ < 0 ? 0 : idx_);
            else if (paused_) { Mci(L"resume bm"); paused_ = false; status_ = L"Грає"; }
            else { Mci(L"pause bm"); paused_ = true; status_ = L"Пауза"; }
            break;
        case 4: Mci(L"stop bm"); Mci(L"close bm"); playing_ = paused_ = false; status_ = L"Зупинено"; break;
        case 5: Scan(); scroll_ = 0; idx_ = -1; playing_ = false; Mci(L"close bm"); status_ = L"Список оновлено"; break;
        case 6: ShellExecuteW(nullptr, L"open", Dir().c_str(), nullptr, nullptr, SW_SHOWNORMAL); break;
        case 7:
            if ((INT_PTR)ShellExecuteW(nullptr, L"open", L"spotify:", nullptr, nullptr, SW_SHOWNORMAL) <= 32)
                ShellExecuteW(nullptr, L"open", L"https://open.spotify.com", nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case 20: scroll_ = std::max(0, scroll_ - 5); break;
        case 21: scroll_ += 5; break;
        }
    }
    void OnSlider(int, float v) override { volume_ = v; if (playing_) SetVolume(); }
};
}

std::unique_ptr<Tab> MakeMusicTab() { return std::make_unique<MusicTab>(); }
