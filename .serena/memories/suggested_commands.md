# Common Commands & Workflows

- Build firmware (resistive 2.8"): `pio run -e cyd_28r`
- Build firmware (capacitive 2.8"): `pio run -e cyd_28c`
- Build firmware (capacitive 3.5"): `pio run -e cyd_35c`
- Host native tests: `pio test -e native`
- Generate compilation database: `pio run -t compiledb`
- Package binaries for testing: `bash .agents/skills/package-binaries/package_binaries.sh cyd_28c`
- Serena project health-check: `serena project health-check .`
- CodeGraph exploration: `codegraph explore "<symbol>"`
- Context-Mode sandbox: `ctx_execute(language: "shell", code: "...")`
