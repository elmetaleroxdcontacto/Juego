# Changelog

## 2026-03-26

### Frontend y portada

- se profesionalizo la portada de `Chilean Footballito` en GUI y CLI con arquitectura de menu compartida
- se agregaron `Continuar`, `Cargar guardado`, `Creditos` y `Salir` al frontend real
- se ampliaron las configuraciones con idioma, velocidad de texto, perfil visual, modo de musica y fade de audio
- las configuraciones ahora se persisten en `saves/game_settings.cfg`

### GUI y rendimiento

- se diferio el cambio de pagina con cola de mensajes para evitar solapes visuales
- se agrego cache de modelos para vistas pesadas como `Fichajes`, `Finanzas` y `Noticias`
- se incorporo traza de tiempo por pagina en el runtime de GUI
- la cabecera ahora resuelve y muestra mejor la division activa, y bloquea `Division` y `Club` cuando hay carrera activa

### Audio y assets

- la musica del frontend se movio a `assets/audio/`
- se agrego `assets/audio/menu_themes.csv` como manifiesto de temas y metadatos basicos
- el audio del menu ahora soporta alcance configurable y fade-in/fade-out

### Persistencia y compatibilidad

- los saves legacy migran automaticamente nombres visibles de division a IDs canonicos
- la reconstruccion de divisiones cargadas ahora se hace desde el registro real del juego

### Tooling y documentacion

- `build.bat` se preparo para compilar objetivos especificos y correr tests opcionalmente
- `README.md` se actualizo para reflejar frontend, audio, settings persistentes y nuevas rutas de compilacion
- `TODO.md` mantiene el historial append-only de todas las entregas
