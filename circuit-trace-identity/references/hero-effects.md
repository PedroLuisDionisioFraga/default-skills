# Hero Auxiliary Effects

Optional layer on top of `<CircuitTrace variant="hero" />`. All four effects compose; apply only the ones that fit.

All JSX here assumes it's inside the Hero `<section>` (which must be `relative overflow-hidden`). All CSS belongs in `globals.css` — append below the `.circuit-pulse` block from `assets/circuit-trace.css`.

---

## 1. Aurora glow (drifting color blobs behind the headline)

**JSX** — insert inside the same absolute overlay div that holds the bottom gradient fade:

```tsx
<div
  aria-hidden="true"
  className="pointer-events-none absolute inset-0"
>
  <div className="absolute left-1/2 top-1/2 h-[620px] w-[620px] -translate-x-1/2 -translate-y-1/2 rounded-full bg-[color:var(--brand-primary)]/18 blur-[140px] hero-aurora-a" />
  <div className="absolute left-[58%] top-[42%] h-[460px] w-[460px] -translate-x-1/2 -translate-y-1/2 rounded-full bg-[color:var(--brand-accent)]/14 blur-[130px] hero-aurora-b" />
  <div className="absolute inset-0 bg-gradient-to-b from-transparent via-[color:var(--bg)]/10 to-[color:var(--bg)]" />
</div>
```

**CSS**:

```css
.hero-aurora-a {
  animation: hero-aurora-drift-a 14s ease-in-out infinite;
}

.hero-aurora-b {
  animation: hero-aurora-drift-b 18s ease-in-out infinite;
}

@keyframes hero-aurora-drift-a {
  0%, 100% { transform: translate(-50%, -50%) scale(1); }
  50%      { transform: translate(-52%, -48%) scale(1.08); }
}

@keyframes hero-aurora-drift-b {
  0%, 100% { transform: translate(-50%, -50%) scale(1); }
  50%      { transform: translate(-46%, -54%) scale(0.92); }
}

@media (prefers-reduced-motion: reduce) {
  .hero-aurora-a,
  .hero-aurora-b {
    animation: none;
  }
}
```

Tune opacity by changing `/18` and `/14` in the Tailwind classes (roughly `0.18` and `0.14` alpha). If the brand colors are darker, raise to `/25` and `/18`.

---

## 2. Corner brackets (CAD-style frame)

**JSX** — top-level child of the Hero section, outside the content wrapper:

```tsx
<div
  aria-hidden="true"
  className="pointer-events-none absolute inset-6 z-[5] hidden md:block"
>
  <span className="absolute left-0 top-0 h-6 w-6 border-l border-t border-[color:var(--brand-primary)]/40" />
  <span className="absolute right-0 top-0 h-6 w-6 border-r border-t border-[color:var(--brand-primary)]/40" />
  <span className="absolute bottom-0 left-0 h-6 w-6 border-b border-l border-[color:var(--brand-primary)]/40" />
  <span className="absolute bottom-0 right-0 h-6 w-6 border-b border-r border-[color:var(--brand-primary)]/40" />
</div>
```

No CSS needed — pure Tailwind borders.

Hidden on mobile (`hidden md:block`) because at small widths the brackets feel cramped against safe-area padding.

---

## 3. Telemetry markers (mono labels with pulsing dot)

Three corners, each conveying a different signal. Add them as siblings of the content wrapper.

**Top-left — section marker**:

```tsx
<div
  aria-hidden="true"
  className="pointer-events-none absolute left-6 top-24 z-[5] hidden items-center gap-2 font-mono text-[10px] uppercase tracking-[0.25em] text-[color:var(--fg-subtle)] md:flex lg:left-16"
>
  <span className="h-px w-6 bg-[color:var(--fg-subtle)]/60" />
  <span>SEC · 01 / HERO</span>
</div>
```

**Top-right — system status (with pulsing dot)**:

```tsx
<div
  aria-hidden="true"
  className="pointer-events-none absolute right-6 top-24 z-[5] hidden items-center gap-2 font-mono text-[10px] uppercase tracking-[0.25em] text-[color:var(--fg-subtle)] md:flex lg:right-16"
>
  <span className="relative inline-flex h-1.5 w-1.5">
    <span className="absolute inset-0 animate-ping rounded-full bg-[color:var(--brand-accent)]/80" />
    <span className="relative inline-block h-1.5 w-1.5 rounded-full bg-[color:var(--brand-accent)]" />
  </span>
  <span>SYS · ONLINE</span>
  <span className="h-px w-6 bg-[color:var(--fg-subtle)]/60" />
</div>
```

**Bottom-right — location / version**:

```tsx
<div
  aria-hidden="true"
  className="pointer-events-none absolute bottom-10 right-6 z-[5] hidden flex-col items-end gap-1 font-mono text-[10px] uppercase tracking-[0.25em] text-[color:var(--fg-subtle)] md:flex lg:right-16"
>
  <span className="text-[color:var(--fg-subtle)]/80">
    LAT −22.906 / LNG −43.172
  </span>
  <span className="text-[color:var(--fg-subtle)]/60">
    BR · 2026 / v1.0
  </span>
</div>
```

Adjust copy to fit the project: swap `SEC · 01 / HERO` for the page's section id, the coords for actual location, the version for whatever cadence you ship.

No CSS needed — `animate-ping` is a Tailwind utility.

---

## 4. Terminal cursor (blinks after the gradient word)

**JSX** — replace the second `<span>` inside the H1 (the one with `bg-clip-text`) so the cursor sits right after the text:

```tsx
<h1 className="sr relative mb-8 font-headline text-5xl font-bold leading-[1.02] tracking-tight text-[color:var(--fg)] sm:text-6xl md:text-7xl lg:text-[5.25rem]">
  <span className="block">{titleLine1}</span>
  <span className="relative block bg-gradient-to-r from-[color:var(--brand-primary)] to-[color:var(--brand-accent)] bg-clip-text text-transparent">
    {titleLine2}
    <span
      aria-hidden="true"
      className="hero-cursor ml-1 inline-block h-[0.75em] w-[3px] translate-y-[0.1em] rounded-sm bg-[color:var(--brand-accent)] align-middle"
    />
  </span>
</h1>
```

**CSS**:

```css
.hero-cursor {
  animation: hero-cursor-blink 1.05s steps(2, end) infinite;
}

@keyframes hero-cursor-blink {
  0%, 50%   { opacity: 1; }
  51%, 100% { opacity: 0; }
}

@media (prefers-reduced-motion: reduce) {
  .hero-cursor {
    animation: none;
  }
}
```

`steps(2, end)` produces the hard on/off blink of a terminal cursor (no fade).

---

## Putting it all together

The full Hero wiring in fraga-tech uses all four:

1. `<CircuitTrace variant="hero" seed={11} intensity="normal" />`
2. Aurora wrapper (two blobs + bottom gradient fade)
3. Corner brackets wrapper
4. Three telemetry markers
5. Terminal cursor inside the H1 gradient span

Order in the JSX doesn't matter as long as content stays on `z-10` and the effects stay on `z-[5]` or lower.
