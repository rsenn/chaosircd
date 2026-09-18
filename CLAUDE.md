# CLAUDE.md

## Karpathy Guidelines

Behavioral guidelines to reduce common LLM coding mistakes, derived from
[Andrej Karpathy's observations](https://x.com/karpathy/status/2015883857489522876)
on LLM coding pitfalls.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

### 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

### 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

### 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

## Shell Scripts

All shell scripts in this repository (`test/`, `tests/`, and anywhere else) must be
**Bourne-shell compatible** - no bashisms. Concretely:

- Shebang: `#!/bin/sh`, not `#!/bin/bash` or `#!/usr/bin/env bash`.
- No arrays, `[[ ... ]]`, `<<<` here-strings, `$'...'` ANSI-C quoting, `coproc`,
  `read -t`/`read -u`, `${BASH_SOURCE[0]}`, `declare -a`, or process substitution `<(...)`.
- `local` inside functions is fine (supported by dash and ash-family shells even
  though it's not strictly POSIX).
- Prefer named pipes (`mkfifo`, `<>` redirection) over coprocesses when a script needs
  bidirectional I/O with a child process.
- Before considering a shell script done, verify it with **both**:
  - `dash -n script.sh` (syntax check) and an actual `dash script.sh` run where feasible
  - `shish -n script.sh` (syntax check) and an actual `shish script.sh` run where feasible

## No npm/yarn Projects

Do not add npm- or yarn-based projects to this repository (vendored or otherwise) -
no `package.json`-driven build steps, no Node.js as a build-time dependency. This
applies even to otherwise-reasonable integrations (e.g. vendoring a JS web client)
that would normally pull in a JS toolchain to build static assets.

## GitHub Pages site

This project's GitHub Pages site (the `gh-pages` branch) is **generated, not
hand-maintained here**. Use the global `github-pages` skill and the shared site
build tool in the `rsenn/rsenn` repo, at `../rsenn` (relative to this repo root;
i.e. `~/Projects/rsenn/rsenn`):

- site definition, landing page, theme, favicon: `../rsenn/sites/chaosircd/`
- generator and publisher: `../rsenn/tools/site/` (see its `README.md`)
  - build: `qjsm ../rsenn/tools/site/build.js chaosircd` (`node` works too)
  - publish: `../rsenn/tools/site/sync.sh chaosircd` (commits locally; `--push` only after the user confirms)
- the markdown that becomes the site's pages is **this repo's own** `README.md`,
  `doc/` and `examples/`; a doc page appears on the site only once it is listed in
  `nav` in `../rsenn/sites/chaosircd/site.config.js`.

Do not add or extend a `tools/site/`, Pages workflow or `publish.sh` in this repo (any
existing ones are superseded and slated for removal), and do
not edit `gh-pages` by hand.
