---
name: circuit-trace-identity
description: Apply Pedro's "Circuit Trace / Architectural Blueprint" visual identity to a website — SVG PCB-style animated background (CircuitTrace component) plus optional Hero auxiliary effects (aurora glow, corner brackets, telemetry markers, terminal cursor). Use when building a new consultancy, tech, portfolio, SaaS, or engineering site, or when the user asks to apply "my usual theme", "minha identidade visual", "circuit trace", or "blueprint" aesthetic.
---

# Circuit Trace Identity

Pedro's reusable visual identity system for technology / consultancy / portfolio websites. The core is a **PCB-inspired SVG background** (`CircuitTrace`) aligned with the "Architectural Blueprint" design north star — orthogonal paths with rounded corners, animated signal pulses, and solder-pad nodes. Optional Hero-specific effects layer on top: drifting aurora glow, CAD-style corner brackets, monospace telemetry markers, and a terminal cursor blink after the gradient word.

Do **not** regenerate this system from scratch each time. Copy the canonical files in `assets/` into the target project and integrate per the placement guide.

---

## When to trigger

Apply this skill when the user:
- Asks to apply "minha identidade visual", "meu tema habitual", "circuit trace", "blueprint", or "PCB theme"
- Is starting a new consultancy / engineering / tech portfolio / SaaS landing page and wants the familiar aesthetic
- References "the fraga-tech look" or asks to bring over effects from a previous project

Do **not** apply when:
- The project already has a very different aesthetic (brutalist, playful, editorial magazine) unless the user explicitly asks to replace it
- A single small component is requested — the skill is a whole-site identity, not a one-off decoration

## Stack assumptions

Default target: **Next.js 15 / 16 App Router + React 18+ / 19 + Tailwind v4** (`@import "tailwindcss"` in `globals.css`, no `tailwind.config.js`).

The component is a client component (`"use client"`). It uses `useId` + `useMemo` only — no external deps beyond React.

If the target stack differs (Vite, plain React, Remix), the TSX still works; only the CSS import location changes.

---

## Integration steps

Always follow this order:

### 1. Verify / create CSS variables

The component reads these CSS custom properties at runtime:
- `--brand-primary` — main trace color
- `--brand-accent` — accent trace + node color
- `--bg` — node center fill (so nodes feel inset)
- `--fg-subtle` — dot grid color (hero variant only)

Open the project's `globals.css` (or equivalent). If any of the four is missing, add the defaults from [references/css-vars-contract.md](references/css-vars-contract.md). Do **not** overwrite existing design tokens — only alias or add missing ones.

### 2. Copy the component

Copy [assets/CircuitTrace.tsx](assets/CircuitTrace.tsx) verbatim to the project's components directory. Typical destinations:
- `components/CircuitTrace.tsx`
- `app/_components/CircuitTrace.tsx`
- `src/components/CircuitTrace.tsx`

Do not refactor, rename, or split the file.

### 3. Append the animation CSS

Append the contents of [assets/circuit-trace.css](assets/circuit-trace.css) to the project's `globals.css` (Tailwind v4) or import it as a separate stylesheet. This registers `.circuit-pulse` and `@keyframes circuit-pulse` with a `prefers-reduced-motion` fallback.

### 4. Place variants across sections

Use the canonical placements from [references/integration-guide.md](references/integration-guide.md). In short:

| Variant  | Viewport     | Where to use                                     | Intensity default |
| -------- | ------------ | ------------------------------------------------ | ----------------- |
| `hero`   | 1600×900     | Full-bleed Hero / landing cover                  | `normal`          |
| `edge`   | 1600×220     | Top or bottom strip of a section                 | `subtle`          |
| `corner` | 520×520      | Decorative corner block on a section             | `subtle` / `normal` |
| `panel`  | 800×500      | Inside a card or feature block                   | `subtle`          |

Wrap every instance in a `pointer-events-none absolute` container with the desired size and position — the component fills its parent. Always give the parent `overflow-hidden`.

### 5. (Optional) Add Hero auxiliary effects

If the project has a Hero section and the user wants the full identity, layer the effects from [references/hero-effects.md](references/hero-effects.md):
- Aurora glow (two drifting blobs behind the headline)
- Corner brackets (4 L-shapes framing the viewport)
- Telemetry markers (mono labels in 3 corners, with pulsing dot on SYS · ONLINE)
- Terminal cursor after the gradient word in the H1

Each block is copy-paste JSX + matching CSS keyframes. All respect `prefers-reduced-motion`.

---

## Props reference

```tsx
<CircuitTrace
  variant="hero" | "edge" | "corner" | "panel"  // default: "hero"
  seed={number}                                  // default: 7 — deterministic layout
  animated={boolean}                             // default: true
  intensity="subtle" | "normal" | "bold"         // default: "normal"
  className={string}                             // extra classes on the wrapper
/>
```

- Change `seed` per placement so different corners/edges in the same page don't look identical.
- Lower `intensity` (`subtle`) on sections with dense content; raise (`bold`) only on dedicated backdrop sections.
- `animated={false}` for screenshot / print contexts.

## Accessibility

- Root `<div>` has `aria-hidden="true"` — the component is purely decorative.
- Pulses, aurora, and cursor all freeze under `prefers-reduced-motion: reduce`.
- No focus targets, no keyboard interaction, no text content.

## Rules

**Do**
- Use the `hero` variant exactly once per page (typically the Hero).
- Give every placement a unique `seed` to avoid repetition.
- Pair corner placements with a soft gradient fade when they sit behind text.

**Don't**
- Stack multiple `hero` variants — they collide visually.
- Mix with the old "blur blob" / `radial-gradient` background pattern; replace those placements with `corner` variants instead.
- Override the radial fade mask on the `hero` variant — it's what keeps the text readable.
- Add 1px borders or grid lines as separators; use background color shifts (matches the parent design system this identity was built for).
