LOGO ASSETS

Location: logos/<vehicle internal model name>/
Manufacturer image: brand.png
Car-name image: car.png

Recommended manufacturer size: 256 x 256 or 512 x 512 pixels.
Recommended car-name size: 512 x 128 or 512 x 256 pixels for wide wordmarks.
Maximum recommended canvas: 512 x 512 pixels for either file.
Keep the original aspect ratio; a square canvas is optional. Use transparent RGBA
PNG, leave a small clear border, and crop excessive empty margins. Do not stretch
art to fill a square. Smaller sources do not need upscaling. Oversized files are
resampled to a maximum 512-pixel edge at runtime; malformed/oversized inputs are
rejected safely. Each logo is independent and falls back to normal text when absent.

Enable Manufacturer or Car name individually from the Logos sidebar. Both are off
at startup. Folder names use internal model names, including non-car vehicles.
The Cheetah sample includes a 512 x 512 manufacturer and 512 x 171 car wordmark.
Result-screen words use real bundled fonts, not PNG text images.
