# BDO Amity Solver

A C++17 solver for Black Desert Online's Amity conversation minigame.

Given an NPC, a conversation goal, and the knowledge available to the player,
the solver selects and orders knowledge to maximize the chance of completing
the goal. Expected favor is used to choose between equally successful orders.

## Features

- Supports spark, failure, consecutive, favor, and free-talk goals
- Selects and orders knowledge for the NPC's available conversation slots
- Calculates goal probability and expected accumulated favor
- Loads knowledge entries from `data/knowledge.json`
- Includes deterministic unit tests

Combo effects are not supported yet.

## Usage

```bash
./build/amity_solver \
  --interest 30 \
  --favor 15 \
  --slots 8 \
  --category "Residents of Velia" \
  --goal consecutive-spark \
  --target 3
```

List available knowledge categories:

```bash
./build/amity_solver --list-categories
```

Run `./build/amity_solver --help` for all goal names and options.

## Build and Test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

On multi-configuration generators such as Visual Studio, add
`--config Debug` or `--config Release` to the build and test commands.

## Disclaimer

This project is not affiliated with Pearl Abyss. Black Desert Online is a
trademark of Pearl Abyss Corp.
