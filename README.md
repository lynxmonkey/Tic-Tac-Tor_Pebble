# Toroidal Tic-Tac-Toe (Pebble Time 2)

3×3 tic-tac-toe on a 3D torus.

## Layout
```
Makefile                      host tests:  make test
tests/test_core.c             game-engine tests (6,764 checks)
tests/test_torus.c            geometry/camera/projection tests
pebble/                       Pebble project  (pebble build / pebble install --emulator emery)
  package.json, wscript
  src/c/core/                 PLATFORM-INDEPENDENT: board, wrapping, 12 lines, game/turns, AI, persistence
  src/c/render/torus_geom.*   platform-independent torus maths (board geometry, camera, projection)
  src/c/render/*_render.*     Pebble drawing (flat, torus)
  src/c/ui/                   menu, settings, game screen
  src/c/input_pebble.*        buttons -> NEXT/PREVIOUS/SELECT/BACK
  src/c/platform_storage.*    Pebble persist_* behind the TttStorage interface
```

## Controls
Up = PREVIOUS, Down = NEXT, Select = SELECT, Back = BACK (leave; game is already saved).
Long-press Select toggles Torus/Flat for the current game (presentation only).
