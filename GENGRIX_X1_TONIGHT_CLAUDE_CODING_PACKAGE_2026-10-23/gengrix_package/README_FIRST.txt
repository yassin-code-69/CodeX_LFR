GENGRIX-X1 CLAUDE HANDOFF PACKAGE - 2026-10-23

CONTENTS
1. MASTER_ENGINEERING_HANDOFF.md: complete authoritative instructions, hardware lock, geometry, algorithm backlog, safety acceptance, test plan and public research references.
2. CLAUDE_START_MESSAGE.txt: paste this message into the existing Claude coding session, alongside the master Markdown.
3. CLAUDE_STAGED_FOLLOWUPS.txt: short follow-up prompts for separate Claude sessions or context-limit management.
4. REFERENCE_ONLY_V3_3_HANDOFF.zip: historical V3.3 source and independent audit from an earlier conversation. Do NOT overwrite the actual current Claude dev/v3.4-phase-a branch with this older reference.

HOW TO USE
- Preferred: Continue in the SAME Claude session/workspace where dev/v3.4-phase-a already exists.
- Attach MASTER_ENGINEERING_HANDOFF.md, then paste CLAUDE_START_MESSAGE.txt.
- Tell Claude to execute changes in the local workspace, not answer with a plan only.
- If context gets tight, use the staged follow-ups and prioritize producing a complete buildable checkpoint.
- Inspect build/test logs and source ZIP. Only upload/flash with SAFE startup defaults after independent checks.

ABSOLUTE CONSTRAINTS
- No hardware modifications, no parts added or changed, no rewiring.
- Keep all four original buttons and all pin mappings.
- No external EEPROM/encoders. Internal STM32 Flash emulation only.
- No push/merge to main without express permission.
- Unmeasured track performance remains UNTESTED.

IMPORTANT KNOWN DATA
- User reported length 190 mm, width 205 mm, sensor ground clearance 10 mm.
- Wheel diameter 35 mm, inner tire-to-tire gap 145 mm, axle-to-front-center sensor 160 mm.
- Wheel track width approx 175 mm inferred using photo-estimated ~30 mm tire tread width; NOT an exact measured value.
- Total vertical robot height unknown; 10 mm is sensor clearance, not robot height.
- STM32F103C8, fourteen IR sensors through MUX, 2x 25GA 12V 1000 rpm motors, BTS7960 software pin setup, 3S 1500mAh 30C battery, total 510-530g.
