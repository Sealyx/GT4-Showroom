# Closed-gameplay stability audit —1.9.2

The audit found and fixed a real handling-memory ownership defect. The previous
heavy-ramming reward substituted an ASI-owned vector for vehicle+0x384. VehFuncs
owns this pointer, uses an extended tail and frees it when the vehicle is removed.
The installed2.5.2 binary allocates/deletes0x128 bytes; upstream source confirms
the vehicle-destructor path and reads at+0xF7. A224-byte vector is incompatible.
This can fail with the showroom closed, after claiming a heavy reward.

The new reward never replaces or copies handling memory. It maintains physical
mass/turn-mass/buoyancy on the individual pooled vehicle, plus the heavy flag.
Deletion/recycling discards metadata. Reset/eviction restores only matching owned
physical targets. Proof ability still sets only car bullet/explosion bits.
32 records bound the registry. No shared handling or plugin allocation is freed.

Closed idle work is also reduced: native entity capture immediately forwards
when no private target/deferred pins exist; repeated catalogue scanning stops
after startup warmup until opening. The7 native hook targets remain unchanged.
Current installed ASI/INI and every other mod remain untouched.

Evidence limits: no fresh18Oct fault-address dump/report was available. Older
Windows reports from17Oct21:15 and16Oct22:29 identify AnimatedTextures.asi,
exception0x40000015 at module offset0x383A6. Those reports are a separate lead,
not proof about the user's latest crash. Full memory dumps were not uploaded.
Other native plugins can still conflict; this audit does not certify the whole
modded game as crash-free.

Validation:14 executable suites, separate-DLL render-chain checks, current
Win32 binary/installed VehFuncs ownership inspection,10,000 production-field
updates with mocked external deletion/replacement and pool recycling. No GTA
session occurred. See tests/evidence-v192 and TEST-RESULTS.md.

Retest: restart after updating; play with showroom closed before any reward,
then claim each ability, ram traffic/test protection, leave the reward so it can
despawn, open/close repeatedly, save/reload and start a new game. Compare a
session with this ASI temporarily disabled if crashes recur. Preserve the
matching log and fresh CrashInfo/Windows fault address; do not remove unrelated
mods on the assumption that the old AnimatedTextures reports explain this run.
