#pragma once
#include "../UI/Tab.h"
#include <memory>

std::unique_ptr<Tab> MakeHomeTab();
std::unique_ptr<Tab> MakeAccelerationTab();
std::unique_ptr<Tab> MakeCleaningTab();
std::unique_ptr<Tab> MakeProgramsTab();
std::unique_ptr<Tab> MakeMusicTab();
std::unique_ptr<Tab> MakeCrosshairsTab();
std::unique_ptr<Tab> MakeColorsTab();
std::unique_ptr<Tab> MakeDriversTab();
std::unique_ptr<Tab> MakeSettingsTab();

// Cleanup hooks called when the window closes
void ShutdownMusic();
void ShutdownCrosshairs();
void ShutdownColors();
