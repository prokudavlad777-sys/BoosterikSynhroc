#pragma once
#include "Ui.h"
#include "Tab.h"
#include <memory>
#include <vector>

class MainWindow {
public:
    bool Create(HINSTANCE inst);
    HWND Handle() const { return hwnd_; }
private:
    static LRESULT CALLBACK Proc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Handle(UINT, WPARAM, LPARAM);
    void Paint();
    void SelectTab(int i);
    Gdiplus::PointF ToLogical(LPARAM lp) const;

    HWND hwnd_ = nullptr;
    float scale_ = 1.f;
    int cur_ = 0;
    int hoverSide_ = -1;
    std::vector<std::unique_ptr<Tab>> tabs_;
};
