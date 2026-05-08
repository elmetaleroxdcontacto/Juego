# Índice de Análisis de Bugs - Football Manager Game

Este repositorio contiene un análisis exhaustivo de bugs encontrados en 5 áreas críticas del proyecto.

---

## 📋 Documentos Disponibles

### 1. **BUG_SUMMARY.md** ← EMPEZAR AQUÍ
Resumen ejecutivo con:
- 5 bugs CRÍTICOS (crash inmediato)
- 7 bugs ALTO (corrupción datos/funcionalidad rota)  
- 5 bugs MEDIO (lógica inconsistente)
- Tabla rápida de ubicaciones y tiempos de fix
- Checklist de prioridades

**Leer si:** Necesitas overview rápido (5 minutos)

---

### 2. **BUG_ANALYSIS_DETAILED.md** ← ANÁLISIS COMPLETO
Análisis detallado de cada bug con:
- Número de línea exacto
- Fragmento de código problemático
- Explicación del problema
- Impacto esperado (crash/data loss/logic error)
- Severidad (crítico/alto/medio)
- Recomendaciones de fix

**Leer si:** Necesitas entender la raíz de cada bug (30 minutos)

---

### 3. **BUG_FIXES_GUIDE.md** ← CÓMO ARREGLARLO
Guía práctica con:
- Código ❌ INCORRECTO para cada bug
- Código ✅ CORRECTO con 1-3 opciones de fix
- Explicación de por qué funciona cada fix
- Helper functions reutilizables
- Patrones para problemas recurrentes

**Leer si:** Vas a implementar los fixes (45 minutos)

---

### 4. **INDEX.md** (este archivo)
Índice navegable y checklist de implementación.

---

## 🔴 BUGS CRÍTICOS (Arreglar AHORA)

| ID | Archivo | Línea | Problema | Fix Time |
|----|---------|-------|----------|----------|
| C1 | include/engine/models.h | ~200 | Team* myTeam sin inicialización | 5 min |
| C2 | src/gui/gui_view_management.cpp | 146 | Acceso a myTeam sin null check | 10 min |
| C3 | src/career/week_simulation.cpp | 1492 | schedule[week-1] out-of-bounds | 15 min |
| C4 | src/career/week_simulation.cpp | 1200 | activeTeams[i] out-of-bounds | 15 min |
| C5 | src/gui/gui_view_management.cpp | 210 | Player vector nullptr deref | 10 min |

**Total de fixes críticos: ~55 minutos**

---

## 🟠 BUGS ALTO IMPACTO (Esta semana)

| ID | Archivo | Línea | Problema | Fix Time |
|----|---------|-------|----------|----------|
| A1 | src/gui/gui_view_management.cpp | 210 | Vector sin validación size | 10 min |
| A2 | src/career/app_services.cpp | 234 | myTeam sin revalidación | 15 min |
| A3 | src/career/week_simulation.cpp | 162 | schedule bounds ausente | 15 min |
| A4 | src/io/save_serialization.cpp | 110 | Dinero negativo no validado | 20 min |
| A5 | src/io/io.cpp | 188 | Plantilla vacía no validada | 15 min |
| A6 | src/io/save_manager.cpp | 182 | myTeam assignment sin validate | 10 min |
| A7 | src/career/week_simulation.cpp | 1270 | Pointer arithmetic sin bounds | 20 min |

**Total de fixes altos: ~105 minutos**

---

## 🟡 BUGS MEDIO IMPACTO (Próxima sprint)

| ID | Archivo | Línea | Problema |
|----|---------|-------|----------|
| M1 | include/engine/models.h | 25-70 | Player init incompleta |
| M2 | src/career/app_services.cpp | 467 | activeTeams access pattern |
| M3 | src/io/save_serialization.cpp | 253 | Negative match stats |
| M4 | src/io/save_serialization.cpp | 360 | Negative transfer fields |
| M5 | src/io/io.cpp | 420 | Money validation inconsistent |

---

## 🎯 PLAN DE IMPLEMENTACIÓN

### FASE 1: Bloquear Crashes (2 horas)
```
[  ] C1 - Initialize Career.myTeam = nullptr
[  ] C2 - Add null checks in GUI myTeam access
[  ] C3 - Add bounds check for schedule access
[  ] C4 - Add bounds check for activeTeams access
[  ] C5 - Validate players vector elements
```

### FASE 2: Validación de Datos (1.5 horas)
```
[  ] A4 - Add money validation in save loading
[  ] A5 - Add squad size validation post-load
[  ] A6 - Add null check after findTeamByName
[  ] M3 - Add stat validation in decode functions
[  ] M4 - Add transfer field validation
```

### FASE 3: Refactoring (2 horas)
```
[  ] A7 - Fix pointer arithmetic with bounds
[  ] A3 - Centralize schedule access validation
[  ] A2 - Refactor myTeam access patterns
[  ] M1 - Add explicit initializers to Player
[  ] M5 - Standardize money validation
```

### FASE 4: Testing (1 hora)
```
[  ] Unit test: validateLoadedCareerBasics()
[  ] Unit test: finalizeLoadedTeam()
[  ] Integration test: Save/load cycle
[  ] Stress test: Empty vectors/null pointers
```

**Total estimado: 6.5 horas de desarrollo**

---

## 📊 Estadísticas

### Por Archivo
```
src/career/week_simulation.cpp      4 críticos, 1 alto
src/gui/gui_view_management.cpp     2 críticos, 1 alto
src/io/save_serialization.cpp       0 críticos, 1 alto, 3 medios
include/engine/models.h             1 crítico, 0 altos, 1 medio
src/career/app_services.cpp         0 críticos, 1 alto, 1 medio
src/io/io.cpp                       0 críticos, 1 alto, 1 medio
src/io/save_manager.cpp             0 críticos, 1 alto
```

### Por Tipo
```
Out-of-bounds access      4 bugs
nullptr dereference       3 bugs
Missing validation        7 bugs
Incorrect initialization  2 bugs
Unsafe casting            2 bugs
```

### Por Impacto
```
Crash/Undefined behavior  5 críticos
Data corruption           7 altos
Logic errors              3 medios
```

---

## 🔍 Cómo Usar Esta Documentación

### Flujo 1: "Necesito reportar esto al cliente"
1. Lee **BUG_SUMMARY.md** (5 min)
2. Compila tabla de severidades
3. Presenta timeline de fixes

### Flujo 2: "Voy a arreglarlo ahora"
1. Lee **BUG_SUMMARY.md** checklist (5 min)
2. Para cada bug, abre **BUG_FIXES_GUIDE.md**
3. Copia código ✅ CORRECTO y adáptalo
4. Verifica con **BUG_ANALYSIS_DETAILED.md** si dudas

### Flujo 3: "Necesito entender la raíz"
1. Lee **BUG_ANALYSIS_DETAILED.md** sección específica
2. Examina el código problemático en el archivo
3. Compara con fix en **BUG_FIXES_GUIDE.md**
4. Verifica que el fix cubre el problema

---

## 🚀 Quick Start: Arreglar los 5 Críticos (1 hora)

### Bug C1: Initialize myTeam
```bash
Archivo: include/engine/models.h
Buscar: "Team* myTeam;"
Cambiar a: "Team* myTeam = nullptr;"
```

### Bug C2: Validate myTeam in GUI
```bash
Archivo: src/gui/gui_view_management.cpp
En línea 145, agregar:
    if (!state.career.myTeam) {
        model.detail.content = "Carrera no iniciada.";
        return model;
    }
```

### Bug C3: Validate schedule access
```bash
Archivo: src/career/week_simulation.cpp
En línea 1490, cambiar:
    const auto& matches = career.schedule[...];
    
Por:
    if (career.currentWeek < 1 || career.currentWeek > career.schedule.size()) return;
    const auto& matches = career.schedule[...];
```

### Bug C4: Validate activeTeams access
```bash
Archivo: src/career/week_simulation.cpp
En línea 1200, cambiar:
    if (career.activeTeams[i] == career.myTeam)
    
Por:
    if (i < career.activeTeams.size() && career.activeTeams[i] == career.myTeam)
```

### Bug C5: Validate players vector
```bash
Archivo: src/gui/gui_view_management.cpp
En línea 210, cambiar:
    const Player& player = *players[i];
    
Por:
    if (!players[i]) continue;
    const Player& player = *players[i];
```

---

## 📞 Support

Para cada bug, documentación incluye:
- ✅ Ejemplo de código correcto
- ❌ Ejemplo de código incorrecto
- 🔧 Opciones de fix (1-3 variantes)
- ⚠️ Posibles side effects
- ✓ Cómo verificar que fue arreglado

---

## 📝 Cambios Recientes

- [x] Identificados 17 bugs en 7 archivos
- [x] Clasificados por severidad (5 críticos, 7 altos, 5 medios)
- [x] Documentados con línea exacta y descripción
- [x] Proporcionados fixes con código
- [x] Creado plan de implementación

---

## 🏁 Próximos Pasos

1. **Hoy:** Arreglar los 5 bugs críticos (1 hora)
2. **Mañana:** Arreglar los 7 bugs altos (2 horas)
3. **Esta semana:** Unit tests para validaciones
4. **Próxima semana:** Refactoring de medios + stress testing
5. **Después:** Code review + merge a main

---

**Análisis completado:** 6 Mayo 2026  
**Documentos:** 4 archivos MD (~15,000 palabras)  
**Cobertura:** 17 bugs en 7 archivos críticos

