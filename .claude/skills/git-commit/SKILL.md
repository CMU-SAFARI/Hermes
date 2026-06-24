---
name: git-commit
description: Prepare a Hermes commit with required review and verification. Use when creating commits or when the user asks to commit changes.
---

# Hermes Git Commit Workflow

## Goals

1. Ensure the change conforms to the codebase formatting norm.
2. Review the staged or unstaged change set.
3. Run the appropriate local verification.
4. Create a focused commit message that follows the repository Lore protocol.

## Workflow

1. Check changed files with:

   ```bash
   git diff --name-only
   git diff --cached --name-only
   ```

2. **Enforce formatting before anything else.** Run the bundled script to bring
   every changed C++ file up to the repo's `.clang-format`:

   ```bash
   bash "$(git rev-parse --show-toplevel)/.claude/skills/git-commit/format.sh" --fix
   ```

   It reformats only the files that differ from `HEAD` (staged, unstaged, and new
   untracked), re-running clang-format to a fixed point. It no-ops cleanly if the
   repo has no `.clang-format`, and errors (exit 3) if a `.clang-format` exists but
   `clang-format` is not installed — install it rather than skipping. Run without
   `--fix` for a non-mutating check (exit 1 lists offenders); add `--all` to sweep
   the whole tree. Any files it rewrites must be (re-)staged in step 5 so the
   formatting lands in this commit.

3. Run `code-review` for every commit-worthy change set.
4. Run `testing` when code or executable model behavior changed.
5. Stage only the intended files (including anything the formatter rewrote).
6. Review the staged diff:

   ```bash
   git diff --cached
   ```

7. Commit using the Lore format from `AGENTS.md`.

## Commit Rules

- Keep one coherent change per commit.
- Do not stage generated artifacts, build output, or local machine state.
- Do not add AI co-author lines.
- Record external constraints and rejected alternatives when they mattered.

## Message Shape

```text
<intent line>

<short rationale body>

Constraint: ...
Rejected: ... | ...
Confidence: low|medium|high
Scope-risk: narrow|moderate|broad
Directive: ...
Tested: ...
Not-tested: ...
```

