# Análisis Detallado de Bugs - Proyecto Football Manager

## 1. include/engine/models.h

### Bug 1.1: Inicialización incompleta de Player
**Línea:** ~25-70 (constructor por defecto de Player)
**Problema:** La estructura `Player` tiene múltiples campos sin inicializador explícito. Los campos numéricos inicializados con `= 0` están correctos, pero campos como `role`, `roleDuty`, `individualInstruction`, etc., pueden dejar strings en estado indeterminado en algunos compiladores.
**Descripción:** Los campos `lastTrainedSeason` y `lastTrainedWeek` se inicializan a `-1`, lo que es problematico cuando se convierten a `size_t` sin validación posterior.
**Impacto:** Logic error / Potencial corrupción de datos
**Severidad:** Medio

### Bug 1.2: Struct Career sin inicialización explícita de myTeam
**Línea:** ~200-280 (struct Career)
**Problema:** `Team* myTeam;` es un puntero sin inicializar por defecto a nullptr
**Descripción:** Si Career se crea sin inicialización explícita, `myTeam` puede contener basura de memoria, causando crashes al acceder.
**Impacto:** Crash / Undefined behavior
**Severidad:** Crítico

---

## 2. src/gui/gui_view_management.cpp (gui_view_builders.cpp)

### Bug 2.1: Acceso a seller sin null check después de findTeamByName
**Línea:** 117
**Código:**
```cpp
const Team* seller = state.career.findTeamByName(state.selectedTransferClub);
const Player* player = seller ? findPlayerByName(*seller, state.selectedTransferPlayer) : nullptr;
```
**Problema:** En línea 146, se accede directamente a `*state.career.myTeam` sin validación:
```cpp
detail << "Rol esperado " << expectedRoleLabel(*state.career.myTeam, *player) << "\r\n";
```
Si `myTeam` es nullptr, crash.
**Impacto:** Crash / nullptr dereference
**Severidad:** Crítico

### Bug 2.2: Iteración sobre vector sin validación de tamaño
**Línea:** 210
**Código:**
```cpp
const Player& player = *players[i];
```
**Problema:** `players` puede ser vacío, pero no hay validación antes del acceso.
**Descripción:** Si `players` está vacío y se intenta acceder a índices, crash por out-of-bounds.
**Impacto:** Crash / Out-of-bounds access
**Severidad:** Alto

---

## 3. src/career/app_services.cpp

### Bug 3.1: findTeamByName sin validación del resultado
**Línea:** 234-240 (chooseAutomaticWeeklyDecision)
**Código:**
```cpp
if (!career.myTeam) return "No hay una carrera activa.";
if (!career.debtStatus.canBuyPlayers) { ... }
if (!canAffordTransfer(career.debtStatus, transferCost, playerWage)) { ... }
if (!career.debtStatus.canOfferHighSalaries &&
    playerWage > max(12000LL, career.myTeam->sponsorWeekly / 2)) { // <-- Acceso sin null check después de validación
```
**Problema:** La validación `if (!career.myTeam)` está al inicio, pero hay múltiples llamadas posteriores que acceden a `career.myTeam` sin revalidar en la función.
**Impacto:** Logic error / Potential nullptr dereference
**Severidad:** Alto

### Bug 3.2: activeTeams acceso sin validación de índices
**Línea:** 429-434
**Código:**
```cpp
if (career.activeTeams.empty()) {
    return; // Pero si no está vacío...
}
Team* selectedTeam = career.activeTeams.front(); // OK
for (Team* team : career.activeTeams) { // OK, iteración con for-each
    // ...
}
```
**Problema:** Si bien hay una validación de `empty()`, hay accesos a `activeTeams[i]` en otras partes sin esta validación (línea 467-468).
**Impacto:** Potential crash if activeTeams modified during iteration
**Severidad:** Medio

---

## 4. src/career/week_simulation.cpp

### Bug 4.1: scheduledTeam() retorna nullptr sin validación en caller
**Línea:** 50, usado en línea 170
**Código:**
```cpp
Team* scheduledTeam(Career& career, int index) {
    TeamRepository teams(career);
    return teams.getActiveTeamByScheduleIndex(index); // Puede retornar nullptr
}
```
**Problema:** En línea 162:
```cpp
for (const auto& match : career.schedule[static_cast<size_t>(career.currentWeek - 1)]) {
    if (Team* home = scheduledTeam(career, match.first)) {
        homeGames[home]++;
    }
}
```
Solo valida `home`, pero no valida que `currentWeek` sea válido antes de acceder a `schedule`.
**Impacto:** Out-of-bounds access / Potential crash
**Severidad:** Crítico

### Bug 4.2: Conversión de int -1 a size_t sin validación
**Línea:** 1474
**Código:**
```cpp
if (career.activeTeams.empty() || career.schedule.empty()) {
    // ...
}
```
**Problema:** En `updateShortlistAlerts()` (línea ~1520):
```cpp
int index = team_mgmt::playerIndexByName(*seller, parts[1]);
if (index < 0) continue; // Validación
const Player& player = seller->players[static_cast<size_t>(index)];
```
Si `index < 0` se continúa, pero la validación con `< 0` es correcta aquí.
**Impacto:** Logic error (puede saltarse el check si hay bug en lógica anterior)
**Severidad:** Alto

### Bug 4.3: Acceso a schedule sin bounds checking
**Línea:** 162, 1038, 1492
**Código ejemplo (línea 1492):**
```cpp
const auto& matches = career.schedule[static_cast<size_t>(career.currentWeek - 1)];
```
**Problema:** No valida que `currentWeek - 1` sea un índice válido en `schedule`.
**Descripción:** Si `currentWeek` es 0 o es > `schedule.size()`, hay out-of-bounds access.
**Impacto:** Crash / Out-of-bounds access
**Severidad:** Crítico

### Bug 4.4: Acceso a activeTeams[i] sin validación de bounds
**Línea:** 1200, 1212
**Código:**
```cpp
if (career.activeTeams[i] == career.myTeam) { // <-- No valida que i < activeTeams.size()
    // ...
}
Team* team = career.activeTeams[i]; // <-- Mismo problema
```
**Problema:** Loop usa `i` como índice sin validar que es menor que `activeTeams.size()`.
**Impacto:** Out-of-bounds access / Crash
**Severidad:** Crítico

---

## 5. src/io/save_serialization.cpp

### Bug 5.1: validateLoadedCareerBasics no valida dinero negativo
**Línea:** 110-120
**Código:**
```cpp
bool validateLoadedCareerBasics(const Career& career) {
    if (career.currentSeason <= 0 || career.currentWeek <= 0) return false;
    if (career.allTeams.empty()) return false;
    if (!career.myTeam) return false;
    for (const auto& team : career.allTeams) {
        if (team.name.empty() || team.division.empty() || team.players.empty()) return false;
        // NO VALIDA: team.budget < 0 o team.debt < 0
        for (const auto& player : team.players) {
            if (player.name.empty()) return false;
        }
    }
    return true;
}
```
**Problema:** No valida que `budget >= 0`, `debt >= 0`, `week >= 1`, `season >= 1`.
**Descripción:** Save corrupted podría cargar con presupuesto negativo sin ser detectado.
**Impacto:** Data corruption / Game state inconsistency
**Severidad:** Alto

### Bug 5.2: decodeMatchCenterSnapshot permite fields vacías
**Línea:** 253-275
**Código:**
```cpp
MatchCenterSnapshot decodeMatchCenterSnapshot(const string& encoded) {
    // ... 
    if (idx < fields.size()) snapshot.myGoals = parseIntField(fields[idx++]);
    // Si field está vacío, parseIntField retorna 0 (default)
    // Pero no valida que los valores sean >= 0
    return snapshot;
}
```
**Problema:** Valores como `myGoals`, `oppGoals` pueden ser negativos sin validación.
**Impacto:** Data corruption / Logic error
**Severidad:** Medio

### Bug 5.3: Falta validación de semana/temporada en PendingTransfer
**Línea:** 360-377
**Código:**
```cpp
entry.effectiveSeason = parseIntField(parts[3], 1);
entry.loanWeeks = parseIntField(parts[4]); // No valida >= 0
// ...
entry.contractWeeks = parseIntField(parts[7], 104);
```
**Problema:** No valida que `loanWeeks`, `contractWeeks` sean >= 0.
**Descripción:** Transfer pendiente podría tener semanas negativas.
**Impacto:** Data corruption / Logic error
**Severidad:** Medio

---

## 6. src/io/io.cpp

### Bug 6.1: rebalanceWeakestSurplusPlayer() con bestIndex = -1
**Línea:** 154-170
**Código:**
```cpp
bool rebalanceWeakestSurplusPlayer(Team& team, RoleCoverage& coverage, const string& targetPosition) {
    int bestIndex = -1; // <-- Sin inicialización
    // ...
    if (bestIndex < 0) return false;
    team.players[bestIndex].position = targetPosition; // <-- Validación correcta
```
**Problema:** Si bien hay validación `if (bestIndex < 0)`, el cast a `size_t` no ocurre aquí, está correctamente protegido.
**Impacto:** OK (validación presente)
**Severidad:** N/A (Bug evitado)

### Bug 6.2: jsonLongField no valida valores negativos
**Línea:** 420-430 (aproximadamente)
**Código:**
```cpp
team.budget = max(0LL, jsonLongField(fields, "budget", team.budget));
team.sponsorWeekly = max(0LL, jsonLongField(fields, "sponsor_weekly", team.sponsorWeekly));
```
**Problema:** Aunque usa `max(0LL, ...)` para asegurar no negativo, esto sucede en `applyJsonTeamFields`. Pero en otras partes del código de carga no está garantizado.
**Impacto:** Data corruption
**Severidad:** Medio

### Bug 6.3: finalizeLoadedTeam no valida plantilla mínima
**Línea:** 188-195
**Código:**
```cpp
void finalizeLoadedTeam(Team& team, int minimumPlayers = 18) {
    trimSquadForDivision(team);
    assignMissingPositions(team);
    // ...
    ensureMinimumSquad(team, effectiveMinimum); // Asume que esto funciona
    // Pero si ensureMinimumSquad no agrega ningún jugador...
    if (team.players.empty()) { // <-- SIN VALIDACION
        // Plantilla vacía después de "finalizar"
    }
}
```
**Problema:** No valida que la plantilla tenga al menos 1 jugador después de `finalizeLoadedTeam`.
**Impacto:** Empty squad / Game state inconsistency
**Severidad:** Alto

---

## 7. src/io/save_manager.cpp

### Bug 7.1: myTeam asignado sin validación de findTeamByName
**Línea:** 182
**Código:**
```cpp
career.myTeam = controlledTeam.empty() ? nullptr : career.findTeamByName(controlledTeam);
```
**Problema:** Si `findTeamByName` retorna nullptr pero `controlledTeam` no es vacío, `myTeam` queda nullptr, lo que puede causar issues.
**Descripción:** Si el archivo guardado hace referencia a un equipo que no existe (nombre cambiado, datos corruptos), myTeam es nullptr sin feedback.
**Impacto:** Silent data loss / Game unplayable
**Severidad:** Alto

---

## 8. Bugs de Casts Peligrosos

### Bug 8.1: Pointer arithmetic sin bounds checking
**Línea:** src/career/week_simulation.cpp ~1270
**Código:**
```cpp
int idx = static_cast<int>(best - &career.myTeam->players[0]);
Player& player = career.myTeam->players[static_cast<size_t>(idx)];
```
**Problema:** `best` es un puntero obtenido de iteración. Si `best` apunta fuera del vector (ej: después del último elemento), el cálculo es undefined behavior.
**Impacto:** Undefined behavior / Potential crash
**Severidad:** Alto

### Bug 8.2: Cast de resultado de búsqueda sin validación
**Línea:** src/gui/gui_view_management.cpp ~145
**Código:**
```cpp
const Player& player = *players[i]; // <-- Doble dereference sin validación
```
**Problema:** Si `players` contiene nullptr, crash al desreferenciar.
**Impacto:** Crash / nullptr dereference
**Severidad:** Crítico

---

## Resumen de Severidades

### CRÍTICO (4):
1. Bug 1.2: Career.myTeam sin inicialización
2. Bug 2.1: Acceso a myTeam sin null check en GUI
3. Bug 4.1: schedule out-of-bounds access
4. Bug 4.4: activeTeams out-of-bounds access
5. Bug 8.2: Player vector nullptr dereference

### ALTO (7):
1. Bug 2.2: Vector iteration without size check
2. Bug 3.1: Repeated myTeam access without revalidation
3. Bug 4.3: schedule bounds checking missing
4. Bug 5.1: No negative money validation in save load
5. Bug 6.3: Empty squad not validated after finalize
6. Bug 7.1: myTeam assignment without validation
7. Bug 8.1: Pointer arithmetic without bounds

### MEDIO (5):
1. Bug 1.1: Player initialization incomplete
2. Bug 3.2: activeTeams access pattern
3. Bug 5.2: Negative match stats not validated
4. Bug 5.3: Negative transfer fields not validated
5. Bug 6.2: Money validation inconsistent

---

## Recomendaciones de Fix

### Priority 1 (Hacer ASAP):
- [ ] Add nullptr checks before all `career.myTeam` accesses in GUI
- [ ] Validate `currentWeek` before accessing `schedule[currentWeek-1]`
- [ ] Add bounds checking for `activeTeams` vector access
- [ ] Initialize `Career.myTeam` explicitly to nullptr

### Priority 2 (Hacer esta semana):
- [ ] Validate save file constraints (no negative budgets/weeks)
- [ ] Add minimum squad size validation after `finalizeLoadedTeam`
- [ ] Protect pointer arithmetic with bounds checks
- [ ] Add null check after `findTeamByName` calls

### Priority 3 (Refactoring):
- [ ] Create utility functions for safe vector access
- [ ] Implement consistent null checking patterns
- [ ] Add validation wrappers for money/week/season fields
