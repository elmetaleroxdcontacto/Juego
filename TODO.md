# Lista de Tareas para Juego de Manager de Fútbol Carga de Equipos desde Archivo

## Tareas Completadas
- [x] Analizar la tarea del usuario: Hacer que el juego cargue equipos desde un archivo en lugar de codificados.
- [x] Leer main.cpp para entender la estructura del código.
- [x] Leer archivos de equipos para confirmar divisiones y conteos de equipos.
- [x] Planificar: Modificar initializeLeague para cargar desde teams.txt, actualizar selección de equipos en Juego Rápido y Modo Carrera.
- [x] Editar main.cpp para implementar la carga desde teams.txt en initializeLeague.
- [x] Actualizar selección de equipos en Juego Rápido para cargar desde teams.txt.
- [x] Actualizar bucles de selección de equipos en Modo Carrera para usar índices correctos para 16 equipos de Primera.
- [x] Verificar que los cambios sean correctos.
- [x] Compilar el juego para verificar errores.
- [x] Ejecutar el juego y verificar que inicie correctamente.
- [x] Probar el juego para asegurar que la nueva carga de equipos funcione correctamente (el usuario puede interactuar).
- [x] Implementar la estructura de Jugador con atributos como nombre, posición, ataque, defensa, resistencia, habilidad, edad, valor, lesiones, etc.
- [x] Crear la clase Equipo con métodos para calcular ataque total, defensa total, habilidad promedio, presupuesto, puntos, etc.
- [x] Desarrollar el sistema de liga con tabla de posiciones y clasificación por puntos y diferencia de goles.
- [x] Agregar modo carrera con guardado y carga de progreso en archivo career_save.txt.
- [x] Implementar simulación de partidos con consideración de tácticas, habilidades y resistencia.
- [x] Añadir mercado de transferencias para comprar y vender jugadores.
- [x] Incluir sistema de lesiones aleatorias y recuperación semanal.
- [x] Desarrollar modo juego rápido con selección de equipo y gestión básica.
- [x] Cargar equipos desde archivos de texto con datos de jugadores.
- [x] Agregar estadísticas de equipo y jugadores (victorias, empates, derrotas, goles, etc.).
- [x] Implementar logros y eventos aleatorios en partidos.
- [x] Crear menús en español para una mejor experiencia de usuario.
- [x] Normalizar nombres de equipos para manejar caracteres especiales en archivos.
- [x] Agregar envejecimiento de jugadores con disminución de habilidades.
- [x] Implementar curación de lesiones al final de temporada.
- [x] Desarrollar selección de división en modo carrera (Primera, Primera B, Segunda).
- [x] Actualizar tabla de liga después de cada partido simulado.
- [x] Comprobar que la carga de equipos desde archivos funciona correctamente.
- [x] Verificar que el modo carrera permite guardar y cargar progreso sin errores.
- [x] Probar la simulación de partidos con diferentes tácticas y habilidades.
- [x] Confirmar que el mercado de transferencias permite comprar y vender jugadores correctamente.
- [x] Validar el sistema de lesiones y recuperación semanal.
- [x] Asegurar que el modo juego rápido funciona con selección y gestión básica.
- [x] Revisar que las estadísticas de equipo y jugadores se actualizan correctamente.
- [x] Verificar que los logros y eventos aleatorios se activan adecuadamente.
- [x] Confirmar que los menús en español se muestran correctamente y son funcionales.
- [x] Probar el envejecimiento de jugadores y la disminución de habilidades.
- [x] Validar la curación de lesiones al final de temporada.
- [x] Comprobar la selección de división en modo carrera.
- [x] Asegurar que la tabla de liga se actualiza después de cada partido simulado.
- [x] Agregar opción de retiro de jugador al cumplir entre 35 y 45 años.
- [x] Crear archivo Colo-Colo.txt en primera_division con la plantilla actual de Colo-Colo de la Primera División Chilena.
- [x] Corregir duplicados en XI al completar la alineacion inicial.
- [x] Ampliar la normalizacion de posiciones (laterales, carrileros, etc.).
- [x] Priorizar position_raw cuando position y position_raw difieren.
- [x] Mejorar el scraper para inferir posiciones desde el nombre en Tercera A/B.
- [x] Re-scrapear LigaChilena completa con las nuevas reglas.
- [x] Implementar asignacion automatica de posiciones para jugadores N/A al cargar equipos.
- [x] Validar todas las ligas y equipos (0 N/A tras asignacion).
- [x] Completar plantillas con <18 jugadores en Segunda Division (3 equipos).
Resumen: Se corrigio el XI, se amplio la normalizacion de posiciones, se mejoro el scraper, se re-scrapeo la liga y se validaron/ajustaron plantillas en todas las divisiones.

- Actualizar las plantillas de todos los equipos de la Primera División con los jugadores reales actuales de la temporada 2024/2025, obtenidos de transfermarkt.com.


## Notas
- teams.txt tiene 16 equipos para Primera División.
- Original: Primera 9 (0-8), Segunda 10 (9-18), Primera B 12 (19-30), Total 31.
- Nuevo: Primera 16 (0-15), Segunda 10 (16-25), Primera B 12 (26-37), Total 38.
- Plantillas canonicas: players.csv (players.txt/players.json quedan como referencia).
- Equipos por division se leen desde LigaChilena/<division>/teams.txt.

- [x] Agregar Tercera División A con equipos y jugadores
- [x] Agregar Tercera División B con equipos y jugadores
- [x] Verificar que la API incluya las nuevas divisiones

- [x] Documentar todo el trabajo realizado en español en TODO.md
Nota: valores monetarios usan enteros de 64 bits; entrada manual hasta 1e12.
- [x] Modularizacion del codigo en archivos (models/io/simulation/ui) y uso de std::filesystem.

## Tareas Recientes (2026)

- [x] Corregir error de compilación en utils.cpp: "argument of type 'const char *' is incompatible with parameter of type 'LPCWSTR'"
  - El problema era que las funciones Windows FindFirstFile/FindNextFile usaban versiones Unicode por defecto
  - Solución: Cambiar a versiones ANSI explícitas (FindFirstFileA, FindNextFileA) y estructura WIN32_FIND_DATAA
- [x] Crear script de compilación build.bat para facilitar la compilación del juego
  - Compila todos los archivos fuente: main.cpp, io.cpp, models.cpp, simulation.cpp, ui.cpp, utils.cpp
  - Genera el ejecutable FootballManager.exe
  - Uso: ejecutar build.bat en la carpeta del proyecto
- [x] Agregar pausa después de cada opción en los menús del juego
  - Modo Carrera: después de cada opción (excepto volver), muestra "Presiona Enter para continuar..."
  - Juego Rápido: después de cada opción (excepto volver), muestra "Presiona Enter para continuar..."
- [x] Mejoras prioritarias: ventaja local y clima, fatiga/condicion persistente, eventos de partido, entrenamiento con rendimientos decrecientes y limite semanal, vista de equipo con XI, y riesgo de lesiones por edad/condicion/tactica.
- [x] Mejoras prioritarias: ventaja local y clima, fatiga/condicion persistente, eventos de partido, entrenamiento con rendimientos decrecientes y limite semanal, vista de equipo con XI, y riesgo de lesiones por edad/condicion/tactica.
- [x] Implementar moral de equipo, potencial de jugadores y desarrollo por partidos.
- [x] Agregar ascensos/descensos con playoffs y ajustes de division.
- [x] Incorporar finanzas semanales (ingresos + sueldos) y mercado con ofertas/CPU.
- [x] Añadir lesiones con tipo y duracion; nuevas tacticas (Pressing/Counter).
- [x] Incluir estadisticas post-partido y editor rapido de equipo/jugadores.
- [x] Scouting con incertidumbre, contratos/salarios y renovaciones.\n- [x] Roles por jugador y plan de entrenamiento semanal.\n- [x] Cantera, eventos de club y dashboard semanal.\n- [x] Ajuste de tacticas CPU segun rival/moral.\n- [x] Modo Copa (eliminacion directa).
- [x] Roles por jugador, salarios/contratos, historial de lesiones y plan de entrenamiento semanal.
- [x] Scouting con incertidumbre, desarrollo juvenil, eventos de club y dashboard semanal.
- [x] IA tactica adaptativa, partidos clave y modo Copa (eliminacion directa).
 
## Cambios recientes (2026-03-05)
- Implementado formato de Segunda División (14 equipos) con grupos Norte/Sur, calendario intra-grupo ida/vuelta (14 fechas), tablas por grupo y clasificación automática a playoff/descenso.
- Playoff de Segunda División ahora con llaves ida y vuelta; repechaje 4° vs 4° a partido único.
- Tablas especiales para Primera División y Primera B con nuevos criterios de desempate (DG, ganados, GF, goles visita, menos rojas/amarillas, sorteo).
- Nuevos campos por equipo: goles de visita, tarjetas amarillas/rojas y semilla de sorteo; guardado/carga actualizado.
- Simulación de tarjetas y registro de goles de visita en cada partido.
- Final por el título en Primera División y Primera B si hay empate en puntos (partido único + penales).
- Liguilla de Primera B (2°-8°) ida/vuelta con reglas de localía; final con prórroga + penales.
- Descenso en Primera B: último lugar con definición si hay empate en puntos.
- Ascensos/descensos actualizados: Primera División recibe campeón + liguilla de Primera B; Primera B asciende campeón + liguilla y desciende 1 a Segunda.

## Cambios recientes (2026-03-06)
- Ajustada la implementación de Primera B 2026 para usar desempates por título y descenso en cancha neutral, sin ventaja de localía.
- `playMatch` ahora soporta partidos en sede neutral; se usa en finales y definiciones de desempate.
- Corregida la evaluación de llaves ida/vuelta de la liguilla de ascenso para decidir por puntaje de serie y luego diferencia de gol.
- Final de la liguilla de Primera B ajustada para aplicar prórroga y luego penales solo si persiste la igualdad tras puntaje de serie y diferencia de gol.
- Empates múltiples por el 1° lugar o por el último lugar ahora se reducen a dos equipos usando el orden oficial de clasificación antes del partido definitorio.
- Validado el fixture de Primera B: 16 equipos, 30 fechas, 8 partidos por fecha y segunda rueda espejada invirtiendo localía.
- Compilación verificada exitosamente con `build.bat` tras los cambios.

## Cambios recientes (2026-03-06) - Tercera Division A/B
- Implementado formato 2026 de Tercera División A con 16 equipos, todos contra todos ida y vuelta y calendario automático de 30 fechas.
- Agregados desempates específicos para Tercera A: puntos, diferencia de gol, goles a favor, partidos ganados, goles de visita y enfrentamientos directos.
- Se incorporó registro persistente de enfrentamientos directos por equipo y su guardado/carga en la carrera.
- Implementado playoff de ascenso de Tercera A para 2°-5°: semifinales ida y vuelta (2° vs 5°, 3° vs 4°) y final a partido único.
- Implementado cierre de temporada específico de Tercera A con ascenso directo del 1°, ascenso adicional por playoff y cruces de promoción/descenso frente a Tercera B.
- Implementado formato 2026 de Tercera División B con 28 equipos divididos en Zona Norte y Zona Sur, cada una con tabla independiente y fixture automático por grupo.
- Agregado soporte general de divisiones con grupos en la UI y en el calendario para reutilizar la lógica entre Segunda División y Tercera B.
- Implementados ascensos directos de Tercera B para los campeones de Zona Norte y Zona Sur.
- Implementada final entre campeones de grupo para definir al campeón de Tercera B.
- Implementados cruces 2°/3° entre zonas y llaves de promoción entre Tercera B y Tercera A.
- Actualizados `teams.txt` de Tercera A y Tercera B a una estructura 2026 de 16 y 28 equipos respectivamente.
- Creadas carpetas placeholder para clubes nuevos que no existían en el repositorio, permitiendo que el cargador genere planteles automáticos y no descarte equipos.
- Validaciones realizadas:
  - Tercera A queda con 16 equipos y 30 fechas.
  - Tercera B queda con 28 equipos y 2 grupos de 14.
  - Compilación verificada exitosamente con `build.bat`.
- Nota de implementación:
  - En Tercera B la fase zonal quedó a una rueda por grupo y los cruces 2°/3° se usaron como filtro para la promoción A/B siguiendo la especificación dada en la solicitud.

## Cambios recientes (2026-03-06) - Motor de competiciones
- Creado un nuevo módulo de configuración de competiciones en `competition.h` y `competition.cpp`.
- Centralizadas en `CompetitionConfig` las reglas base por división:
  - perfil de tabla
  - handler de fin de temporada
  - formato de grupos
  - ingresos semanales
  - factor salarial
  - divisor de presupuesto
  - tamaño máximo de plantel
  - cantidad esperada de equipos
- `models.cpp` ahora usa esta configuración para:
  - decidir el perfil de desempates de tabla
  - determinar si una división usa grupos
  - construir grupos regionales
  - generar calendarios según formato configurado
- `io.cpp` ahora usa la configuración para:
  - limitar tamaño máximo de plantel por división
  - calcular presupuesto inicial según divisor configurado
- `ui.cpp` ahora usa la configuración para:
  - nombres de grupos (por ejemplo, Zona Norte / Zona Sur)
  - ingresos base por división
  - factor salarial semanal
  - despacho del cierre de temporada según handler de competición
- `build.bat` fue actualizado para compilar el nuevo módulo `competition.cpp`.
- Resultado:
  - el juego depende menos de `if (division == ...)` repartidos por múltiples archivos
  - queda una base más limpia para seguir moviendo playoffs, ascensos y descensos a reglas declarativas
- Validación realizada:
  - compilación verificada exitosamente con `build.bat`.
