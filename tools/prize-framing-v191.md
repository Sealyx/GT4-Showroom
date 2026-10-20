# Restored static REALBIG studio1.9.1

User correction2026-10-18: restore the old static studio, raise its ground floor
slightly and enlarge the car after1.9.0 looked too small. This supersedes the
earlier request for a mesh prize environment; do not bring that studio back
without another request. Ordinary showroom/native gameplay code is unchanged.

Reuse exact1.8.1 backdrop pixels; title band y0-.35 and its proportions stay
unchanged. Piecewise screen/reference Y knots are0/0,.35/.35,.66/.74,.91/.91,1/1.
Only lower curtain/floor bands are reframed. Center4:3 artwork is extended with
mirrored curtain/floor wings at wider native viewports. No square viewport or
vehicle/title stretching. The car and mirrored reflection are separate3D draws.

Camera distance7.4/zoom, eyeZ=.75+1.25/zoom, targetZ=.75, centerY=.52H.
Normalized size caps:span5.1,height1.6. NameY=.278H. Previous1.9.0 useddistance9.4,
centerY=.60H,span4.5,height1.5. Retain the gentle.88->1 reveal, not the old huge
intro zoom. Existing all-angle/cross-aspect framing regression covers long/tall
models. Runtime resource, tuning/ability and ordinary wheel paths are preserved.

Only fresh1.9.1 stills/movie/provenance establish the current presentation.
They use a standalone shared renderer, not a GTA gameplay test.
