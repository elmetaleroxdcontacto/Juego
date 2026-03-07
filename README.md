# ⚽ C++ Football Manager Simulator

A football management simulation game developed in **C++**, inspired by games like **Football Manager**.

The project focuses on **simulation systems, football logic, management gameplay, and long-term career progression**. It currently centers on the **Chilean league structure for the 2026 season** and is being actively refactored into a more scalable open source codebase.

---

# 📌 Project Status

🚧 **Active Development**

The game is playable and already contains a substantial management simulation core, while the architecture is being reorganized into a cleaner `src/` + `include/` layout.

Current focus areas include:

- Match simulation refactor
- Tactical systems and AI structure
- Transfer market architecture
- Career mode progression
- Save/data organization
- Repository cleanup and contributor readiness

---

# 🎮 Features

## Current Systems

- Career mode
- Chilean league system with multiple divisions
- Promotions and relegations
- League tables and seasonal progression
- Match simulation with tactical and contextual modifiers
- Team management
- Player attributes, morale, chemistry and development data
- Injuries, suspensions and fitness recovery
- Transfer market, pre-contracts and negotiations
- Scouting, shortlist and scouting reports
- Club finances and upgrade systems
- Board confidence and objectives
- Save / Load system
- Match analysis and weekly reporting
- Windows GUI plus console fallback
- Validation suite for data and save integrity

---

# 🏆 Leagues Implemented

The project currently focuses on the **Chilean football league system**.

Supported divisions:

- Primera División
- Primera B
- Segunda División Profesional
- Tercera División A
- Tercera División B

Each league is prepared with:

- Promotion and relegation rules
- Standings and season handling
- Division-specific competition behavior
- Squad and data loading from external files under `data/LigaChilena/`

---

# 🧠 Planned Improvements

The next major improvements planned for the project are:

- Further match engine modularization
- Stronger tactical cause/effect
- More advanced transfer negotiation flow
- Better rival AI adaptation
- Deeper player progression and youth generation
- Cleaner service layer separation from UI
- More externalized data formats and mod support
- Expanded validation and test coverage

---

# 🛠 Technologies

The project is currently built with:

- **C++17**
- Standard Library
- Windows GUI APIs for the native desktop client
- CMake
- Batch build tooling for Windows convenience
- Git / GitHub

It also includes:

- Console interface fallback
- External text/CSV/JSON-style data files for teams and players
- A validation mode for structural consistency checks

---

# 📂 Project Structure

Current repository structure:

```text
FootballManagerGame/
├── data/
│   └── LigaChilena/
├── docs/
│   ├── ARCHITECTURE.md
│   └── ROADMAP.md
├── include/
│   ├── career/
│   ├── competition/
│   ├── engine/
│   ├── gui/
│   ├── io/
│   ├── simulation/
│   ├── transfers/
│   ├── ui/
│   ├── utils/
│   └── validators/
├── saves/
├── src/
│   ├── career/
│   ├── competition/
│   ├── engine/
│   ├── gui/
│   ├── io/
│   ├── simulation/
│   ├── transfers/
│   ├── ui/
│   ├── utils/
│   └── validators/
├── tests/
├── build.bat
├── CMakeLists.txt
└── README.md
```

Compatibility bridge headers are still present in `include/` while the migration away from legacy include paths is completed.

---

# 🚀 How to Build

## Clone the repository

```bash
git clone https://github.com/elmetaleroxdcontacto/Juego.git
cd Juego
```

## Recommended: Build with CMake

```bash
cmake -S . -B build-cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake
```

The executable is generated under:

```text
build-cmake/bin/
```

## Windows Convenience Build

If you prefer the existing batch workflow:

```powershell
build.bat
```

Build without launching the game:

```powershell
$env:FM_SKIP_RUN='1'
cmd /c build.bat
```

`build.bat` will try CMake first and fall back to direct `g++` compilation if CMake is not usable in the current environment.

---

# 🎯 Project Goals

The main goal is to build a **deep football management simulator in C++** with:

- Realistic football systems
- Long-term career gameplay
- Scalable architecture
- Contributor-friendly project structure
- Clear separation between simulation logic and UI

The project is also intended to be a solid open source base for experimenting with:

- Match engines
- Tactical systems
- Sports simulation architecture
- Data-driven league/gameplay systems

---

## 🤝 Contributing

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

## 📋 Help Wanted

Some areas currently needing work:

- Improve match simulation
- Transfer system improvements
- Player attributes system
- Statistics and analytics system
- Code refactoring and optimization

Check the **Issues** section for available tasks.

## ⭐ Support the Project

If you like the project you can help by:

- ⭐ Starring the repository
- Sharing feedback
- Reporting bugs
- Contributing to development

## 📬 Contact

If you want to collaborate, open an **Issue** or contact through GitHub.

Repository:
https://github.com/elmetaleroxdcontacto/Juego
