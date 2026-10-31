#include "Restore.h"
#include "../Backup/Backup.h"
#include "../Optimizers/Optimizer.h"
#include "../Optimizers/Power.h"
#include "../Permissions/Permissions.h"
#include "../Core/Util.h"
#include <cstdio>
#include <cwchar>
#include "../Logger/Logger.h"
#include <cstdlib>

namespace Restore {

Result Run() {
    Result r;
    if (!Backup::Exists()) {
        r.lines.push_back(L"Резервної копії ще немає. Спочатку застосуйте оптимізацію або створіть копію в Налаштуваннях.");
        return r;
    }
    const std::wstring f = Backup::FilePath();
    const bool admin = Permissions::IsAdmin();
    wchar_t buf[128];

    for (const RegTweak& t : Optimizer::Tweaks()) {
        GetPrivateProfileStringW(L"reg", t.id, L"", buf, 128, f.c_str());
        if (!buf[0]) { ++r.skipped; continue; }
        if (t.needsAdmin && !admin) {
            ++r.skipped;
            r.lines.push_back(std::wstring(L"Пропущено (потрібен адміністратор): ") + t.id);
            continue;
        }
        bool existed = buf[0] == L'1';
        const wchar_t* bar = wcschr(buf, L'|');
        DWORD v = bar ? (DWORD)wcstoul(bar + 1, nullptr, 10) : 0;
        bool ok = existed ? util::RegSetDword(t.root, t.path, t.name, v) : true;
        if (!existed) util::RegDelValue(t.root, t.path, t.name);     // it did not exist originally
        if (ok) ++r.restored; else ++r.failed;
    }

    GetPrivateProfileStringW(L"power", L"scheme", L"", buf, 128, f.c_str());
    if (buf[0]) { if (Power::SetActive(buf)) ++r.restored; else ++r.failed; }

    GetPrivateProfileStringW(L"spi", L"clientanim", L"", buf, 128, f.c_str());
    if (buf[0]) {
        bool anim = buf[0] == L'1';
        Optimizer::SetAnimations(anim);
        ++r.restored;
    }

    wchar_t sum[160];
    swprintf(sum, 160, L"Відновлено: %d, пропущено: %d, помилок: %d", r.restored, r.skipped, r.failed);
    r.lines.insert(r.lines.begin(), sum);
    Log::Write(sum);
    return r;
}
}
