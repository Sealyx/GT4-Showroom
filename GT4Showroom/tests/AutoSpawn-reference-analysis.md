# AutoSpawn reference analysis — 17 October 2026

Inspected the user-provided AutoSpawn.zip (AutoSpawn.lua, author HzanRsxa2959, script version 4.3.1). The archive and bundled MoonLoader/native libraries were read, never executed or installed. Original SHA-256: eae461ea9ba3b0db2f4f5e09a78c2d02eaa496aa66fbb889582eb9a4f454df2f.

## How its editable 3D preview works

The Lua script uses mimgui, hzanlib and MoonAdditions. AutoSpawn.lua lines 155–302 implement the preview. On a model change it requests/loads the selected model, creates one native GTA vehicle (around line 217), disables its collision and damage, freezes its velocities, and calls the native pre-render method. It projects a chosen screen point into the current world, updates the vehicle transform each frame, and compensates for camera movement. Bounds determine its initial distance. Sliders/toggles control rotation, rotation speed, zoom and white preview lighting. Settings are saved to INI. Selection changes and closing delete the preview vehicle.

This is a real editable GTA vehicle rendered through the engine, positioned in the current gameplay view. It does not create a remote independent studio or render a catalog of small offscreen models. Model-loading calls happen on selection changes, rather than for all vehicles every frame.

## Applied to GT4 Showroom 1.2.0

Keep cached geometry and change transforms, paint and light on the GPU. Reuse selected/nearby models; keep a bounded CPU/GPU cache on close so reopening can reuse it. Do not copy its screen-positioned world vehicle: the requested showroom still has its own studio and hides CJ. Native model streaming advances incrementally while our pause is held instead of forcing LoadAllModelsNow for each missing thumbnail. Only selected metadata refreshes on opening/selection.

The user's screenshot is reproduced by the old showroom renderer with ZFUNC=ALWAYS and SHADEMODE=FLAT inherited from a HUD draw. This lets far-side wheels/interior draw over bodywork and causes faceted studio bands. The new renderer sets owned depth/shading/texture/lighting state explicitly, and a clean-state versus contaminated-state regression now produces identical pixels. Restoring those states is checked separately. Native really-low-detail `_vlo` atomics are omitted, matching the close-up role that the engine's visibility callbacks normally choose.

No AutoSpawn source or bundled DLL is copied into the new mod; its source did not include an explicit reuse license. GT4 Showroom remains a single standalone ASI. AutoSpawn is a useful example for small MoonLoader tools with editable native entity previews; it is not evidence that Lua is inherently faster or that all MoonLoader library methods are portable.

## Verification limits

The reference code was analyzed, not run. The updated renderer was compiled and tested outside GTA, with 11 private installed-model fixtures and hardware/software/mixed vertex processing, state contamination/restoration, resolution reset and a 2x-MSAA host. Actual installed-model extraction, paused streaming, add-on compatibility and game FPS still require GTA testing.
