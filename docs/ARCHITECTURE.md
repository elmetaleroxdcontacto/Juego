# Architecture

## Overview

The project is being migrated from a flat, root-level layout into a layered C++ codebase with clearer boundaries between simulation logic, application services, data loading, and presentation.

Current high-level layers:

- `include/engine` and `src/engine`
  - Core domain models such as `Player`, `Team`, `Career`, match data, save/load state and world state.
- `include/competition` and `src/competition`
  - Competition rules, table profiles, season handlers and division-specific behavior.
- `include/simulation` and `src/simulation`
  - Match engine, tactical modifiers, event generation, fatigue, injuries and player impact.
- `include/career` and `src/career`
  - Career services, board summaries, opponent reports, negotiation helpers and application-facing orchestration.
- `include/transfers` and `src/transfers`
  - Transfer negotiation rules, promised roles, salary expectations and move rejection logic.
- `include/io` and `src/io`
  - External data loading, squad normalization and division bootstrap.
- `include/ui`, `src/ui`, `include/gui`, `src/gui`
  - Console UI and Windows GUI.
- `include/validators` and `src/validators`
  - Validation suite for league data, save/load integrity and scheduling consistency.
- `include/utils` and `src/utils`
  - Shared helpers for paths, random utilities, parsing and console input.

## Design Direction

The target architecture is:

- Simulation code returns data instead of printing directly.
- UI layers decide what to show and how to show it.
- Large files are gradually split by domain rather than by historical growth.
- Data lives under `data/`, saves under `saves/`, and build artifacts stay outside source folders.

## Current Migration Notes

- The repository now compiles from `src/` and consumes public headers from `include/`.
- Compatibility bridge headers exist in `include/` to avoid breaking older include paths during migration.
- `io` loading now returns structured warnings that can be surfaced by UI/services instead of writing directly to stdout.
- Match simulation now stores warnings, report lines and event logs inside `MatchResult`, which prepares the engine for UI-independent presentation.

## Known Remaining Work

- `src/ui/ui.cpp` and `src/simulation/simulation.cpp` are still large and remain active migration targets.
- `app_services.cpp` still acts as a transitional orchestration layer and should be further split by domain.
- More service-oriented APIs are needed for season flow, cup flow and management interactions.
