# Integration Guide — Canonical Placements

Proven placements from the fraga-tech project. Follow these first; deviate only when the page structure genuinely differs.

Every placement follows the same pattern: a wrapper `div` with `pointer-events-none absolute` + size/position classes, and the parent section has `relative overflow-hidden`.

---

## Hero (full-viewport, one per page)

```tsx
import { CircuitTrace } from "@/components/CircuitTrace";

<section className="relative flex min-h-screen items-center justify-center overflow-hidden bg-[color:var(--bg)] px-6 pt-28 pb-20 lg:px-16">
  <CircuitTrace variant="hero" seed={11} intensity="normal" />

  <div
    aria-hidden="true"
    className="pointer-events-none absolute inset-0"
  >
    {/* Gradient fade into page bg at the bottom — keeps text legible */}
    <div className="absolute inset-0 bg-gradient-to-b from-transparent via-[color:var(--bg)]/10 to-[color:var(--bg)]" />
  </div>

  {/* hero content with z-10 goes here */}
</section>
```

Notes:
- The `hero` variant already renders its own dot grid + radial fade mask. Do not add `.grid-lines` on top.
- Keep headline content in a `relative z-10` wrapper.

---

## About (right-edge corner, desktop only)

```tsx
<section className="relative overflow-hidden bg-[color:var(--bg)] px-6 py-24 lg:px-16 lg:py-32">
  <div className="pointer-events-none absolute -right-24 top-1/2 hidden h-[520px] w-[520px] -translate-y-1/2 lg:block">
    <CircuitTrace variant="corner" seed={41} intensity="subtle" />
  </div>

  <div className="relative mx-auto max-w-7xl ..."> {/* content */} </div>
</section>
```

---

## Services (top edge strip)

```tsx
<section className="relative overflow-hidden bg-[color:var(--surface-muted)] px-6 py-24 lg:px-16 lg:py-32">
  <div className="pointer-events-none absolute inset-x-0 top-0 h-48">
    <CircuitTrace variant="edge" seed={23} intensity="subtle" />
  </div>

  <div className="relative mx-auto max-w-7xl"> {/* content */} </div>
</section>
```

---

## Cases (left corner + bottom edge)

```tsx
<section className="relative overflow-hidden bg-[color:var(--bg)] px-6 py-24 lg:px-16 lg:py-32">
  <div className="pointer-events-none absolute -left-24 top-24 hidden h-[520px] w-[520px] lg:block">
    <CircuitTrace variant="corner" seed={53} intensity="subtle" />
  </div>
  <div className="pointer-events-none absolute inset-x-0 bottom-0 h-40">
    <CircuitTrace variant="edge" seed={97} intensity="subtle" />
  </div>

  <div className="relative mx-auto max-w-7xl"> {/* content */} </div>
</section>
```

---

## Contact (two corners — replaces any blur-blob decoration)

```tsx
<section className="relative overflow-hidden bg-[color:var(--surface-muted)] px-6 py-24 lg:px-16 lg:py-32">
  <div className="pointer-events-none absolute -right-20 -top-20 h-[520px] w-[520px]">
    <CircuitTrace variant="corner" seed={67} intensity="normal" />
  </div>
  <div className="pointer-events-none absolute -bottom-20 -left-20 h-[520px] w-[520px] rotate-180">
    <CircuitTrace variant="corner" seed={89} intensity="subtle" />
  </div>

  <div className="relative z-10 mx-auto max-w-3xl"> {/* form */} </div>
</section>
```

Rotating the second corner by 180° gives visual variation without needing a second seed family.

---

## Seed hygiene

Use a distinct seed per placement so no two CircuitTrace instances on the page look identical. Canonical seeds used in fraga-tech:

| Placement           | Seed |
| ------------------- | ---- |
| Hero                | 11   |
| Services top edge   | 23   |
| About right corner  | 41   |
| Cases left corner   | 53   |
| Cases bottom edge   | 97   |
| Contact top-right   | 67   |
| Contact bottom-left | 89   |

Reusing the same seed across two sections is acceptable only if the variants differ (e.g., same seed for `corner` and `edge` will produce different layouts).

---

## Intensity ladder

- `subtle` — decoration under dense text / cards. Default for all non-hero placements.
- `normal` — used on hero and on the "featured" corner of the Contact section.
- `bold` — reserve for full-bleed backdrop sections with minimal text on top.

---

## Anti-patterns

- **Do not** place `CircuitTrace` inside a card. The effect is a section-level backdrop.
- **Do not** nest two `CircuitTrace` instances in the same container — they compound and muddy.
- **Do not** combine with the legacy blur-blob pattern (`absolute rounded-full bg-[color:var(--brand-primary)]/10 blur-[120px]`). Replace blobs with `corner` variants.
- **Do not** put a `CircuitTrace` inside an element that doesn't have `overflow-hidden` — the generated paths extend past the viewBox on purpose.
