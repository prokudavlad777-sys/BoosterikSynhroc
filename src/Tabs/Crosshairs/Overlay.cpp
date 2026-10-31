#include "Overlay.h"

using namespace Gdiplus;

namespace Crosshair {

const ARGB kColors[5] = {0xFFCDFF4D, 0xFFFF3B3B, 0xFFFFFFFF, 0xFF2EE6FF, 0xFFFF4DDF};
static const int N = 256;      // overlay window size in pixels

void Render(Graphics& g, float cx, float cy, const CrosshairSettings& s, Bitmap* custom) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    ARGB c = kColors[s.colorIdx % 5];
    if (s.style >= 100) {
        if (custom) {
            float d = s.size * 2.5f;
            g.DrawImage(custom, RectF(cx - d / 2, cy - d / 2, d, d));
        }
        return;
    }
    Pen outline(Color(190, 0, 0, 0), s.thickness + 2);
    Pen pen(Color(c), s.thickness);
    outline.SetStartCap(LineCapFlat); pen.SetStartCap(LineCapFlat);
    bool cross = s.style == 0 || s.style == 3;
    bool dot = s.style == 1 || s.style == 3;
    if (cross) {
        float a = s.gap, b = s.gap + s.size;
        PointF segs[4][2] = {{{cx - b, cy}, {cx - a, cy}}, {{cx + a, cy}, {cx + b, cy}},
                             {{cx, cy - b}, {cx, cy - a}}, {{cx, cy + a}, {cx, cy + b}}};
        for (auto& sg : segs) g.DrawLine(&outline, sg[0], sg[1]);
        for (auto& sg : segs) g.DrawLine(&pen, sg[0], sg[1]);
    }
    if (dot) {
        float r = s.thickness + 1;
        SolidBrush bo(Color(190, 0, 0, 0)), bc{Color(c)};
        g.FillEllipse(&bo, cx - r - 1, cy - r - 1, 2 * r + 2, 2 * r + 2);
        g.FillEllipse(&bc, cx - r, cy - r, 2 * r, 2 * r);
    }
    if (s.style == 2) {
        float r = s.size;
        g.DrawEllipse(&outline, cx - r, cy - r, 2 * r, 2 * r);
        g.DrawEllipse(&pen, cx - r, cy - r, 2 * r, 2 * r);
    }
}

static LRESULT CALLBACK OverlayProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_NCHITTEST) return HTTRANSPARENT;
    return DefWindowProcW(h, m, w, l);
}

void Overlay::Show(const CrosshairSettings& s, Bitmap* custom) {
    if (!hwnd_) {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = OverlayProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"BoosterikCrosshair";
        RegisterClassExW(&wc);
        hwnd_ = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                L"BoosterikCrosshair", L"", WS_POPUP, 0, 0, N, N, nullptr, nullptr, wc.hInstance, nullptr);
        if (!hwnd_) return;
    }
    Bitmap bmp(N, N, PixelFormat32bppPARGB);
    {
        Graphics g(&bmp);
        g.Clear(Color(0, 0, 0, 0));
        Render(g, N / 2.f, N / 2.f, s, custom);
    }
    HBITMAP hbm = nullptr;
    bmp.GetHBITMAP(Color(0, 0, 0, 0), &hbm);
    if (!hbm) return;
    HDC scr = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(scr);
    HGDIOBJ old = SelectObject(mem, hbm);
    POINT pos{(GetSystemMetrics(SM_CXSCREEN) - N) / 2, (GetSystemMetrics(SM_CYSCREEN) - N) / 2};
    SIZE size{N, N};
    POINT src{0, 0};
    BLENDFUNCTION bf{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(hwnd_, scr, &pos, &size, mem, &src, 0, &bf, ULW_ALPHA);
    SelectObject(mem, old);
    DeleteObject(hbm);
    DeleteDC(mem);
    ReleaseDC(nullptr, scr);
    SetWindowPos(hwnd_, HWND_TOPMOST, pos.x, pos.y, N, N, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    visible_ = true;
}

void Overlay::Hide() {
    if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
    visible_ = false;
}

void Overlay::Destroy() {
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    visible_ = false;
}
}
