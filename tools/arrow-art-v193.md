# Final arrow design1.9.3

The user refined the supplied GT4 crops with a symmetric arrowhead diagram.
Construct equal rear legs around the arrow's own centerline, then rigidly tilt
the entire shape30degrees toward the upper left. Do not force the rotated rear
points onto the same screen Y: that was a rejected draft. Native pixels use one
uniform scale for both axes; preserve this aspect in exported crops, too.

The yellow pointer has a darker left gold face and brighter right gold face.
The color boundary provides their separation. Do not draw a center seam stroke.
Keep the lighter/darker outer-edge shading and soft shadow. The input hotspot
remains the tip. Geometry is shared by the ASI and standalone preview.

The silver thumbnail marker was reduced to24x8 design pixels and lightened
(top~93%, lower edge~47% of near-white). Its smooth1.2s cosine pulse restarts on
car selection using pause-aware time. It is absent on the Paint page, including
the transition. These crops do not establish exact GT4 blink timing.

Current evidence: previews/arrows.gif,showroom.mp4,arrows-refined.png and
provenance.json. Final two-face rendering has no center line. Earlier draft
inspection images outside the release are superseded. No GTA launch occurred.
