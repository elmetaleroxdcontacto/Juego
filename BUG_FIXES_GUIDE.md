# Guía de Fixes para Bugs Encontrados

---

## CRÍTICO #1: Career.myTeam sin inicialización

**Archivo:** `include/engine/models.h`

### ❌ Código Actual
```cpp
struct Career {
    Team* myTeam;  // <-- Sin inicialización
    LeagueTable leagueTable;
    int currentSeason;
    // ...
};
```

### ✅ Fix
```cpp
struct Career {
    Team* myTeam = nullptr;  // <-- Inicializar explícitamente
    LeagueTable leagueTable;
    int currentSeason = 1;   // También inicializar otros campos críticos
    int currentWeek = 1;
    // ...
};
```

---

## CRÍTICO #2: nullptr dereference en GUI (myTeam)

**Archivo:** `src/gui/gui_view_management.cpp`  
**Línea:** 146

### ❌ Código Actual
```cpp
const Team* seller = state.career.findTeamByName(state.selectedTransferClub);
const Player* player = seller ? findPlayerByName(*seller, state.selectedTransferPlayer) : nullptr;
auto selectedPreview = std::find_if(...);
if (!player) {
    model.detail.content = "Selecciona un objetivo para ver detalle.";
} else {
    std::ostringstream detail;
    detail << player->name << " | " << seller->name << " | " << normalizePosition(player->position) << "\r\n";
    // ...
    detail << "Rol esperado " << expectedRoleLabel(*state.career.myTeam, *player) << "\r\n";
    //                                             ^-- Sin validar que myTeam != nullptr
}
```

### ✅ Fix
```cpp
const Team* seller = state.career.findTeamByName(state.selectedTransferClub);
const Player* player = seller ? findPlayerByName(*seller, state.selectedTransferPlayer) : nullptr;

if (!state.career.myTeam) {
    model.detail.content = "Carrera no iniciada.";
    return model;
}

auto selectedPreview = std::find_if(...);
if (!player) {
    model.detail.content = "Selecciona un objetivo para ver detalle.";
} else {
    std::ostringstream detail;
    detail << player->name << " | " << seller->name << " | " << normalizePosition(player->position) << "\r\n";
    // ...
    detail << "Rol esperado " << expectedRoleLabel(*state.career.myTeam, *player) << "\r\n";
    // Ahora validado
}
```

---

## CRÍTICO #3: schedule[currentWeek-1] out-of-bounds

**Archivo:** `src/career/week_simulation.cpp`  
**Línea:** 1492 (y otros)

### ❌ Código Actual
```cpp
void updateShortlistAlerts(Career& career) {
    if (!career.myTeam || career.scoutingShortlist.empty() || career.currentWeek % 4 != 0) return;
    
    vector<string> active;
    for (const auto& item : career.scoutingShortlist) {
        // ...
    }
}

// Otra función:
void processWeekMatches(...) {
    if (career.currentWeek >= 1 && career.currentWeek <= static_cast<int>(career.schedule.size())) {
        for (const auto& match : career.schedule[static_cast<size_t>(career.currentWeek - 1)]) {
            // ... Acceso correcto aquí
        }
    }
}

// Pero en otra función:
void updateContracts(Career& career) {
    // ...
    const auto& matches = career.schedule[static_cast<size_t>(career.currentWeek - 1)];
    // ^-- SIN VALIDACIÓN
}
```

### ✅ Fix
```cpp
// Crear helper function
namespace {
    bool isValidWeek(const Career& career, int week) {
        return week >= 1 && week <= static_cast<int>(career.schedule.size());
    }
    
    const std::vector<std::pair<int, int>>* getWeekMatches(const Career& career, int week) {
        if (!isValidWeek(career, week)) return nullptr;
        return &career.schedule[static_cast<size_t>(week - 1)];
    }
}

// Usar helper:
void updateContracts(Career& career) {
    // ...
    const auto* matches = getWeekMatches(career, career.currentWeek);
    if (!matches) {
        emitUiMessage("[ERROR] Semana inválida en updateContracts");
        return;
    }
    
    for (const auto& match : *matches) {
        // ... Seguro ahora
    }
}

// O inline (más corto):
void updateContracts(Career& career) {
    if (career.currentWeek < 1 || career.currentWeek > static_cast<int>(career.schedule.size())) {
        return; // Salir si inválido
    }
    
    const auto& matches = career.schedule[static_cast<size_t>(career.currentWeek - 1)];
    // ... Resto del código
}
```

---

## CRÍTICO #4: activeTeams[i] out-of-bounds

**Archivo:** `src/career/week_simulation.cpp`  
**Línea:** 1200, 1212

### ❌ Código Actual
```cpp
void someFunction(Career& career) {
    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        // En otro lugar (sin loop):
        if (career.activeTeams[i] == career.myTeam) {  // <-- i puede estar fuera de scope
            //...
        }
    }
}

// Más específicamente:
void updateRivalMemoryForUserMatch(...) {
    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        Team* team = career.activeTeams[i];  // OK dentro del loop
    }
}

// Pero afuera:
void checkTeamStatus(Career& career) {
    static size_t lastCheckedIndex = 0;
    // ...
    Team* team = career.activeTeams[lastCheckedIndex];  // <-- PELIGROSO si activeTeams cambió
}
```

### ✅ Fix
```cpp
// Helper function
namespace {
    Team* getActiveTeamAtIndex(const Career& career, size_t index) {
        if (index >= career.activeTeams.size()) return nullptr;
        return career.activeTeams[index];
    }
}

// Usar:
void checkTeamStatus(Career& career) {
    static size_t lastCheckedIndex = 0;
    
    Team* team = getActiveTeamAtIndex(career, lastCheckedIndex);
    if (!team) {
        lastCheckedIndex = 0;  // Reset si es inválido
        team = getActiveTeamAtIndex(career, 0);
    }
    
    if (!team) return;  // Aún inválido, salir
    
    // ... Usar team seguro
}

// O más simple (si es dentro de un loop):
void someFunction(Career& career) {
    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        Team* team = career.activeTeams[i];
        
        // Aquí team es seguro porque i < size()
        if (team == career.myTeam) {
            // ...
        }
    }
    // NO acceder a activeTeams[i] fuera del loop
}
```

---

## CRÍTICO #5: Player vector nullptr dereference

**Archivo:** `src/gui/gui_view_management.cpp`  
**Línea:** 210

### ❌ Código Actual
```cpp
std::vector<Player*> players = buildPlayerList(...);  // Puede contener nullptr

for (size_t i = 0; i < players.size(); ++i) {
    const Player& player = *players[i];  // <-- Crash si players[i] == nullptr
    
    // Usar player
    std::cout << player.name << "\n";
}
```

### ✅ Fix Opción A (validar each element)
```cpp
std::vector<Player*> players = buildPlayerList(...);

for (size_t i = 0; i < players.size(); ++i) {
    if (!players[i]) continue;  // <-- Skip nullptr
    
    const Player& player = *players[i];
    std::cout << player.name << "\n";
}
```

### ✅ Fix Opción B (filtrar upfront)
```cpp
std::vector<Player*> rawPlayers = buildPlayerList(...);

// Filtrar nullptrs
std::vector<Player*> players;
for (Player* p : rawPlayers) {
    if (p) players.push_back(p);
}

// Ahora seguro
for (size_t i = 0; i < players.size(); ++i) {
    const Player& player = *players[i];
    std::cout << player.name << "\n";
}
```

### ✅ Fix Opción C (asegurar en source)
```cpp
std::vector<Player*> buildPlayerList(const Team& team) {
    std::vector<Player*> players;
    for (auto& player : team.players) {
        players.push_back(&player);  // Nunca nullptr
    }
    return players;
}

// Entonces en el caller, ningún nullptr
for (Player* p : players) {
    std::cout << p->name << "\n";  // Seguro
}
```

---

## ALTO #1: Dinero negativo en cargas de save

**Archivo:** `src/io/save_serialization.cpp`  
**Línea:** 110-120

### ❌ Código Actual
```cpp
bool validateLoadedCareerBasics(const Career& career) {
    if (career.currentSeason <= 0 || career.currentWeek <= 0) return false;
    if (career.allTeams.empty()) return false;
    if (!career.myTeam) return false;
    for (const auto& team : career.allTeams) {
        if (team.name.empty() || team.division.empty() || team.players.empty()) return false;
        // NO VALIDA DINERO
        for (const auto& player : team.players) {
            if (player.name.empty()) return false;
        }
    }
    return true;
}
```

### ✅ Fix
```cpp
bool validateLoadedCareerBasics(const Career& career) {
    // === Validaciones de Career ===
    if (career.currentSeason <= 0) return false;
    if (career.currentWeek <= 0 || career.currentWeek > 52) return false;
    if (career.allTeams.empty()) return false;
    if (!career.myTeam) return false;
    
    // === Validaciones de Equipos ===
    for (const auto& team : career.allTeams) {
        if (team.name.empty() || team.division.empty()) return false;
        
        // Validar dinero
        if (team.budget < 0) {
            emitUiMessage("[Integridad] Equipo " + team.name + " tiene presupuesto negativo");
            return false;
        }
        if (team.debt < 0) {
            emitUiMessage("[Integridad] Equipo " + team.name + " tiene deuda negativa");
            return false;
        }
        
        // Validar plantilla
        if (team.players.empty()) return false;
        
        for (const auto& player : team.players) {
            if (player.name.empty()) return false;
            if (player.age < 16 || player.age > 40) return false;
            if (player.skill < 1 || player.skill > 99) return false;
            if (player.wage < 0) return false;  // Salario no puede ser negativo
        }
    }
    
    return true;
}
```

---

## ALTO #2: Plantilla vacía después de finalizeLoadedTeam

**Archivo:** `src/io/io.cpp`  
**Línea:** 188-195

### ❌ Código Actual
```cpp
void finalizeLoadedTeam(Team& team, int minimumPlayers = 18) {
    trimSquadForDivision(team);
    assignMissingPositions(team);
    int effectiveMinimum = minimumPlayers;
    const int maxSquad = maxSquadForDivision(team.division);
    if (maxSquad > 0) effectiveMinimum = min(effectiveMinimum, maxSquad);
    ensureMinimumSquad(team, effectiveMinimum);
    ensureCompetitiveRoleCoverage(team);
    // NO VALIDA si la plantilla quedó vacía
}
```

### ✅ Fix
```cpp
void finalizeLoadedTeam(Team& team, int minimumPlayers = 18) {
    trimSquadForDivision(team);
    assignMissingPositions(team);
    int effectiveMinimum = minimumPlayers;
    const int maxSquad = maxSquadForDivision(team.division);
    if (maxSquad > 0) effectiveMinimum = min(effectiveMinimum, maxSquad);
    
    ensureMinimumSquad(team, effectiveMinimum);
    ensureCompetitiveRoleCoverage(team);
    
    // === VALIDACIÓN POST-FINALIZE ===
    if (team.players.empty()) {
        // CRISIS: crear plantilla de emergencia
        int minSkill = 40;
        int maxSkill = 65;
        getDivisionSkillRange(team.division, minSkill, maxSkill);
        
        for (int i = 0; i < 18; ++i) {
            const std::string positions[] = {"ARQ", "DEF", "MED", "DEL"};
            std::string pos = positions[i % 4];
            team.addPlayer(makeRandomPlayer(pos, minSkill, maxSkill, 20, 32));
        }
        
        emitUiMessage("[WARN] Plantilla vacía en " + team.name + ", se recreó con 18 jugadores básicos");
    }
    
    // Validación final
    assert(!team.players.empty());
}
```

---

## ALTO #3: myTeam asignado sin validación

**Archivo:** `src/io/save_manager.cpp`  
**Línea:** 182

### ❌ Código Actual
```cpp
bool loadCareer(Career& career, const string& path) {
    // ... cargar datos del archivo ...
    
    std::string controlledTeam = readField("controlled_team");
    career.myTeam = controlledTeam.empty() ? nullptr : career.findTeamByName(controlledTeam);
    // ^-- Si findTeamByName retorna nullptr, myTeam queda nullptr sin warning
    
    return true;  // Retorna true aunque myTeam sea inválido
}
```

### ✅ Fix
```cpp
bool loadCareer(Career& career, const string& path) {
    // ... cargar datos del archivo ...
    
    std::string controlledTeam = readField("controlled_team");
    
    if (controlledTeam.empty()) {
        career.myTeam = nullptr;
    } else {
        career.myTeam = career.findTeamByName(controlledTeam);
        
        // VALIDACIÓN: verificar que se encontró el equipo
        if (!career.myTeam) {
            emitUiMessage("[ERROR] No se encontró equipo controlado: " + controlledTeam);
            emitUiMessage("[INFO] Equipos disponibles: " + joinStringValues(teamNames, ", "));
            
            // Fallback: usar primer equipo
            if (!career.allTeams.empty()) {
                career.myTeam = &career.allTeams.front();
                emitUiMessage("[FALLBACK] Usando equipo por defecto: " + career.myTeam->name);
            } else {
                return false;  // No hay equipos, cargar falló
            }
        }
    }
    
    return true;
}
```

---

## ALTO #4: Pointer arithmetic sin bounds

**Archivo:** `src/career/week_simulation.cpp`  
**Línea:** ~1270

### ❌ Código Actual
```cpp
const Player* best = nullptr;
for (const auto& player : career.myTeam->players) {
    if (player.injured) continue;
    if (!best || player.skill > best->skill) best = &player;  // <-- best apunta a player dentro del vector
}

if (best) {
    int idx = static_cast<int>(best - &career.myTeam->players[0]);  // <-- Puede ser negativo si best es invalido
    Player& player = career.myTeam->players[static_cast<size_t>(idx)];  // <-- Cast inseguro
}
```

### ✅ Fix Opción A (validar el puntero)
```cpp
const Player* best = nullptr;
for (const auto& player : career.myTeam->players) {
    if (player.injured) continue;
    if (!best || player.skill > best->skill) best = &player;
}

if (best && !career.myTeam->players.empty()) {
    // Validar que best apunta dentro del vector
    const auto* begin = &career.myTeam->players[0];
    const auto* end = begin + career.myTeam->players.size();
    
    if (best >= begin && best < end) {  // <-- Validar antes de arithmetic
        int idx = static_cast<int>(best - begin);
        Player& player = career.myTeam->players[static_cast<size_t>(idx)];
        // ... usar player
    }
}
```

### ✅ Fix Opción B (encontrar por índice)
```cpp
const Player* best = nullptr;
size_t bestIndex = std::numeric_limits<size_t>::max();

for (size_t i = 0; i < career.myTeam->players.size(); ++i) {
    const auto& player = career.myTeam->players[i];
    if (player.injured) continue;
    if (best == nullptr || player.skill > best->skill) {
        best = &player;
        bestIndex = i;
    }
}

if (best && bestIndex != std::numeric_limits<size_t>::max()) {
    Player& player = career.myTeam->players[bestIndex];
    // ... usar player
}
```

---

## Templates para Fixes Recurrentes

### Patrón 1: Safe nullptr check
```cpp
// ❌ INCORRECTO
void doSomething(Career& career) {
    career.myTeam->doAction();  // Crash si nullptr
}

// ✅ CORRECTO
void doSomething(Career& career) {
    if (!career.myTeam) return;  // O throw exception
    career.myTeam->doAction();
}
```

### Patrón 2: Safe vector access
```cpp
// ❌ INCORRECTO
Team* team = activeTeams[index];  // OOB si index >= size

// ✅ CORRECTO
if (index >= activeTeams.size()) return nullptr;
Team* team = activeTeams[index];
```

### Patrón 3: Safe int to size_t cast
```cpp
// ❌ INCORRECTO
int idx = -1;
// ...
player = players[static_cast<size_t>(idx)];  // Wraparound!

// ✅ CORRECTO
int idx = -1;
if (idx < 0 || idx >= static_cast<int>(players.size())) return;
player = players[static_cast<size_t>(idx)];
```

