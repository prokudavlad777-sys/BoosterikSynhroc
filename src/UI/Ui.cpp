#include "Ui.h"
#include "../Core/Util.h"
#include <map>
#include <memory>

namespace ui {

ARGB WithAlpha(ARGB c, BYTE a) { return (c & 0x00FFFFFF) | ((ARGB)a << 24); }

static std::unique_ptr<FontFamily> g_family;
static std::map<int, std::unique_ptr<Font>> g_fonts;
static std::map<std::wstring, std::unique_ptr<Bitmap>> g_images;

static Font* GetFont(float px, bool bold) {
    if (!g_family) {
        g_family.reset(new FontFamily(L"Segoe UI"));
        if (!g_family->IsAvailable()) g_family.reset(FontFamily::GenericSansSerif()->Clone());
    }
    int key = (int)(px * 10) + (bold ? 100000 : 0);
    auto it = g_fonts.find(key);
    if (it != g_fonts.end()) return it->second.get();
    Font* f = new Font(g_family.get(), px, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
    g_fonts[key].reset(f);
    return f;
}

void Text(Graphics& g, const std::wstring& s, const RectF& r, float px, ARGB color, bool bold,
          StringAlignment h, StringAlignment v, bool wrap) {
    if (s.empty()) return;
    StringFormat fmt;
    fmt.SetAlignment(h);
    fmt.SetLineAlignment(v);
    fmt.SetTrimming(StringTrimmingEllipsisCharacter);
    if (!wrap) fmt.SetFormatFlags(StringFormatFlagsNoWrap);
    SolidBrush br{Color(color)};
    g.DrawString(s.c_str(), -1, GetFont(px, bold), r, &fmt, &br);
}

static void RoundPath(GraphicsPath& p, const RectF& r, float rad) {
    float d = min(rad * 2, min(r.Width, r.Height));
    if (d <= 0.5f) { p.AddRectangle(r); return; }
    p.AddArc(r.X, r.Y, d, d, 180, 90);
    p.AddArc(r.X + r.Width - d, r.Y, d, d, 270, 90);
    p.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0, 90);
    p.AddArc(r.X, r.Y + r.Height - d, d, d, 90, 90);
    p.CloseFigure();
}

void Fill(Graphics& g, const RectF& r, float radius, ARGB fill) {
    GraphicsPath p;
    RoundPath(p, r, radius);
    SolidBrush br{Color(fill)};
    g.FillPath(&br, &p);
}

void Stroke(Graphics& g, const RectF& r, float radius, ARGB color, float width) {
    GraphicsPath p;
    RoundPath(p, r, radius);
    Pen pen(Color(color), width);
    g.DrawPath(&pen, &p);
}

void Bar(Graphics& g, const RectF& r, double frac, ARGB color) {
    frac = frac < 0 ? 0 : (frac > 1 ? 1 : frac);
    Fill(g, r, r.Height / 2, col::card2);
    if (frac > 0.001) Fill(g, RectF(r.X, r.Y, max(r.Height, (float)(r.Width * frac)), r.Height), r.Height / 2, color);
}

void Card(Graphics& g, const RectF& r, const std::wstring& title) {
    Fill(g, r, 14, col::card);
    Stroke(g, r, 14, col::line);
    if (!title.empty()) Text(g, title, RectF(r.X + 18, r.Y + 12, r.Width - 36, 24), 15, col::text, true);
}

Bitmap* Image(const std::wstring& rel) {
    auto it = g_images.find(rel);
    if (it != g_images.end()) return it->second.get();
    std::unique_ptr<Bitmap> bmp(Bitmap::FromFile(util::Path(rel).c_str()));
    if (bmp && bmp->GetLastStatus() != Ok) bmp.reset();
    Bitmap* raw = bmp.get();
    g_images[rel] = std::move(bmp);
    return raw;
}

void Shutdown() {
    g_images.clear();
    g_fonts.clear();
    g_family.reset();
}
}
