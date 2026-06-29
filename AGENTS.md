# Agent Instructions - andy-project

## Project scope

This repository contains Andy's personal projects and web pages. The
`core2/` directory owns the standalone M5Stack Core2 game firmware.

## Core2 boundary

- Treat `core2/` as a completely local, offline game.
- Do not connect it to the LBSocial Mac mini, `lbsocial-local-ai`, Ollama, or
  any cloud or local AI service.
- Do not add Wi-Fi, remote APIs, telemetry, accounts, or network-required
  gameplay unless Xuebin explicitly changes the project direction.
- Keep gameplay deterministic and functional without any external service.
- Never commit credentials, local configuration, `.pio/` output, or device
  secrets.

## Build and verification

Run Core2 commands from `core2/`:

```bash
pio run -e m5stack-core2
```

Before committing firmware changes, also run:

```bash
git diff --check
```

## Repository workflow

- Inspect the current Git state and relevant files before editing.
- Keep changes scoped to Andy's projects.
- Use English for code, comments, filenames, and technical documentation.
- Do not create a pull request when Xuebin explicitly requests direct `main`;
  verify that only intended files are changed before committing or pushing.
