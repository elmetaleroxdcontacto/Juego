# Audio del frontend

Esta carpeta centraliza los assets de audio usados por `Chilean Footballito`.

## Manifiesto

- `menu_themes.csv`: registra ID, archivo, titulo, licencia declarada, uso previsto y notas de cada tema.

## Tema actual

- `Los Miserables - El Crack  Video Oficial (HD Remastered).mp3`
- Uso: musica del frontend
- Alcance configurable: portada principal o todo el frontend
- Licencia registrada en el manifiesto: dominio publico, segun confirmacion explicita del usuario

## Integracion

- La GUI Win32 busca primero `assets/audio/menu_themes.csv`.
- Si el manifiesto no existe, el juego aun conserva fallback hacia el tema por defecto.
- La seleccion actual del tema queda preparada desde `GameSettings.menuThemeId`.

## Futuro sugerido

- versionar mas de un tema y exponer selector de pista
- agregar metadatos tecnicos como duracion o volumen recomendado
- incorporar sonidos UI separados del tema musical
