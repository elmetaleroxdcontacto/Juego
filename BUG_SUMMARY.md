# Resumen Ejecutivo de Bugs - Football Manager

## BUGS CRÍTICOS (5 TOTAL)

### 1. **Career.myTeam sin inicialización** 
- **Archivo:** include/engine/models.h
- **Línea:** ~200-280
- **Impacto:** CRASH
- **Severidad:** CRÍTICO
- **Descripción:** Puntero `Team* myTeam` no inicializado por defecto a nullptr. Accesos posteriores = undefined behavior.

### 2. **nullptr dereference en GUI (myTeam)**
- **Archivo:** src/gui/gui_view_management.cpp
- **Línea:** 146
- **Impacto:** CRASH
- **Severidad:** CRÍTICO
- **Descripción:** Acceso a `state.career.myTeam->name` sin null check. Si myTeam es nullptr, crash inmediato.

### 3. **schedule[currentWeek-1] out-of-bounds**
- **Archivo:** src/career/week_simulation.cpp
- **Línea:** 1492 (y 162, 1038)
- **Impacto:** CRASH
- **Severidad:** CRÍTICO
- **Descripción:** Sin validar que `currentWeek` es índice válido. Si currentWeek > schedule.size(), crash.

### 4. **activeTeams[i] out-of-bounds**
- **Archivo:** src/career/week_simulation.cpp
- **Línea:** 1200, 1212
- **Impacto:** CRASH
- **Severidad:** CRÍTICO
- **Descripción:** Acceso directo a activeTeams[i] sin verificar i < activeTeams.size().

### 5. **Player vector nullptr dereference**
- **Archivo:** src/gui/gui_view_management.cpp
- **Línea:** 210
- **Impacto:** CRASH
- **Severidad:** CRÍTICO
- **Descripción:** Doble dereference `*players[i]` sin validar que players[i] no es nullptr.

---

## BUGS ALTO IMPACTO (7 TOTAL)

### 6. **Vector sin validación de tamaño antes de iterate**
- **Archivo:** src/gui/gui_view_management.cpp (gui_view_overview.cpp)
- **Línea:** 210
- **Impacto:** CRASH
- **Severidad:** ALTO
- **Descripción:** Iteración sobre `players` sin comprobar si está vacío primero.

### 7. **myTeam access sin revalidación**
- **Archivo:** src/career/app_services.cpp
- **Línea:** 234-240
- **Impacto:** LOGIC ERROR
- **Severidad:** ALTO
- **Descripción:** Se valida `if (!career.myTeam)` al inicio pero se accede múltiples veces sin revalidación posterior.

### 8. **schedule bounds checking ausente**
- **Archivo:** src/career/week_simulation.cpp
- **Línea:** 162
- **Impacto:** CRASH
- **Severidad:** ALTO
- **Descripción:** No valida `currentWeek >= 1 && currentWeek <= schedule.size()` antes de acceso.

### 9. **Dinero negativo en cargas de save**
- **Archivo:** src/io/save_serialization.cpp
- **Línea:** 110-120
- **Impacto:** DATA CORRUPTION
- **Severidad:** ALTO
- **Descripción:** `validateLoadedCareerBasics()` no verifica `budget >= 0` o `debt >= 0`. Save corrupto carga sin detección.

### 10. **Plantilla vacía después de finalizeLoadedTeam**
- **Archivo:** src/io/io.cpp
- **Línea:** 188-195
- **Impacto:** GAME UNPLAYABLE
- **Severidad:** ALTO
- **Descripción:** Sin validar que la plantilla tenga mínimo 1 jugador después de cargar.

### 11. **myTeam asignado sin validación de búsqueda**
- **Archivo:** src/io/save_manager.cpp
- **Línea:** 182
- **Impacto:** SILENT DATA LOSS
- **Severidad:** ALTO
- **Descripción:** `career.myTeam = findTeamByName(...)` - Si retorna nullptr pero nombre no vacío, myTeam inválido sin feedback.

### 12. **Pointer arithmetic sin bounds**
- **Archivo:** src/career/week_simulation.cpp
- **Línea:** ~1270
- **Impacto:** UNDEFINED BEHAVIOR
- **Severidad:** ALTO
- **Descripción:** `int idx = static_cast<int>(best - &players[0])` sin validar que `best` apunta dentro del vector.

---

## BUGS MEDIO IMPACTO (5 TOTAL)

### 13. **Player initialization incompleta**
- **Archivo:** include/engine/models.h
- **Línea:** 25-70
- **Impacto:** LOGIC ERROR
- **Severidad:** MEDIO
- **Descripción:** Campos sin inicializador explícito. lastTrainedSeason/Week = -1 problemáticos en conversiones a size_t.

### 14. **activeTeams access pattern**
- **Archivo:** src/career/app_services.cpp
- **Línea:** 467-468
- **Impacto:** POTENTIAL CRASH
- **Severidad:** MEDIO
- **Descripción:** Acceso a activeTeams sin consistente validación en todos los paths.

### 15. **Negative match stats no validados**
- **Archivo:** src/io/save_serialization.cpp
- **Línea:** 253-275
- **Impacto:** DATA CORRUPTION
- **Severidad:** MEDIO
- **Descripción:** decodeMatchCenterSnapshot() acepta myGoals/oppGoals negativos sin validación.

### 16. **Negative transfer fields**
- **Archivo:** src/io/save_serialization.cpp
- **Línea:** 360-377
- **Impacto:** DATA CORRUPTION
- **Severidad:** MEDIO
- **Descripción:** loanWeeks, contractWeeks pueden ser negativos sin validación.

### 17. **Inconsistent money validation**
- **Archivo:** src/io/io.cpp
- **Línea:** 420-430
- **Impacto:** DATA CORRUPTION
- **Severidad:** MEDIO
- **Descripción:** Algunos paths validan `budget >= 0`, otros no. Inconsistencia en applyJsonTeamFields vs otros.

---

## TABLA RÁPIDA: CRASH RISK

| Archivo | Línea | Tipo | Severidad | Fix Time |
|---------|-------|------|-----------|----------|
| models.h | ~200 | Init | CRÍTICO | 5 min |
| gui_view_*.cpp | 146 | nullptr | CRÍTICO | 10 min |
| week_simulation.cpp | 1492 | OOB | CRÍTICO | 15 min |
| week_simulation.cpp | 1200 | OOB | CRÍTICO | 15 min |
| gui_view_*.cpp | 210 | nullptr | CRÍTICO | 10 min |
| save_serialization.cpp | 110 | Validation | ALTO | 20 min |
| io.cpp | 188 | Validation | ALTO | 15 min |
| save_manager.cpp | 182 | Validation | ALTO | 10 min |
| week_simulation.cpp | 1270 | Arithmetic | ALTO | 20 min |

---

## QUICK FIX CHECKLIST

### Inmediato (HORAS):
- [ ] Add `if (!career.myTeam) return;` en GUI línea 146
- [ ] Add bounds check para `schedule` en línea 1492: `if (career.currentWeek <= 0 || career.currentWeek > career.schedule.size())`
- [ ] Add bounds check para `activeTeams` en línea 1200-1212
- [ ] Validate `!players.empty()` antes de línea 210
- [ ] Initialize `Career::myTeam = nullptr` en constructor

### Esta semana:
- [ ] Implementar validación en `validateLoadedCareerBasics()` para dinero/semanas
- [ ] Add squad size validation post-load
- [ ] Add null check después de todos los `findTeamByName()`
- [ ] Fix pointer arithmetic bounds check

### Próxima sprint:
- [ ] Refactorizar para usar safe accessors
- [ ] Audit completo de conversiones int<->size_t
- [ ] Unified validation framework para saves

---

## ARCHIVOS MÁS PROBLEMÁTICOS

1. **src/career/week_simulation.cpp** - 4 bugs críticos (out-of-bounds)
2. **src/gui/gui_view_management.cpp** - 3 bugs críticos (nullptr access)
3. **src/io/save_serialization.cpp** - 3 bugs altos (validation)
4. **include/engine/models.h** - 2 bugs (initialization)
5. **src/io/io.cpp** - 2 bugs (validation)

