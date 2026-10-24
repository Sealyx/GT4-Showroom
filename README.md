# GT4 Showroom

<img src="previews/preview.gif"
     alt="Vehicle Selection"
     width="650">

Gran Turismo 4 inspired vehicle showroom and spawner for classic GTA San Andreas.

[![Video Demonstration](https://img.youtube.com/vi/VIDEO_ID/hqdefault.jpg)](https://www.youtube.com/watch?v=9CMLVn978-E)

## Features

- Animated 3D showroom with installed and add-on vehicles, full paint names and native livery texture previews.
- Persistent Favorites, country filtering, and name, class, top speed or model ID sorting from the right-click sidebar.
- Cinematic view with free orbit, zoom and optional automatic rotation.
- Model name/ID search, configurable exclusions and country badges.
- Keyboard, mouse and controller navigation, including an on-screen search keyboard.

## Requirements

- Classic **GTA San Andreas PC 1.0 US, 32-bit**, using a supported Hoodlum/Compact-compatible executable.
- Windows with working **Direct3D 9, DirectSound and Windows Imaging Component** support. The Windows system libraries used for PNG decoding are not separately bundled.
- A working **32-bit ASI loader**. Keep your existing working loader; do not install duplicate loaders for this mod.
- The **complete `install/` contents**, with the ASI, INI and `GT4Showroom` assets together. Mod Loader is recommended for managing this folder.
- For the PS2/controller route: **GInput SA 1.11**, configured correctly, plus a working controller/PS2 adapter and its compatible Windows driver. GInput is external and is not bundled. Keyboard/mouse use does not require it.
- For add-on vehicles: their correctly registered models, textures and any loader/limit-adjuster configuration they already require.
- **VehFuncs and IVF are optional**; install them when the particular vehicle requires their features. They are not included in this package.

**CLEO and MoonLoader are NOT REQUIRED**. 

No font installation or build toolchain is needed to play. Mobile, Definitive Edition and unverified executable editions are unsupported.

## Installation

1. Close GTA.
2. Copy the **contents** of `install/` into one Mod Loader folder, for example `modloader/GT4 Showroom/`.
3. Keep `GT4Showroom.asi`, `GT4Showroom.ini`, the `GT4Showroom/` asset folder together. Avoid leaving another active copy of this ASI elsewhere.
4. Start GTA, load ordinary gameplay and press Select on the controller or F1 on the keyboard while on foot.

## Controls

### Keyboard
| Input | Action |
| --- | --- |
| F4 | Open/close showroom ; configurable in the INI, on foot outside cutscenes |
| Left/Right or mouse wheel | Browse vehicles, colors or liveries |
| Enter / Space | Vehicle → color → livery when available → spawn |
| Backspace | Return one selection step; exit from the vehicle list |
| Right mouse / Menu key | Open the sorting, country, Favorites and Cinematic sidebar |
| Up / Down | Add / remove the selected favorite |
| F | Toggle Favorites / All Cars |
| Backtick | Search an ID or internal/display model name |
| GTSPAWN | Type outside search to reload settings |

### Controller
| Input | Action |
| --- | --- |
| Select | Open/close showroom |
| Select (while the showroom is open) | Open / close the sidebar |
| D-pad Left / Right or left stick | Browse vehicles, colors or liveries |
| D-pad or left stick | Select a sidebar row / filter / extra |
| D-pad Right or Cross; highlighting a category already reveals its pane | Enter the next sidebar pane |
| D-pad Left or Circle | Return to the previous sidebar pane |
| Confirm a vehicle / color / livery; spawn | Cross |
| Circle (leave Favorites or Cinematic first if active) | Return one selection step; leave the showroom |
| Square or D-pad Up | Add the selected vehicle to Favorites |
| D-pad Down | Remove the selected favorite |
| Triangle | Open Favorites / return to All Cars |
| L3 (press the left stick) | Open / close model search |
| D-pad or left stick | Move around the search keyboard |
| Cross | Type the highlighted search key |
| Square / DEL + Cross | Delete the last search character |
| Circle / L3 ? CLOSE + Cross | Close search without submitting |
| Cross | Pause / resume the Cinematic car rotation |
| R2 / L2; R1 / L1 | Zoom in / out in Cinematic |

Right mouse opens the sidebar and never substitutes for Backspace. The pointer eases toward the highlighted control after keyboard, wheel or controller navigation, including prize/search/sidebar controls. Mouse movement takes control back; leaving the mouse idle does not start a timed snap-back. The lower vehicle scrollbar can also be dragged.

## Configuration

- `GT4Showroom.ini`: opening key, volumes, zoom, exclusions, country assignments, logo switches, shadows, license-failure presentation and bonus options. Reopen the showroom, type `GTSPAWN` outside search or choose **Reload settings** in the sidebar to reload. Reloading is deferred while a license result is active.
- `[VanillaCountries]`: move an internal model name to another country, or remove it for a white badge. These assignments are independent of exclusions and `[Countries]`.
- `[Countries]`: add-on model IDs or names; an unassigned add-on has a white badge. Country keys such as `SouthKorea` retain their INI spelling while the menu displays **South Korea**.
- `[Logos] Car=0` and `Manufacturer=0`: independent switches, both off by default. Put `car.png` and/or `brand.png` in `logos/<internal model name>/`. Each missing/invalid image falls back to its own text. All 212 vanilla folders are provided, including non-car vehicles; `cheetah` contains the sample images.
- `[GT4Showroom] Shadows=1`: showroom shadows; the sidebar saves this switch. Cinematic freezes the light phase and shadow-caster orientation/scale on entry; the car and background continue animating.
- `[LicenseFail] Enabled=1`: replace supported driving-school failure results. Global Sound/MusicVolume/EffectsVolume settings apply.
- `[SecretUnlocks]`: `Model`, `EnableModkit`, `EnablePaintjob` and `EnableAbilities` configure REALBIG. Eligible vehicles must be visible and installed; tuning/paintjobs must exist for that vehicle. Each enabled reward gets one random heavy-ramming or bullet/explosion-proof ability.
- `GT4Showroom/vehicles.ini`: optional display metadata and paint names. `favorites.ini` stores personal Favorites and ordering; it is not replaced by a fresh personal Favorites file.

In Cinematic, Space or clicking the car toggles rotation; drag the car to orbit and use the wheel to zoom. Backspace returns. Enter `ENTHUSIA`, `GT4` or `REALBIG` in model search for the alternate preview, normal preview or configured bonus vehicle.
