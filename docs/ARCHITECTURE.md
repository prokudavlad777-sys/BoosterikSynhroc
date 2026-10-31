# Архітектура Boosterik

Win32 + GDI+ (без зовнішніх залежностей), C++20.

| Папка | Відповідальність |
|---|---|
| `src/Core` | Утиліти (`Util`), спільні типи (`App.h`) |
| `src/UI` | Ui-інструменти (малювання), `Tab` (база вкладки), `MainWindow` |
| `src/Tabs/*` | По одній вкладці (`*Tab.cpp`) + `GameMode` |
| `src/Optimizers` | Легкий/Середній/Високий рівні (`Optimizer`), `Power`, `Cleaner` |
| `src/Backup`, `src/Restore` | Збереження та відновлення початкового стану (`configs/backup.ini`) |
| `src/Monitoring` | Навантаження ЦП/пам'яті, процеси, Find FPS Losses |
| `src/Hardware` | ЦП, GPU, драйвер, ОС |
| `src/Permissions` | Перевірка/запит прав адміністратора |
| `src/Logger` | Журнал `configs/boosterik.log` |

## Як додати вкладку
1. Створіть `src/Tabs/<Name>/<Name>Tab.cpp`, клас успадковує `Tab`.
2. Додайте `Make<Name>Tab()` у `src/Tabs/Tabs.h` і в `MainWindow::Create`.
3. Покладіть іконку в `assets/icons/tabs/`.

## Безпека змін
- Усі зміни реєстру/схеми живлення описані в `Optimizer.cpp` (`Tweaks()`).
- Перед першою зміною `Backup::EnsureExists()` зберігає оригінальні значення.
- `Restore::Run()` повертає їх (значення, яких не існувало, видаляються).
