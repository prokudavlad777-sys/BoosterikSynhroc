#include "Permissions.h"
#include "../Core/Util.h"
#include <shellapi.h>

namespace Permissions {
bool IsAdmin() {
    SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;
    PSID admins = nullptr;
    BOOL member = FALSE;
    if (AllocateAndInitializeSid(&nt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0, &admins)) {
        CheckTokenMembership(nullptr, admins, &member);
        FreeSid(admins);
    }
    return member != FALSE;
}

bool RelaunchAsAdmin() {
    wchar_t exe[MAX_PATH * 2] = {};
    GetModuleFileNameW(nullptr, exe, (DWORD)(sizeof(exe) / sizeof(exe[0])));
    HINSTANCE r = ShellExecuteW(nullptr, L"runas", exe, nullptr, nullptr, SW_SHOWNORMAL);
    return (INT_PTR)r > 32;
}
}
