<p align="center"><img src="assets/icons/logo.png" width="160" alt="Boosterik"></p>

# Boosterik × SynhroBoost — Gaming Toolkit

Target of first release: **v1.0.0**

## Tabs
| Icon | Tab |
|---|---|
| <img src="assets/icons/tabs/home.png" width="40"> | Головна |
| <img src="assets/icons/tabs/acceleration.png" width="40"> | Прискорення |
| <img src="assets/icons/tabs/cleaning.png" width="40"> | Очищення |
| <img src="assets/icons/tabs/programs.png" width="40"> | Програми |
| <img src="assets/icons/tabs/music.png" width="40"> | Музика |
| <img src="assets/icons/tabs/crosshairs.png" width="40"> | Приціли |
| <img src="assets/icons/tabs/colors.png" width="40"> | Кольори |
| <img src="assets/icons/tabs/drivers.png" width="40"> | Драйвери |
| <img src="assets/icons/tabs/settings.png" width="40"> | Налаштування |

## Features
3 optimization levels (Easy / Medium / High), Game Mode, Find FPS Losses, Custom Crosshairs, Color Profiles, Spotify + Local Music, Backup & Restore, GitHub Actions.

## Що вже працює
- **Прискорення:** 3 рівні (Легкий / Середній / Високий), Game Mode, Find FPS Losses, відкат змін.
- **Очищення:** тимчасові файли, дампи, кеш шейдерів, кошик.
- **Програми:** список за ЦП/пам'яттю, завершення, пріоритет.
- **Музика:** локальні mp3/wav/wma + кнопка Spotify.
- **Приціли:** накладка по центру екрана, власні PNG.
- **Кольори:** експозиція, гамма, тіні, контраст, насиченість + профілі.
- **Драйвери:** версія драйвера GPU і посилання на офіційні сайти.
- **Налаштування:** адміністратор, резервна копія / відновлення, журнал.

Усі зміни системи зберігаються в `configs/backup.ini` перед першою оптимізацією й відкочуються кнопкою «Відкотити зміни».

## Build (Windows)
```
cmake -S . -B build -A x64
cmake --build build --config Release
```

## Workflow
One GitHub Issue per feature; Milestones: Alpha → Beta → Release.
