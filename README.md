# BDO Amity Solver

A C++17 implementation of a Black Desert Online (BDO) Amity conversation solver.

The project models the BDO Amity minigame and attempts to find an optimal ordering of available knowledge entries for a given NPC and conversation objective.

## Features

- Static knowledge database
- NPC definitions
- Support for multiple conversation goals
- Knowledge filtering by available entries
- Heuristic ordering engine
- Extensible architecture for future exact solvers (DFS / Dynamic Programming)

## Project Structure

```text
src/
├── main.cpp
├── Knowledge.hpp
├── KnowledgeBase.hpp
├── KnowledgeBase.cpp
├── Npc.hpp
├── Goal.hpp
├── AmitySolver.hpp
└── AmitySolver.cpp
```

## Concepts

### Knowledge

Each knowledge entry contains:

- Interest
- Minimum Favor
- Maximum Favor

Example:

```cpp
{
    "Igor Bartali",
    20,
    45,
    47
}
```

### NPC

Each NPC contains:

- Name
- Interest
- Favor
- Available conversation slots

Example:

```cpp
Npc npc{
    "Lulu",
    20,
    26,
    8
};
```

### Goals

Supported conversation objectives:

```cpp
enum class GoalType {
    FreeTalk,

    Spark,
    ConsecutiveSpark,

    FailSpark,
    ConsecutiveFail,

    MinimumFavor,
    AccumulatedFavor
};
```

Examples:

```cpp
Goal{
    GoalType::ConsecutiveSpark,
    5
};
```

```cpp
Goal{
    GoalType::MinimumFavor,
    31
};
```

```cpp
Goal{
    GoalType::FreeTalk,
    0
};
```

## Usage

Define the target NPC:

```cpp
Npc npc{
    "Lulu",
    20,
    26,
    8
};
```

Define the goal:

```cpp
Goal goal{
    GoalType::FailSpark,
    4
};
```

Specify currently available knowledge:

```cpp
std::vector<std::string> availableNames = {
    "Islin Bartali",
    "Crio",
    "David Finto",
    "Alustin",
    "Igor Bartali",
    "Artemio Fiazza"
};
```

Load knowledge:

```cpp
auto available =
    KnowledgeBase::select(availableNames);
```

Generate ordering:

```cpp
auto order =
    AmitySolver::simpleSort(
        available,
        goal,
        npc
    );
```

Limit the result to the number of available conversation slots:

```cpp
if (order.size() > npc.slots) {
    order.resize(npc.slots);
}
```

Print the result:

```cpp
AmitySolver::printOrder(
    order,
    npc
);
```

## Current Status

The current implementation uses heuristic sorting rules:

| Goal | Strategy |
|--------|----------|
| FailSpark | Lowest Interest first |
| ConsecutiveFail | Lowest Interest first |
| Spark | Highest Interest first |
| ConsecutiveSpark | Highest Interest first |
| MinimumFavor | Favor weighted by spark probability |
| AccumulatedFavor | Favor weighted by spark probability |
| FreeTalk | Favor weighted by spark probability |

The current implementation is a heuristic solver and does **not** guarantee an optimal solution.

## Future Work

### Combo Effects

Support BDO conversation combo mechanics:

- Interest buffs
- Favor buffs
- Delayed effects
- Multi-turn effects

### Exact Solver

Replace heuristic ordering with:

- DFS
- Memoization
- Dynamic Programming

Potential state representation:

```cpp
State {
    usedKnowledgeMask,
    currentPosition,
    currentFavor,
    currentConsecutiveSpark,
    activeBuffs
}
```

### Database Expansion

Planned support for:

- Balenos
- Serendia
- Calpheon
- Mediah
- Valencia
- Kamasylvia
- Drieghan
- O'dyllita
- Ulukita

### Localization

Future localization support:

- English
- Russian
- Korean

Knowledge entries are stored using canonical names to simplify localization mapping.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/amity_solver
```

## Disclaimer

This project is not affiliated with Pearl Abyss.

Black Desert Online is a trademark of Pearl Abyss Corp.