# Agent Instructions - andy-project

## Project scope

This repository contains Andy's creative projects and web pages. The `core2/` directory owns the standalone M5Stack Core2 game firmware.

When changing the firmware, also follow [`core2/AGENTS.md`](./core2/AGENTS.md).

## Core2 boundary

- Treat `core2/` as a completely local, offline game.
- Do not connect it to the LBSocial Mac mini, `lbsocial-local-ai`, Ollama, or any cloud or local AI service.
- Do not add Wi-Fi, remote APIs, telemetry, accounts, or network-required gameplay unless the repository owner explicitly changes the project direction.
- Keep gameplay deterministic and functional without any external service.
- Never commit credentials, local configuration, `.pio/` output, or device secrets.

## Build and verification

Run Core2 commands from `core2/`:

```bash
pio run -e m5stack-core2
```

Do not upload firmware to a physical device unless the repository owner or Andy explicitly asks for a device deployment.

Before committing firmware changes, also run:

```bash
git diff --check
```

## Repository workflow

- Inspect the current Git state and relevant files before editing.
- Keep changes scoped to Andy's projects.
- Use English for code, comments, filenames, and technical documentation.
- Do not create a pull request when the repository owner explicitly requests direct `main`; verify that only intended files are changed before committing or pushing.
- After every coding change or feature completion, ask whether the project should be updated on GitHub.

## Audience and tone

- Keep all content, comments, game elements, and responses appropriate, positive, encouraging, and clear.
- Before any action, ask Andy what he would like to build or learn next, offering simple options if needed.
- Explain each planned step in plain language before executing, and obtain Andy's explicit confirmation.
- After completing a coding change, remind Andy to commit and push the changes to GitHub as a backup.
- A parent/guardian may occasionally co‑teach or join the workflow. Communicate clearly and avoid overly complex implementation plans when simple guidance is enough.
- If Andy asks a question or makes a request that is unclear or underspecified, ask simple clarifying questions instead of making assumptions.

## Privacy and confidentiality

- This repository is public.
- Keep repository content focused on the projects themselves.
- Do not write, store, or commit personal information such as exact age, birthday, school, class, teacher, home location, full legal name, private discussion details, account credentials, device secrets, or private chat transcript details.
- Do not add analytics, tracking scripts, ads, uploads, forms collecting personal information, account features, camera access, or microphone access unless the repository owner explicitly approves that direction.