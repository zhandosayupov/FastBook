# Design decisions

Format: what was chosen, why, what was rejected.

## D-001 Release optimization level: -O2

Chosen: `-O2 -DNDEBUG`.
Later: Use -O3 and benchmark.

## D-002 Sanitizers in separate presets

ASan+UBSan in `asan`, TSan in `tsan`: they cannot be combined in one binary.

## D-003 -Werror only on own targets

No -Werror during testing and benchmarking.
