# GT4 Showroom

A Gran Turismo 4 inspired vehicle showroom and spawner for classic GTA San Andreas.

## Features

- Animated 3D showroom with installed and add-on vehicles, full paint names and native livery texture previews.
- Persistent Favorites, country filtering, and name, class, top speed or model ID sorting from the right-click sidebar.
- Cinematic view with free orbit, zoom and optional automatic rotation.
- Model name/ID search, configurable exclusions and country badges.
- Keyboard, mouse and controller navigation, including an on-screen search keyboard.
- ENTHUSIA preview motion and configurable REALBIG bonus vehicles with tuning, paintjobs and optional abilities.

## Install

Requires **classic GTA San Andreas PC 1.0 US Hoodlum/Compact, 32-bit**, Windows, DirectX 9 and a working **32-bit ASI loader**. Mod Loader is recommended. Other executable editions, mobile and Definitive Edition are unsupported.

Close GTA. Copy the contents of `install` into one Mod Loader folder, such as `modloader/GT4 Showroom`. Keep `GT4Showroom.asi`, `GT4Showroom.ini` and the `GT4Showroom` asset folder together. With a loader that scans the game root, the same three items can be placed together there instead.

When updating, replace the ASI, keep the matching packaged assets and retain your customized INI, `vehicles.ini` and personal `favorites.ini`. The update ZIP includes an example INI to merge. Disable the mod folder while GTA is closed to uninstall.

CLEO, MoonLoader and Dear ImGui are not required. Add-on vehicles/tuning must already be registered by their own loader or limit adjuster. VehFuncs and IVF are optional dependencies of vehicles that use them. Liveries use the game's registered paintjob slots.

## Controls

| Input | Action |
| --- | --- |
| F1 | Open/close; configurable in the INI, on foot outside cutscenes |
| Left/Right or mouse wheel | Browse vehicles, colors or liveries |
| Enter / Space | Vehicle → color → livery when available → spawn |
| Backspace | Return one selection step; exit from the vehicle list |
| Right mouse / Menu key | Open the sorting, country, Favorites, Cinematic and Reload settings sidebar |
| Up / Down | Add / remove the selected favorite |
| F | Toggle Favorites / All Cars |
| Backtick | Search an ID or internal/display model name |
| Escape | Native pause |
| GTSPAWN | Type outside search to reload settings |

Click selection rectangles or drag the lower browse bar. The cursor returns to the selected rectangle only after keyboard, wheel or controller browsing changes the selection. It stays where you leave it while idle.

In Cinematic, Space or clicking the car toggles rotation; drag the car to orbit and use the wheel to zoom. Backspace returns. Enter `ENTHUSIA`, `GT4` or `REALBIG` in model search for the alternate preview, normal preview or configured bonus vehicle.

Controller: View/Select opens the showroom/sidebar; D-pad/left stick navigates; A confirms; B returns; X adds Favorites; Y opens Favorites; D-pad Down removes; left-stick click opens search. Right stick orbits and triggers zoom in Cinematic. Start/Options opens the native pause menu. Reload settings in the sidebar provides a controller-only alternative to typing GTSPAWN. PlayStation equivalents are Cross/Circle/Square/Triangle for A/B/X/Y.

## Languages and dependencies

Written in **C++17**, with **MinHook C**, Win32/GDI, Direct3D 9 and DirectSound. Built as a standalone Win32 ASI using LLVM-MinGW/Clang and PowerShell. Python helpers, Blender/DragonFF and FFmpeg support asset preparation and previews; players do not need them installed. Bundled Michroma and Liberation Sans fonts load privately and retain their license notices.

Logging hotfix: [GT4Showroom] EnableLog=0 is the default, also when absent in an older INI. Set EnableLog=1 and type GTSPAWN outside search to append diagnostics. Disable and reload to stop writes. Existing logs are preserved. Version2.2.0 passes the native build,19 regression suites and offline renderer checks; this new binary still needs an in-game/controller retest.
