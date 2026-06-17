---
name: git-commit
description: Prepare a Hermes commit with required review and verification. Use when creating commits or when the user asks to commit changes.
---

# Hermes Git Commit Workflow

## Goals

1. Review the staged or unstaged change set.
2. Run the appropriate local verification.
3. Create a focused commit message that follows the repository Lore protocol.

## Workflow

1. Check changed files with:

   ```bash
   git diff --name-only
   git diff --cached --name-only
   ```

2. Run `code-review` for every commit-worthy change set.
3. Run `testing` when code or executable model behavior changed.
4. Stage only the intended files.
5. Review the staged diff:

   ```bash
   git diff --cached
   ```

6. Commit using the Lore format from `AGENTS.md`.

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

