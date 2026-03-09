# Football Manager Chile Simulator

A football management simulator in C++ focused on long-term club building, tactical simulation, and the Chilean league pyramid for the 2026 ruleset.

The project is being refactored into a cleaner modular architecture so match simulation, career flow, transfers, development, finance, and presentation can evolve independently.

## Project Status

Active development.

The repository is playable, validates its data/load flow, and now includes a stronger architectural base:

- `GameController` and `GameEngine` to move top-level flow out of `main.cpp`
- A modular match engine built around pre-match context, phases, typed events, and post-match consequences
- Dedicated modules for tactics, fatigue, morale, player progression, youth intake, and finance projection
- AI helpers for in-match management, squad planning, and transfer evaluation
- Career services that keep save/load/simulation callable without UI logic in `main.cpp`

## Current Features

- Career mode with Chilean multi-division structure
- Primera Division, Primera B, Segunda Division, Tercera Division A, and Tercera Division B
- League tables, promotion/relegation, playoffs, and season handling
- Match simulation with:
  - pre-match context
  - tactical profiles
  - phase-by-phase dominance
  - typed match events
  - xG-style approximation
  - morale and fatigue impact
- Team management, lineup management, and tactical setup
- Transfer market, contract renewals, pre-contracts, loans, scouting, and shortlist flow
- Board confidence, objectives, club operations, and manager reputation
- Player development, youth intake, injuries, suspensions, and morale dynamics
- Save/load flow and validation suite
- Windows GUI entry point plus CLI fallback

## Architecture

Current source layout:

```text
include/
  ai/
  career/
  competition/
  development/
  engine/
  finance/
  gui/
  io/
  simulation/
  transfers/
  ui/
  utils/
  validators/

src/
  ai/
  career/
  competition/
  development/
  engine/
  finance/
  gui/
  io/
  simulation/
  transfers/
  ui/
  utils/
  validators/

data/
  LigaChilena/
  configs/
  leagues/
  players/
  teams/

saves/
tests/
docs/
```

Key runtime layers:

- `engine`
  - `GameController`, `GameEngine`, domain models, career state
- `simulation`
  - match context, match phases, event generation, event resolution, stats, tactics, fatigue, morale
- `ai`
  - in-match management, squad planning, transfer evaluation
- `transfers`
  - negotiation rules, transfer targets, club transfer strategy
- `development`
  - training impact, match experience progression, youth intake generation
- `finance`
  - payroll and weekly cash-flow projection
- `ui/gui`
  - presentation and input only

## Data

The current playable database still lives under `data/LigaChilena/`.

The new `data/configs`, `data/leagues`, `data/teams`, and `data/players` directories are now present to support the next step of externalized data and future mod support without hard-wiring league data into code.

## Build

### CMake

```bash
cmake -S . -B build-cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=C:/MinGW/bin/g++.exe
cmake --build build-cmake
```

Binary output:

```text
build-cmake/bin/FootballManager.exe
```

### Windows helper

```powershell
build.bat
```

Skip auto-run after building:

```powershell
$env:FM_SKIP_RUN='1'
cmd /c build.bat
```

Force the direct `g++` fallback when the local MinGW/CMake toolchain cache is unstable:

```powershell
$env:FM_SKIP_RUN='1'
$env:FM_FORCE_FALLBACK='1'
cmd /c build.bat
```

### Validation

```powershell
.\build-cmake\bin\FootballManager.exe --validate
```

Fallback validation:

```powershell
.\FootballManager.exe --validate
```

### Tests

When CMake can configure a clean build tree:

```bash
cmake --build build-cmake --target FootballManagerTests
ctest --test-dir build-cmake --output-on-failure
```

Current test coverage added in this refactor:

- validation suite against external league/save data
- structured match-engine phase output
- tactical pressure impact on phase fatigue
- competition group table selection
- transfer affordability scoring

## Current Refactor Highlights

- `src/main.cpp` is now only a thin entry point
- top-level application flow moved into `src/engine/game_controller.cpp`
- career orchestration wrapped in `src/career/career_manager.cpp`
- match flow now goes through `src/simulation/match_engine.cpp`
- match flow is now split across `match_context`, `match_phase`, `match_event_generator`, `match_resolution`, and `match_stats`
- simulation data returns `MatchContext`, `MatchStats`, and `MatchTimeline`
- finance and development logic are now reusable modules instead of staying embedded in larger gameplay files
- weekly career simulation now lives in `src/career/week_simulation.cpp` instead of `src/ui/ui.cpp`
- cup mode now has its own UI flow in `src/ui/cup_ui.cpp`
- path handling uses a Unicode-safe compatibility layer for Windows and portable helpers elsewhere

## Planned Improvements

- Continue splitting `src/ui/ui.cpp` and `src/engine/models.cpp`
- Move season-end resolution out of `src/ui/ui.cpp`
- Deepen Chile 2026 competition-specific rules and edge cases
- Expand transfer negotiation rounds and competing bids
- Add richer dressing-room dynamics and promised-role consequences
- Improve analytics, opponent reports, and season memory/history
- Expand external JSON/CSV/TXT loading toward editable databases and mods

## Technologies

- C++17
- Standard Library
- CMake
- Windows GUI APIs
- CLI fallback
- Data files under `data/`

## Contributing

Contributions are welcome!

We are looking for contributors interested in:

- C++ development
- Gameplay systems
- Data creation (teams, players, leagues)
- UI / UX
- Testing and balancing

### How to Contribute

1. Fork the repository
2. Create a new branch
3. Make your changes
4. Submit a Pull Request

## Help Wanted

Some areas currently needing work:

- Improve match simulation
- Transfer system improvements
- Player attributes system
- Statistics and analytics system
- Code refactoring and optimization

Check the **Issues** section for available tasks.

## Support the Project

If you like the project you can help by:

- ⭐ Starring the repository
- Sharing feedback
- Reporting bugs
- Contributing to development

## Contact

If you want to collaborate, open an **Issue** or contact through GitHub.

Repository:
https://github.com/elmetaleroxdcontacto/Juego
