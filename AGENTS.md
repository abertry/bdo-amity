# AGENTS.md

## Agent Communication

At start of every agent session, load and use installed `caveman` skill at `full` intensity.
Keep it active for all responses unless user explicitly requests `stop caveman` or `normal mode`.
If skill is unavailable, follow its core behavior: communicate tersely while preserving all technical substance.

## Purpose

You are developing a Black Desert Online (BDO) Amity Conversation Solver written in modern C++.

The objective is to find the optimal conversation sequence for a given NPC, available knowledge set, and conversation goal.

The long-term goal is to replicate and eventually outperform existing community Amity solvers.

This project should evolve from a simple heuristic implementation into a mathematically correct solver based on state-space search and dynamic programming.

---

# Project Goals

Priority order:

1. Correctness
2. Maintainability
3. Extensibility
4. Performance

Do not introduce premature optimizations.

Prefer clear and testable code over clever code.

---

# Technical Requirements

## Language

* C++17 minimum
* STL preferred
* No external runtime dependencies unless explicitly justified

## Build System

* CMake

## Platform

* Windows
* Linux

---

# Architecture Principles

## Single Responsibility

Each component should have a clearly defined responsibility.

### Knowledge

Represents a single knowledge entry.

Contains:

* Interest
* Favor range
* Combo effects (future)

Knowledge objects must remain simple data containers.

---

### KnowledgeBase

Static knowledge repository.

Responsibilities:

* Store knowledge definitions
* Lookup by name
* Return subsets of available knowledge

Must not contain solver logic.

---

### NPC

Represents the NPC currently being targeted.

Contains:

* Name
* Interest
* Favor
* Conversation slots

NPC objects must remain simple data containers.

---

### Goal

Represents the conversation objective.

Current supported goals:

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

Goals must be represented by:

```cpp
struct Goal {
    GoalType type;
    int target;
};
```

Examples:

```cpp
Goal{
    GoalType::Spark,
    3
};
```

```cpp
Goal{
    GoalType::ConsecutiveSpark,
    5
};
```

```cpp
Goal{
    GoalType::AccumulatedFavor,
    67
};
```

```cpp
Goal{
    GoalType::FreeTalk,
    0
};
```

---

### Solver

All decision-making belongs inside the solver.

The solver must eventually become independent of any UI or CLI.

---

# Current Solver

Current implementation is heuristic.

It should be treated as temporary.

The heuristic implementation exists only to validate architecture and data structures.

Do not over-engineer heuristic code.

---

# Long-Term Solver Strategy

The final solver should be an exact optimizer.

Expected techniques:

* DFS
* Memoization
* Dynamic Programming
* Branch and Bound

The solver should evaluate:

* knowledge selection
* knowledge ordering
* combo effects
* success probability
* favor accumulation

The solver should maximize objective satisfaction probability and expected reward.

---

# State Representation

Future solver state will likely resemble:

```cpp
struct State {
    uint64_t usedKnowledgeMask;

    int position;

    int currentFavor;
    int accumulatedFavor;

    int currentSparkChain;
    int currentFailChain;

    ActiveBuffs buffs;
};
```

This structure is only a guideline and may evolve.

---

# Combo Effects

Combo effects are a first-class feature.

The architecture must support:

* Interest bonuses
* Favor bonuses
* Delayed activation
* Multi-turn activation
* Multiple simultaneous effects

Do not hardcode assumptions that prevent combo support.

---

# Database Design

Knowledge entries must use canonical English names.

Example:

```cpp
"Dudora Doriven"
```

not

```cpp
"dudora"
```

This simplifies future localization.

Knowledge names should be treated as stable identifiers.

---

# Localization

Future support:

* English
* Russian
* Korean

Solver logic must never depend on localized strings.

Localization should be implemented through mapping layers.

---

# Coding Standards

## Naming

Types:

```cpp
PascalCase
```

Functions:

```cpp
camelCase
```

Variables:

```cpp
camelCase
```

Constants:

```cpp
UPPER_SNAKE_CASE
```

---

## Memory

Prefer:

* value semantics
* std::vector
* std::string
* std::optional
* std::unordered_map

Avoid:

* raw pointers
* manual memory management

---

## Const Correctness

Use const whenever possible.

Functions should be pure whenever practical.

---

## Complexity

Before introducing complexity:

* justify it
* document it
* measure it

Do not optimize without evidence.

---

# Testing

Every non-trivial solver feature should be testable.

Future additions should include:

* unit tests
* deterministic solver tests
* regression tests

The solver should be verifiable against known BDO conversation examples.

---

# What Not To Do

Do not:

* couple solver logic to UI
* couple solver logic to localization
* introduce global mutable state
* hardcode NPC-specific behavior
* hardcode region-specific behavior

Avoid shortcuts that would make exact solving difficult later.

---

# Development Philosophy

When making design decisions:

1. Favor correctness over speed.
2. Favor extensibility over convenience.
3. Keep data structures simple.
4. Keep the solver isolated.
5. Assume combo effects and exact solving will be implemented later.
6. Preserve clean architecture even if the current heuristic implementation does not need it.

The project is intended to evolve into a complete BDO Amity optimization engine, not merely a sorting utility.
