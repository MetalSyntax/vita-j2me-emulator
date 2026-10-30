# Instrucciones para agentes de IA

Este repositorio es **VitaJ2ME**: un emulador de Java ME (PSPKVM/phoneME)
para PS Vita. Cualquier agente (Claude, Codex, Gemini, Cursor…) debe
seguir el método documentado en:

- **`.claude/skills/vitaj2me-dev/SKILL.md`**: el flujo completo para
  compilar, desplegar por FTP, diagnosticar crashes, parchear Java,
  añadir ajustes/idiomas, documentar, commitear y preparar releases.
- **`CLAUDE.md`**: contexto del proyecto y uso del toolkit LOCAL (nunca el
  `psvita-toolkit` global).
- **`port_progress.md`**: historial de sesiones y bugs (`Bug real #N`).
  Léelo antes de diagnosticar algo que "ya pasó".

Herramientas: `toolkit/deploy.py` (FTP no interactivo),
`toolkit/java_patch.py` (parches a `classes.zip`),
`./toolkit/vitaj2me-toolkit` (TUI).

Reglas mínimas:
- evidencia real (log o dump) antes de arreglar;
- un bug a la vez;
- todo texto visible en inglés, español y portugués (`L()`);
- nada de la UI vieja de PSPKVM en pantalla;
- nunca subir juegos, logs ni dumps a git;
- documentar cada cambio en `port_progress.md`;
- decir siempre qué quedó sin probar en la consola;
- responder al usuario en español.
