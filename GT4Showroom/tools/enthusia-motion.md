# ENTHUSIA motion reference

The local reference was sampled around 598.70–607.20 seconds; dense 30 Hz samples cover only three rapid transitions. Watching the entire recording was unnecessary. Implementation: **source/PreviewMotion.hpp**, applied to the main car/shadow camera in **source/Renderer.hpp**. Background, UI, sound and thumbnails retain the GT4 presentation.

| Relative time | Reference time | Measured presentation |
| --- | --- | --- |
| 0–1.46s | 598.70–600.16s | Small front three-quarter hold, slow yaw drift |
| 1.46–1.97s | 600.16–600.67s | Rapid continuous turn through head-on/side to rear, close zoom |
| 1.97–4.00s | 600.67–602.70s | Close rear hold with small drift |
| 4.00–4.47s | 602.70–603.17s | Rapid continuing orbit through opposite side to elevated front |
| 4.47–6.47s | 603.17–605.17s | Elevated front hold, slow drift/zoom |
| 6.47–6.97s | 605.17–605.67s | Rapid orbit through rear to original front side |
| 6.97–8.50s | 605.67–607.20s | Front hold, easing back to initial framing |

Unwrapped yaw completes two turns and avoids choosing the wrong short path across a 360-degree boundary. Transitions use smooth easing, holds use small linear drift, elevation/zoom follow the shared keyframes. The normalized vehicle framing factor was checked visually against the reference screen occupancy. The stock SA Sultan differs from the reference Subaru in geometry and wheel design; pixel-identical imagery is not claimed.

**Limit:** this is a visually measured reconstruction. No original Konami animation/camera data was recovered. The reference changes vehicle before proving an indefinite idle loop, so the 8.5-second repeating cycle is our continuation. The side-by-side contact sheet uses matched short reference samples to make the result inspectable.
