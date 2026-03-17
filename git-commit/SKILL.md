---
name: git-commit
description: 'Execute git commit with conventional commit message analysis, intelligent staging, and message generation. Use when user asks to commit changes, create a git commit, or mentions "/commit". Supports: (1) Auto-detecting type and scope from changes, (2) Generating conventional commit messages from diff, (3) Interactive commit with optional type/scope/description overrides, (4) Intelligent file staging for logical grouping'
license: MIT
allowed-tools: Bash
---

# Git Commit with Conventional Commits

## Overview

Create standardized, semantic git commits using the Conventional Commits specification. Analyze the actual diff to determine appropriate type, scope, and message.

## Conventional Commit Format

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

## Commit Types

| Type       | Purpose                        |
| ---------- | ------------------------------ |
| `feat`     | New feature                    |
| `fix`      | Bug fix                        |
| `docs`     | Documentation only             |
| `style`    | Formatting/style (no logic)    |
| `refactor` | Code refactor (no feature/fix) |
| `perf`     | Performance improvement        |
| `test`     | Add/update tests               |
| `build`    | Build system/dependencies      |
| `ci`       | CI/config changes              |
| `chore`    | Maintenance/misc               |
| `revert`   | Revert commit                  |

## Emoji Patterns 💈

<table>

<thead>
<tr>
<th>Commit Type</th>
<th>Emoji</th>
<th>Keyword</th>
</tr>
</thead>
<tbody>
<tr>
<td>Accessibility</td>
<td>♿ <code>:wheelchair:</code></td>
<td></td>
</tr>
<tr>
<td>Adding a test</td>
<td>✅ <code>:white_check_mark:</code></td>
<td><code>test</code></td>
</tr>
<tr>
<td>Updating a submodule version</td>
<td>⬆️ <code>:arrow_up:</code></td>
<td></td>
</tr>
<tr>
<td>Dropping back a submodule</td>
<td>⬇️ <code>:arrow_down:</code></td>
<td></td>
</tr>
<tr>
<td>Adding a dependency</td>
<td>➕ <code>:heavy_plus_sign:</code></td>
<td><code>build</code></td>
</tr>
<tr>
<td>Code review changes</td>
<td>👌 <code>:ok_hand:</code></td>
<td><code>style</code></td>
</tr>
<tr>
<td>Animations and transitions</td>
<td>💫 <code>:dizzy:</code></td>
<td></td>
</tr>
<tr>
<td>Bugfix</td>
<td>🐛 <code>:bug:</code></td>
<td><code>fix</code></td>
</tr>
<tr>
<td>Comments</td>
<td>💡 <code>:bulb:</code></td>
<td><code>docs</code></td>
</tr>
<tr>
<td>Initial commit</td>
<td>🎉 <code>:tada:</code></td>
<td><code>init</code></td>
</tr>
<tr>
<td>Configuration</td>
<td>🔧 <code>:wrench:</code></td>
<td><code>cry</code></td>
</tr>
<tr>
<td>Deploy</td>
<td>🚀 <code>:rocket:</code></td>
<td></td>
</tr>
<tr>
<td>Documentation</td>
<td>📚 <code>:books:</code></td>
<td><code>docs</code></td>
</tr>
<tr>
<td>In progress</td>
<td>🚧 <code>:construction:</code></td>
<td></td>
</tr>
<tr>
<td>Interface styling</td>
<td>💄 <code>:lipstick:</code></td>
<td><code>feat</code></td>
</tr>
<tr>
<td>Infrastructure</td>
<td>🧱 <code>:bricks:</code></td>
<td><code>ci</code></td>
</tr>
<tr>
<td>List of ideas (tasks)</td>
<td>🔜 <code> :soon: </code></td>
<td></td>
</tr>
<tr>
<td>Move/Rename</td>
<td>🚚 <code>:truck:</code></td>
<td><code>cry</code></td>
</tr>
<tr>
<td>New feature</td>
<td>✨ <code>:sparkles:</code></td>
<td><code>feat</code></td>
</tr>
<tr>
<td>Package.json in JS</td>
<td>📦 <code>:package:</code></td>
<td><code>build</code></td>
</tr>
<tr>
<td>Performance</td>
<td>⚡ <code>:zap:</code></td>
<td><code>perf</code></td>
</tr>
<tr>
<td>Refactoring</td>
<td>♻️ <code>:recycle:</code></td>
<td><code>refactor</code></td>
</tr>
<tr>
<td>Code Cleaning</td>
<td>🧹 <code>:broom:</code></td>
<td><code>cleanup</code></td>
</tr>
<tr>
<td>Removing a file</td>
<td>🗑️ <code>:wastebasket:</code></td>
<td><code>remove</code></td>
</tr>
<tr>
<td>Removing a dependency</td>
<td>➖ <code>:heavy_minus_sign:</code></td>
<td><code>build</code></td>
</tr>
<tr>
<td>Responsiveness</td>
<td>📱 <code>:iphone:</code></td>
<td></td>
</tr>
<tr>
<td>Reverting changes</td>
<td>💥 <code>:boom:</code></td>
<td><code>fix</code></td>
</tr>
<tr>
<td>Security</td>
<td>🔒️ <code>:lock:</code></td>
<td></td>
</tr>
<tr>
<td>SEO</td>
<td>🔍️ <code>:mag:</code></td>
<td></td>
</tr>
<tr>
<td>Tag of version</td>
<td>🔖 <code>:bookmark:</code></td>
<td></td>
</tr>
<tr>
<td>Approval test</td>
<td>✔️ <code>:heavy_check_mark:</code></td>
<td><code>test</code></td>
</tr>
<tr>
<td>Tests</td>
<td>🧪 <code>:test_tube:</code></td>
<td><code>test</code></td>
</tr>
<tr>
<td>Text</td>
<td>📝 <code>:pencil:</code></td>
<td></td>
</tr>
<tr>
<td>Typing</td>
<td>🏷️ <code>:label:</code></td>
<td></td>
</tr>
<tr>
<td>Error handling</td>
<td>🥅 <code>:goal_net:</code></td>
<td></td>
</tr>
<tr>
<td>Data</td>
<td>🗃️ <code>:card_file_box:</code></td>
<td><code>raw</code></td>
</tr>
</tbody>
</table>


## 💻 Examples

<table>
<thead>
<tr>
<th>Git command</th>

<th>Result on GitHub</th>
</tr>
</thead>
<tbody>
<tr>
<td>
<code>git commit -m ":tada: Initial commit"</code>
</td>
<td>🎉 Initial commit</td>
</tr>
<tr>
<td>
<code>git commit-m ":books: docs: README Update"</code>
</td>
<td>📚 docs: README Update</td>
</tr>
<tr>
<td>
<code>git commit -m ":bug: fix: Infinite loop on line 50"</code>
</td>
<td>🐛 fix: Infinite loop on line 50</td>
</tr>
<tr>
<td>
<code>git commit -m ":sparkles: feat: Login Page"</code>
</td>
<td>✨ feat: Login Page</td>
</tr>
<tr>
<td>
<code>git commit -m ":bricks: ci: Dockerfile Modification"</code>
</td>
<td>🧱 ci: Dockerfile modification</td>
</tr>
<tr>
<tr>
<td>
<code>git commit -m ":recycle: refactor: Passing to arrow functions"</code>
</td>
<td>♻️ refactor: Passing to arrow functions</td>
</tr>
<tr>
<td>
<code>git commit -m ":zap: perf: Response time improvement"</code>
</td>
<td>⚡ perf: Response time improvement</td>
</tr>
<tr>
<tr>
<td>
<code>git commit -m ":boom: fix: Reverting inefficient changes"</code>
</td>
<td>💥 fix: Reverting inefficient changes</td>
</tr>
<tr>
<td>
<code>git commit -m ":lipstick: feat: CSS styling of the form"</code>
</td>
<td>💄 feat: CSS styling of the form</td>
</tr>
<tr>
<td>
<code>git commit -m ":test_tube: test: Creating a new test"</code>
</td>
<td>🧪 test: Creating a new test</td>
</tr>
<tr>
<td>
<code>git commit -m ":bulb: docs: Comments on the LoremIpsum() function"</code>
</td>
<td>💡 docs: Comments on the LoremIpsum() function</td>
</tr>
<tr>
<td>
<code>git commit -m ":card_file_box: raw: RAW Date of the year yyyy"</code>
</td>
<td>🗃️ raw: RAW Date of the year yyyy</td>
</tr>
<tr>
<td>
<code>git commit -m ":broom: cleanup: Removing commented-out code blocks and unused variables in the form validation function"</code>
</td>
<td>🧹 cleanup: Removing commented-out code blocks and unused variables in the form validation function</td>
</tr>
<tr>
<td>
<code>git commit -m ":wastebasket: remove: Removing unused files from the project to maintain organization and continuous updates"</code>
</td>
<td>🗑️ remove: Removing Unused project files to maintain organization and continuous updates</td>
</tr>
</tbody>
</table>
## Breaking Changes

```
# Exclamation mark after type/scope
feat!: Remove deprecated endpoint

# BREAKING CHANGE footer
feat: Allow config to extend other configs

BREAKING CHANGE: `extends` key behavior changed
```

## Workflow

### 1. Analyze Diff

```bash
# If files are staged, use staged diff
git diff --staged

# If nothing staged, use working tree diff
git diff

# Also check status
git status --porcelain
```

### 2. Stage Files (if needed)

If nothing is staged or you want to group changes differently:

```bash
# Stage specific files
git add path/to/file1 path/to/file2

# Stage by pattern
git add *.test.*
git add src/components/*

# Interactive staging
git add -p
```

**Never commit secrets** (.env, credentials.json, private keys).

### 3. Generate Commit Message

Analyze the diff to determine:

- **Type**: What kind of change is this?
- **Scope**: What area/module is affected?
- **Description**: One-line summary of what changed (present tense, imperative mood, <72 chars)

### 4. Execute Commit

```bash
# Single line
git commit -m "<type>[scope]: <description>"

# Multi-line with body/footer
git commit -m "$(cat <<'EOF'
<type>[scope]: <description>

<optional body>

<optional footer>
EOF
)"
```

## Best Practices

- Be brief, concise and descriptive
- One logical change per commit
- Present tense: "add" not "added"
- Imperative mood: "fix bug" not "fixes bug"
- Reference issues: `Closes #123`, `Refs #456`
- Keep description under 72 characters

## Git Safety Protocol

- NEVER update git config
- NEVER run destructive commands (--force, hard reset) without explicit request
- NEVER skip hooks (--no-verify) unless user asks
- NEVER force push to main/master
- If commit fails due to hooks, fix and create NEW commit (don't amend)
