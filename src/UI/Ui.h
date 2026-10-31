#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
using std::min;
using std::max;
#include <gdiplus.h>
#include <string>

namespace ui {
using namespace Gdiplus;

namespace col {
constexpr ARGB bg      = 0xFF0B0C10;
constexpr ARGB side    = 0xFF101116;
constexpr ARGB card    = 0xFF16171D;
constexpr ARGB card2   = 0xFF1E2028;
constexpr ARGB line    = 0xFF2A2C36;
constexpr ARGB accent  = 0xFFCDFF4D;
constexpr ARGB accentD = 0xFF9BC22F;
constexpr ARGB text    = 0xFFF2F3F5;
constexpr ARGB muted   = 0xFF8C90A0;
constexpr ARGB danger  = 0xFFFF5C5C;
constexpr ARGB warn    = 0xFFFFC14D;
constexpr ARGB good    = 0xFF6BE07A;
}

ARGB WithAlpha(ARGB c, BYTE a);

void Text(Graphics& g, const std::wstring& s, const RectF& r, float px, ARGB color, bool bold = false,
          StringAlignment h = StringAlignmentNear, StringAlignment v = StringAlignmentNear, bool wrap = false);
void Fill(Graphics& g, const RectF& r, float radius, ARGB fill);
void Stroke(Graphics& g, const RectF& r, float radius, ARGB color, float width = 1.f);
void Bar(Graphics& g, const RectF& r, double frac01, ARGB color);
void Card(Graphics& g, const RectF& r, const std::wstring& title = L"");
Bitmap* Image(const std::wstring& relativePath);   // cached; relative to the exe folder
void Shutdown();                                   // free GDI+ objects before GdiplusShutdown
}
