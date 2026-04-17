# CSS Variables Contract

`CircuitTrace` reads four CSS custom properties at runtime. If the target project already defines them (even under different semantics), the component will just work. If any are missing, add them to `:root` and `.dark`.

## Required variables

| Variable          | Used for                                             |
| ----------------- | ---------------------------------------------------- |
| `--brand-primary` | Default trace / pulse / node color                   |
| `--brand-accent`  | Accent trace / pulse / node color (~45% of traces)   |
| `--bg`            | Inner square of solder-pad nodes (makes them "inset") |
| `--fg-subtle`     | Dot grid pattern (only used by the `hero` variant)   |

## Default palette (copy into `globals.css` if the project has no tokens yet)

Tested in the fraga-tech project — navy-blue + emerald-green combo that matches the "Architectural Blueprint" north star.

```css
:root {
  --brand-primary: #087ee1;
  --brand-accent: #05e8ba;

  --bg: #f5f5f5;
  --fg-subtle: #99a1af;

  color-scheme: light;
}

.dark {
  --brand-primary: #087ee1;
  --brand-accent: #05e8ba;

  --bg: #0a0f1a;
  --fg-subtle: #64748b;

  color-scheme: dark;
}
```

## Tailwind v4 registration (optional but recommended)

If the project uses Tailwind v4 with `@theme`, also expose the brand colors as utilities so the rest of the site can use them consistently:

```css
@theme {
  --color-brand-primary: #087ee1;
  --color-brand-accent: #05e8ba;
}
```

This lets you write `bg-brand-primary`, `text-brand-accent`, etc.

## Aliasing an existing design system

If the project already has tokens like `--color-primary` or `--accent`, **do not overwrite them**. Add the four CircuitTrace variables as aliases:

```css
:root {
  --brand-primary: var(--color-primary);
  --brand-accent: var(--color-accent);
  /* --bg, --fg-subtle likely already exist — keep them */
}
```

## Dark mode activation

The skill assumes `.dark` class on `<html>` toggles dark mode (matches the Next.js + `next-themes` convention and the fraga-tech setup). If the project uses `prefers-color-scheme` only, move the dark overrides inside a `@media (prefers-color-scheme: dark)` block.
