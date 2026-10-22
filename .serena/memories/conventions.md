# Conventions & Style Guidelines

- RTK prefix: use `rtk` for git/gh CLI commands.
- Never commit `.env` or files containing secrets or passwords.
- Follow Catppuccin color tokens and Montserrat typography scales defined in `.agents/design.md`.
- Adding new settings: update `config/config.h`, `settings_manager`, `ui.cpp`, subsystem wiring, and `test_settings.cpp`.
- Touch target sizing: minimum 32x32 px, preferably >=40x40 px.
- Non-blocking execution: no delay loops in UI thread; keep scans asynchronous.
