# ⚽ Football Manager Simulation (C++)

![Version](https://img.shields.io/badge/version-0.1.0--alpha-blue)
![Language](https://img.shields.io/badge/language-C++-brightgreen)
![Status](https://img.shields.io/badge/status-active%20development-orange)
![Contributions](https://img.shields.io/badge/contributions-welcome-success)

A **Football Manager style simulation game** built in **C++**.

The goal of this project is to create a deep football management experience including leagues, teams, tactics, transfers, and match simulations.

This project is **open source** and collaborators are welcome.

---

# 📌 Project Status

🚧 **Alpha Development**

The core systems of the game are currently under development.
Many systems are experimental and will evolve as the project grows.

---

# 🎮 Current Features

Implemented or in progress:

* League system
* Team structure
* Player database
* Basic match simulation
* Season progression
* Basic statistics

---

# 🚀 Planned Features

Future development goals:

* Transfer market
* Tactical system
* Player development and aging
* Financial system
* Staff and scouting
* Training system
* Save / Load system
* UI improvements
* Multiple leagues
* Modding support

---

# 🗺 Development Roadmap

### Phase 1 – Core Systems

* Player structure
* Team management
* League system
* Match simulation engine

### Phase 2 – Gameplay Systems

* Transfers
* Tactics
* Player attributes and progression
* Match statistics

### Phase 3 – Advanced Systems

* Finances
* Staff and scouting
* Training system
* Dynamic seasons

### Phase 4 – Expansion

* Multiple leagues
* Mod support
* UI improvements

---

# 🛠 Technologies

Current stack:

* **C++**
* **Git / GitHub**

Possible future integrations:

* GUI system
* Database support
* External modding tools

---

# 📂 Project Structure

Current structure:

```
FootballManagerGame/
├── include/
│   ├── career/
│   ├── simulation/
│   └── transfers/
├── src/
│   ├── career/
│   ├── competition/
│   ├── gui/
│   ├── io/
│   ├── simulation/
│   ├── transfers/
│   ├── ui/
│   ├── utils/
│   └── validators/
├── data/
│   └── LigaChilena/
├── saves/
├── tests/
├── build.bat
├── CMakeLists.txt
└── *.cpp / *.h
```

Note:
The project is currently in a staged migration. Some legacy root `.cpp` / `.h` files remain active while shared logic is being moved into `src/` and `include/`.

---

# ▶ How to Run

1. Clone the repository

```
git clone https://github.com/elmetaleroxdcontacto/Juego.git
```

2. Open the project with your preferred C++ IDE

Examples:

* Visual Studio
* Code::Blocks
* CLion

3. Compile the project

Windows batch build:

```powershell
build.bat
```

Build only, without opening the game:

```powershell
$env:FM_SKIP_RUN='1'
cmd /c build.bat
```

If CMake is installed:

```powershell
cmake -S . -B build-cmake
cmake --build build-cmake
```

4. Run the executable

---

# 🤝 Contributing

Contributions are welcome!

We are looking for contributors interested in:

* C++ development
* Gameplay systems
* Data creation (teams, players, leagues)
* UI / UX
* Testing and balancing

### How to contribute

1. Fork the repository
2. Create a branch
3. Make your changes
4. Submit a Pull Request

---

# 📋 Help Wanted

Some areas currently needing work:

* Improve match simulation
* Transfer system
* Player attributes system
* Statistics system
* Code refactoring

Check the **Issues section** for available tasks.

---

# ⭐ Support the Project

If you like the project:

* ⭐ Star the repository
* Share feedback
* Contribute to development

---

# 📬 Contact

If you want to collaborate, open an **Issue** or contact through GitHub.

Repository:
https://github.com/elmetaleroxdcontacto/Juego

---

⚽ *Building a football management simulation from scratch.*
