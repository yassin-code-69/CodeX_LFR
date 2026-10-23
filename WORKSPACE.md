# GENGRIX-X1 workspace map

## The one active firmware tree

**`LFR_PRO_V3/`** at the repository root is the only firmware that is built and flashed.
Open `LFR_PRO_V3/LFR_PRO_V3.ino` in Arduino IDE (board: STMicroelectronics STM32 3.0.0,
Generic STM32F1, part number BluePill F103C8). Host tests in `tests/` compile this same tree.

| Path | Status |
|---|---|
| `LFR_PRO_V3/` | **ACTIVE** firmware (14 files) |
| `tests/` | host regression suites for the active tree |
| `GENGRIX_X1_V3_3_CLAUDE_HANDOFF/.../LFR_PRO_V3_3_SOURCE/` | REFERENCE ONLY: byte-identical copy of the V3.3 baseline (`main` = `2bb762d`). It is a separate sketch folder; never edit or flash it. |
| `GENGRIX_X1_V3_3_CLAUDE_HANDOFF/.../INDEPENDENT_V33_AUDIT.md`, `MASTER_ENGINEERING_CONTEXT.md` | reference requirements / audit |
| `GENGRIX_X1_TONIGHT_CLAUDE_CODING_PACKAGE_2026-10-23/` | reference requirements (untracked; not modified) |
| `SOURCE_CHANGES.patch` | HISTORICAL V3.2 patch only; does not contain V3.3/V3.4 changes. Use git history. |

## Branches

| Branch | Content |
|---|---|
| `main` (`2bb762d`) | V3.3 foundation baseline. Not modified. |
| `dev/v3.4-phase-a` | V3.4 Phase A reliability/safety fixes (this work) |
| `dev/v3.5-performance` | created from the verified V3.4 commit for performance work |

Nothing is pushed or merged without the owner's approval.

## Build (reproducible)

```sh
arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8 --warnings all LFR_PRO_V3
# fallback sensor path:   --build-property "compiler.cpp.extra_flags=-DLFR_FAST_SENSOR=0"
```

Host tests (Git Bash / Linux / WSL with g++): `cd tests && sh run_tests.sh && sh run_v32_regressions.sh && sh run_v33_tests.sh && sh run_v34_tests.sh`
(run in a copy: `run_tests.sh` rewrites `tests/previews/`).
