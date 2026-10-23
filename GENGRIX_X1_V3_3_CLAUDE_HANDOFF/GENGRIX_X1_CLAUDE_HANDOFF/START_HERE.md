# GENGRIX-X1 - Claude Handoff Package

This ZIP was made from the user's latest uploaded `CodeX_LFR.rar` (V3.3 Foundation) and the project's engineering requirements.

## Folder layout

- `MASTER_ENGINEERING_CONTEXT.md`: mandatory goals, locked hardware, outstanding risks and phased roadmap.
- `LFR_PRO_V3_3_SOURCE/`: complete latest user-provided source, tests, stubs, docs, and hash list. Files were copied **unchanged**, except that `.git` history/metadata is excluded.
- `INDEPENDENT_V33_AUDIT.md`: previous independent V3.3 technical audit. Findings should be verified in actual code/tests.
- `V33_AUDIT_REPRO_TESTS.zip`: supplementary reproduction tests, provided for reference.
- `START_HERE.md`: this file.

## Workflow

1. Read `MASTER_ENGINEERING_CONTEXT.md` before proposing changes.
2. Review all modules under `LFR_PRO_V3_3_SOURCE/LFR_PRO_V3/`.
3. Review existing tests under `LFR_PRO_V3_3_SOURCE/tests/`.
4. Review and independently verify `INDEPENDENT_V33_AUDIT.md`.
5. Work on a separate development branch or copy. Do not silently alter the bundled baseline.
6. Make only software changes on the fixed hardware.
7. Report tested facts separately from physical hardware hypotheses.

## Original file integrity

From the original source root, execute:

```
cd LFR_PRO_V3_3_SOURCE
sha256sum -c FILES_SHA256.txt
```

The local source tree contains no external EEPROM or encoder hardware. EEPROM APIs use the MCU's internal Flash.

No firmware changes were introduced when this handoff ZIP was generated.
