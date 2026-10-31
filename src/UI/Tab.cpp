#include "Tab.h"
#include "../Core/App.h"
#include <thread>

using namespace Gdiplus;

void Tab::Begin() { widgets_.clear(); }

void Tab::Btn(Graphics& g, const RectF& r, const std::wstring& text, int id, bool primary, bool enabled) {
    widgets_.push_back({r, id, enabled, false});
    bool hot = enabled && Hover(r);
    ARGB fill, fg;
    if (!enabled)      { fill = ui::col::card2; fg = ui::col::muted; }
    else if (primary)  { fill = hot ? 0xFFE3FF8A : ui::col::accent; fg = 0xFF0B0C10; }
    else               { fill = hot ? 0xFF2A2D38 : ui::col::card2; fg = ui::col::text; }
    ui::Fill(g, r, 10, fill);
    if (!primary && enabled) ui::Stroke(g, r, 10, hot ? ui::col::accent : ui::col::line);
    ui::Text(g, text, RectF(r.X + 8, r.Y, r.Width - 16, r.Height), 14, fg, true, StringAlignmentCenter, StringAlignmentCenter);
}

void Tab::Area(const RectF& r, int id) { widgets_.push_back({r, id, true, false}); }

void Tab::Slide(Graphics& g, const RectF& r, int id, float v) {
    widgets_.push_back({r, id, true, true});
    v = v < 0 ? 0 : (v > 1 ? 1 : v);
    float cy = r.Y + r.Height / 2;
    ui::Fill(g, RectF(r.X, cy - 3, r.Width, 6), 3, ui::col::card2);
    ui::Fill(g, RectF(r.X, cy - 3, r.Width * v, 6), 3, ui::col::accent);
    float kx = r.X + r.Width * v;
    ui::Fill(g, RectF(kx - 9, cy - 9, 18, 18), 9, Hover(r) || drag_ == id ? 0xFFFFFFFF : ui::col::text);
}

void Tab::Check(Graphics& g, const RectF& r, const std::wstring& text, int id, bool checked) {
    widgets_.push_back({r, id, true, false});
    RectF box(r.X, r.Y + (r.Height - 22) / 2, 22, 22);
    ui::Fill(g, box, 6, checked ? ui::col::accent : ui::col::card2);
    ui::Stroke(g, box, 6, checked ? ui::col::accent : (Hover(r) ? ui::col::accent : ui::col::line));
    if (checked) {
        Pen pen(Color(0xFF0B0C10), 2.5f);
        g.DrawLine(&pen, box.X + 5, box.Y + 11, box.X + 9.5f, box.Y + 16);
        g.DrawLine(&pen, box.X + 9.5f, box.Y + 16, box.X + 17, box.Y + 6);
    }
    ui::Text(g, text, RectF(r.X + 32, r.Y, r.Width - 32, r.Height), 14, ui::col::text, false,
             StringAlignmentNear, StringAlignmentCenter);
}

void Tab::MouseDown(PointF p) {
    mouse_ = p;
    pressed_ = drag_ = -1;
    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        if (!it->enabled || !it->r.Contains(p)) continue;
        if (it->slider) { drag_ = it->id; MouseMove(p); }
        else pressed_ = it->id;
        return;
    }
}

void Tab::MouseMove(PointF p) {
    mouse_ = p;
    if (drag_ < 0) return;
    for (auto& w : widgets_) {
        if (w.slider && w.id == drag_) {
            float v = (p.X - w.r.X) / w.r.Width;
            OnSlider(drag_, v < 0 ? 0 : (v > 1 ? 1 : v));
            return;
        }
    }
}

void Tab::MouseUp(PointF p) {
    mouse_ = p;
    if (drag_ >= 0) { drag_ = -1; return; }
    int id = pressed_;
    pressed_ = -1;
    if (id < 0) return;
    for (auto& w : widgets_)
        if (!w.slider && w.id == id && w.enabled && w.r.Contains(p)) { OnButton(id); return; }
}

bool Tab::RunAsync(std::function<void()> work) {
    bool expected = false;
    if (!busy_.compare_exchange_strong(expected, true)) return false;
    std::thread([this, work]() {
        work();
        busy_ = false;
        if (app::requestRepaint) app::requestRepaint();
    }).detach();
    return true;
}
