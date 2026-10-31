#pragma once
#include <string>
namespace GameMode {
bool IsOn();
void Enable();
void Disable();
void Tick();                 // call every second; boosts the fullscreen foreground app
std::wstring Status();
}
