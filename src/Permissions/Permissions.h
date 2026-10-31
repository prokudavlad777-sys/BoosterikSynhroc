#pragma once
namespace Permissions {
bool IsAdmin();
bool RelaunchAsAdmin();   // true if the elevated copy was started (caller should exit)
}
