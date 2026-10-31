#pragma once
#include <functional>

#define BOOSTERIK_NAME L"Boosterik"

namespace app {
enum TabIndex { Home = 0, Acceleration, Cleaning, Programs, Music, Crosshairs, Colors, Drivers, Settings, TabCount };
inline std::function<void(int)> goTab;        // set by MainWindow
inline bool scanRequested = false;           // Home -> Acceleration: run "Find FPS Losses" on enter
inline std::function<void()> requestRepaint;  // set by MainWindow (thread-safe: posts a message)
}
