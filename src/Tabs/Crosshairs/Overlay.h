#pragma once
#include "../../UI/Ui.h"

struct CrosshairSettings {
    int style = 0;          // 0 cross, 1 dot, 2 circle, 3 cross + dot, 100+ = custom image
    int colorIdx = 0;
    float size = 14;        // arm length / radius
    float thickness = 2;
    float gap = 4;
};

namespace Crosshair {
extern const Gdiplus::ARGB kColors[5];
void Render(Gdiplus::Graphics& g, float cx, float cy, const CrosshairSettings& s, Gdiplus::Bitmap* custom);

class Overlay {
public:
    ~Overlay() { Destroy(); }
    void Show(const CrosshairSettings& s, Gdiplus::Bitmap* custom);   // creates/updates and shows
    void Hide();
    void Destroy();
    bool Visible() const { return visible_; }
private:
    HWND hwnd_ = nullptr;
    bool visible_ = false;
};
}
