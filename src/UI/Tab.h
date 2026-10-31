#pragma once
#include "Ui.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

class Tab {
public:
    virtual ~Tab() = default;
    virtual const wchar_t* Title() const = 0;
    virtual const wchar_t* Subtitle() const { return L""; }
    virtual const wchar_t* IconName() const = 0;                  // file stem in assets/icons/tabs
    virtual void Draw(Gdiplus::Graphics& g, const Gdiplus::RectF& area) = 0;
    virtual void OnEnter() {}
    virtual void OnTick() {}                                      // once per second

    void MouseDown(Gdiplus::PointF p);
    void MouseMove(Gdiplus::PointF p);
    void MouseUp(Gdiplus::PointF p);

protected:
    virtual void OnButton(int /*id*/) {}
    virtual void OnSlider(int /*id*/, float /*value01*/) {}

    void Begin();                                                 // call first in Draw(): clears widgets
    void Btn(Gdiplus::Graphics& g, const Gdiplus::RectF& r, const std::wstring& text, int id,
             bool primary = true, bool enabled = true);
    void Area(const Gdiplus::RectF& r, int id);                   // invisible clickable zone
    void Slide(Gdiplus::Graphics& g, const Gdiplus::RectF& r, int id, float value01);
    void Check(Gdiplus::Graphics& g, const Gdiplus::RectF& r, const std::wstring& text, int id, bool checked);
    bool Hover(const Gdiplus::RectF& r) const { return r.Contains(mouse_); }

    // Runs work on a background thread; repaints when finished. Returns false if already busy.
    bool RunAsync(std::function<void()> work);
    bool Busy() const { return busy_.load(); }

    Gdiplus::PointF mouse_{-1, -1};
    mutable std::mutex mu_;

private:
    struct Widget { Gdiplus::RectF r; int id; bool enabled; bool slider; };
    std::vector<Widget> widgets_;
    int pressed_ = -1, drag_ = -1;
    std::atomic<bool> busy_{false};
};
