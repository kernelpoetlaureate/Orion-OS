# Contributing to Orion OS

Thank you for your interest in contributing to Orion OS. This document explains the project's expectations, conventions, and the recommended workflow for contributions.

## High-level process
1. Open an issue describing the proposed change or feature. Link design notes where applicable.
2. Create a focused branch named `feat/<short-desc>` or `fix/<short-desc>`.
3. Draft or update a design document under `docs/` for any non-trivial subsystem changes.
4. Open a pull request targeting `main`. Keep PRs small and focused.
5. Include tests and documentation for any new behavior.

## Repo conventions (summary)
- Directory layout follows the `kernel/` structure: `arch/x86_64/`, `core/`, `drivers/`, `lib/`.
- Public kernel modules expose a single init function:

  `int module_init(void);
`

  Return `0` on success; non-zero for failure.
- Prefix arch-specific symbols with `arch_x86_` (e.g., `arch_x86_init_tables`).
- Panic path signature:

  `void panic(const char *fmt, ...);
`

  Panic should not return.

## Coding and PR expectations
- Provide a design doc or a design note link for changes touching architecture or invariants.
- Keep commits small and logically grouped.
- Include test coverage for host-side, pure userspace logic (see `tests/`).
- Do not commit large toolchain artifacts; add them to `.gitignore`.

## License and CLA
By contributing, you agree your contributions are covered under the repository's license. If you require a Contributor License Agreement (CLA), we will add one and document it here.

## Contact
Open issues or raise discussion PRs for larger design work.
