<p align="center">
  <img src="assets/logo.png" alt="Black Desert Online Amity Solver">
</p>

# Black Desert Online Amity Solver

Small C++17 solver for Black Desert Online's Amity conversation minigame. Selects and orders knowledge to maximize goal success probability, then expected favor.

[![Build and unit tests](https://github.com/abertry/bdo-amity/actions/workflows/cmake.yml/badge.svg)](https://github.com/abertry/bdo-amity/actions/workflows/cmake.yml)
![Unit test coverage](https://img.shields.io/badge/unit_test_coverage-76%25-yellowgreen)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![CMake](https://img.shields.io/badge/CMake-%3E%3D3.16-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

Supports spark, failure, favor, and free-talk goals. Combo effects are not yet supported.

## Build and test

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage

```bash
./build/amity_solver --interest 30 --favor 15 --slots 8 \
  --category "Residents of Velia" --goal consecutive-spark --target 3
```

Use `--help` for all options or `--list-categories` for available knowledge.

Not affiliated with Pearl Abyss Corp. Black Desert Online is a trademark of Pearl Abyss Corp.