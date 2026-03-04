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
