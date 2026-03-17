# Firmware Architecture

## Module layout

- `src/main.c`
  Boot sequence and top-level main loop orchestration.
- `src/app_state.[ch]`
  Shared runtime state, persistent globals, and cross-module data ownership.
- `src/app_display.[ch]`
  OLED driver integration, drawing primitives, battery/roll rendering, and text output.
- `src/app_ballistics.[ch]`
  Ballistic table loading, interpolation, and range presentation logic.
- `src/app_system.[ch]`
  Initialization, timing, sleep control, calibration, battery sampling, and status reporting.
- `src/app_input.[ch]`
  Button state machine and user interaction behavior.
- `src/app_comm.[ch]`
  UART protocol parsing, CRC handling, and command dispatch.

## Dependency direction

- `main` depends on all application services through `app.h`.
- `app_comm` depends on input/system/ballistics/display APIs, but does not own hardware init.
- `app_input` depends on system/display/ballistics APIs.
- `app_system` depends on display and ballistics APIs.
- `app_ballistics` depends on shared state and display output only.
- `app_display` depends on shared state and low-level hardware headers only.
- `app_state` depends only on core definitions and holds no behavior.

## Maintenance rules

- Add new globals only in `app_state.[ch]`.
- Keep display drawing and font logic out of `main.c` and system modules.
- Keep protocol opcodes and host-facing behaviors in `app_comm.c`.
- Keep ballistic math and EEPROM table decoding in `app_ballistics.c`.
- Keep button behavior in `app_input.c`.
- Keep initialization, timing, sleep, and calibration services in `app_system.c`.
