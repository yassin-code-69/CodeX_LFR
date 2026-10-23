# Development workflow

**`firmware/gengrix-x1-active` is the canonical active firmware branch.** Every new piece of work starts from it.

## Branches

| Branch | Role |
|---|---|
| `firmware/gengrix-x1-active` | current verified firmware (baseline V3.5.2, `905389c`) + essential tests/docs |
| `feature/<topic>` or `fix/<topic>` | short-lived work branches created from the active branch |
| `main` (`2bb762d`, V3.3) | untouched until the owner explicitly authorises a merge |
| `dev/v3.4-phase-a`, `dev/v3.5-performance`, `dev/v3.5.2-safety-audit` | historical checkpoints (V3.4.1, V3.5.1, V3.5.2 + handoff package); never rewritten |

## Making a change

1. **Checkpoint:** note the active branch commit; verified binaries stay in `CodeX_LFR_RELEASES/` (outside the repo).
2. **Branch:** `git switch -c feature/<topic> firmware/gengrix-x1-active`.
3. **Develop with a test:** reproduce a bug or describe new behaviour in a host test under `tests/` first; keep the change narrow.
4. **Verify before integrating:**
   - all host suites (README "Host tests"), also under UBSan if available: `CXX="g++ -fsanitize=undefined -fsanitize-trap=undefined"`;
   - STM32 builds RACE_LOCKED and RACE_UNLOCKED with `--warnings all` (0 warnings), note flash/RAM;
   - `git diff firmware/gengrix-x1-active -- LFR_PRO_V3` reviewed line by line.
5. **Integrate** only reviewed and verified changes (fast-forward or a normal merge into the active branch); update `CHANGELOG.md` and the affected docs in the same change.
6. **Physical results** are recorded separately (`TRACK_TEST_PLAN.md` log table); never describe host results as robot results.

## Fixed constraints (no change without an explicit hardware decision)

STM32F103C8T6 Blue Pill target and core 3.0.0 settings · all pin assignments · the four buttons (UP PB13, DOWN PB12, ENTER PB5, BACK/E-STOP PB3) and their functions · 14-channel MUX sensor configuration · BTS7960 drive scheme · 10 kHz PWM · internal-flash settings record (`Settings`, `Calibration`, `Store3`) · `LFR_ALLOW_RACE_START 0` as the committed default · CLASSIC as the boot turn mode. Experimental behaviour must stay behind an explicit switch and must not silently replace the working baseline.

## Never commit

Build outputs (`.bin/.hex/.elf/.o`), logs, release ZIP/RAR archives, generated `tests/previews/`, duplicate firmware trees, handoff/instruction packages (`.gitignore` covers these). Release packages and logs live in `CodeX_LFR_RELEASES/<version>_<commit>/` with SHA256 manifests.
