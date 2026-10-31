#include "MainWindow.h"
#include "../Core/App.h"
#include "../Core/Util.h"
#include "../Monitoring/Monitor.h"
#include "../Permissions/Permissions.h"
#include "../Tabs/Tabs.h"
#include "../Tabs/GameMode/GameMode.h"
#include <dwmapi.h>

using namespace Gdiplus;

static const float kW = 1040, kH = 700, kSide = 220;
static const UINT kTimer = 1;

bool MainWindow::Create(HINSTANCE inst) {
    HDC dc = GetDC(nullptr);
    scale_ = GetDeviceCaps(dc, LOGPIXELSX) / 96.0f;
    ReleaseDC(nullptr, dc);

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = Proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"BoosterikMain";
    RegisterClassExW(&wc);

    tabs_.push_back(MakeHomeTab());
    tabs_.push_back(MakeAccelerationTab());
    tabs_.push_back(MakeCleaningTab());
    tabs_.push_back(MakeProgramsTab());
    tabs_.push_back(MakeMusicTab());
    tabs_.push_back(MakeCrosshairsTab());
    tabs_.push_back(MakeColorsTab());
    tabs_.push_back(MakeDriversTab());
    tabs_.push_back(MakeSettingsTab());

    RECT rc{0, 0, (LONG)(kW * scale_), (LONG)(kH * scale_)};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;   // fixed size
    AdjustWindowRect(&rc, style, FALSE);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2, y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    hwnd_ = CreateWindowExW(0, L"BoosterikMain", L"Boosterik × SynhroBoost — Gaming Toolkit", style, x, y, w, h,
                            nullptr, nullptr, inst, this);
    if (!hwnd_) return false;

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd_, 20, &dark, sizeof(dark));      // dark title bar (Windows 10 2004+/11)

    HWND hw = hwnd_;
    app::goTab = [this](int i) { SelectTab(i); };
    app::requestRepaint = [hw]() { PostMessageW(hw, WM_APP, 0, 0); };

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetTimer(hwnd_, kTimer, 1000, nullptr);
    tabs_[cur_]->OnEnter();
    return true;
}

LRESULT CALLBACK MainWindow::Proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    MainWindow* self = (MainWindow*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (m == WM_NCCREATE) {
        self = (MainWindow*)((CREATESTRUCTW*)l)->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self);
        if (self) self->hwnd_ = h;
    }
    return self ? self->Handle(m, w, l) : DefWindowProcW(h, m, w, l);
}

PointF MainWindow::ToLogical(LPARAM lp) const {
    return PointF((float)(short)LOWORD(lp) / scale_, (float)(short)HIWORD(lp) / scale_);
}

void MainWindow::SelectTab(int i) {
    if (i < 0 || i >= (int)tabs_.size()) return;
    cur_ = i;
    tabs_[cur_]->OnEnter();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

static RectF SideItem(int i) { return RectF(12, 104 + i * 56.f, kSide - 24, 50); }

LRESULT MainWindow::Handle(UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        Paint();
        return 0;
    case WM_TIMER:
        Monitor::I().Update();
        GameMode::Tick();
        tabs_[cur_]->OnTick();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    case WM_APP:
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN: {
        PointF p = ToLogical(l);
        if (p.X < kSide) {
            for (int i = 0; i < (int)tabs_.size(); ++i)
                if (SideItem(i).Contains(p)) { SelectTab(i); break; }
        } else {
            SetCapture(hwnd_);
            tabs_[cur_]->MouseDown(p);
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }
    case WM_MOUSEMOVE: {
        PointF p = ToLogical(l);
        int hs = -1;
        for (int i = 0; i < (int)tabs_.size(); ++i) if (SideItem(i).Contains(p)) hs = i;
        hoverSide_ = hs;
        tabs_[cur_]->MouseMove(p);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONUP: {
        if (GetCapture() == hwnd_) ReleaseCapture();
        tabs_[cur_]->MouseUp(ToLogical(l));
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd_, kTimer);
        GameMode::Disable();
        ShutdownMusic();
        ShutdownCrosshairs();
        ShutdownColors();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd_, m, w, l);
}

void MainWindow::Paint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd_, &ps);
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int cw = rc.right - rc.left, ch = rc.bottom - rc.top;

    HDC mem = CreateCompatibleDC(dc);
    HBITMAP bmp = CreateCompatibleBitmap(dc, cw, ch);
    HGDIOBJ old = SelectObject(mem, bmp);
    {
        Graphics g(mem);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        g.ScaleTransform(scale_, scale_);
        g.Clear(Color(ui::col::bg));

        // sidebar
        ui::Fill(g, RectF(0, 0, kSide, kH), 0, ui::col::side);
        if (Bitmap* logo = ui::Image(L"assets\\icons\\logo.png")) g.DrawImage(logo, RectF(18, 22, 54, 54));
        ui::Text(g, L"Boosterik", RectF(82, 24, 130, 28), 22, ui::col::text, true);
        ui::Text(g, L"× SynhroBoost", RectF(82, 54, 130, 20), 12, ui::col::accent);
        for (int i = 0; i < (int)tabs_.size(); ++i) {
            RectF r = SideItem(i);
            bool sel = i == cur_, hot = i == hoverSide_;
            if (sel || hot) ui::Fill(g, r, 12, sel ? ui::col::card2 : ui::col::card);
            if (sel) ui::Fill(g, RectF(r.X, r.Y + 12, 4, r.Height - 24), 2, ui::col::accent);
            if (Bitmap* ic = ui::Image(std::wstring(L"assets\\icons\\tabs\\") + tabs_[i]->IconName() + L".png"))
                g.DrawImage(ic, RectF(r.X + 12, r.Y + 7, 36, 36));
            ui::Text(g, tabs_[i]->Title(), RectF(r.X + 58, r.Y, r.Width - 62, r.Height), 15,
                     sel ? ui::col::text : ui::col::muted, sel, StringAlignmentNear, StringAlignmentCenter);
        }
        ui::Text(g, std::wstring(L"v") + L"1.0.0" + (Permissions::IsAdmin() ? L"  ·  адміністратор" : L""),
                 RectF(20, kH - 34, kSide - 30, 20), 12, ui::col::muted);

        // content
        RectF area(kSide + 28, 24, kW - kSide - 56, kH - 48);
        ui::Text(g, tabs_[cur_]->Title(), RectF(area.X, area.Y, area.Width, 38), 28, ui::col::text, true);
        ui::Text(g, tabs_[cur_]->Subtitle(), RectF(area.X, area.Y + 40, area.Width, 22), 14, ui::col::muted);
        tabs_[cur_]->Draw(g, RectF(area.X, area.Y + 76, area.Width, area.Height - 76));
    }
    BitBlt(dc, 0, 0, cw, ch, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd_, &ps);
}
