---
name: burgundy-warm-theme
description: Apply burgundy + warm brown theme to web projects. Use when building landing pages, portfolios, link-in-bio, or clinic/professional sites needing an elegant, warm, feminine-professional aesthetic. Provides Tailwind v4 theme tokens, palette, fonts (Plus Jakarta Sans + Cormorant), gradients, shadows, button styles, and CSS patterns.
---

# Burgundy Warm Theme

Elegant warm theme extracted from the Dra. Mikaelly portfolio. Use for professional, feminine, sophisticated sites (clinics, portfolios, link-in-bio, small business).

## Color Palette

| Token | Hex | Use |
|-------|-----|-----|
| `primary` | `#622F32` | Burgundy — headings, primary buttons, accents |
| `secondary` | `#9A5B34` | Warm brown — secondary text, labels |
| `accent` | `#A47150` | Copper accent — highlights |
| `surface` | `#BD9D8C` | Muted beige — borders, dots, subtle dividers |
| `background-light` | `#FCFAF8` | Page background (light) |
| `background-dark` | `#2A1617` | Deep wine — dark text, dark mode bg |

## Fonts

- **Body/UI:** `Plus Jakarta Sans` (via `next/font`)
- **Display/Headings:** `Cormorant` serif (Georgia fallback)

## Tailwind v4 Setup (`globals.css`)

```css
@import "tailwindcss";
@import "tw-animate-css";

@theme {
  --color-primary: #622F32;
  --color-secondary: #9A5B34;
  --color-accent: #A47150;
  --color-surface: #BD9D8C;
  --color-background-light: #FCFAF8;
  --color-background-dark: #2A1617;
  --font-display: var(--font-plus-jakarta-sans), sans-serif;
}
```

## Next.js Font Setup (`app/layout.tsx`)

```tsx
import { Plus_Jakarta_Sans, Cormorant } from "next/font/google";

const jakarta = Plus_Jakarta_Sans({
  subsets: ["latin"],
  variable: "--font-plus-jakarta-sans",
});

const cormorant = Cormorant({
  subsets: ["latin"],
  variable: "--font-cormorant",
  weight: ["400", "500", "600"],
});

// apply: className={`${jakarta.variable} ${cormorant.variable}`}
```

## Signature Patterns

### Gradient (brand — burgundy → copper → beige)

```css
background: linear-gradient(135deg, #622F32 0%, #A47150 50%, #BD9D8C 100%);
```

### Subtle page ambience (radial gradients on `#FCFAF8`)

```css
background-color: #FCFAF8;
background-image:
  radial-gradient(ellipse 60% 40% at 15% 10%, rgba(189, 157, 140, 0.14) 0%, transparent 60%),
  radial-gradient(ellipse 50% 50% at 85% 85%, rgba(98, 47, 50, 0.08) 0%, transparent 60%);
```

### Ornament divider (line · dot · line)

```html
<div class="ornament">
  <span class="ornament-line"></span>
  <span class="ornament-dot"></span>
  <span class="ornament-line right"></span>
</div>
```

```css
.ornament { display: flex; align-items: center; gap: 0.6rem; justify-content: center; }
.ornament-line { height: 1px; width: 40px; background: linear-gradient(90deg, transparent, #BD9D8C80); }
.ornament-line.right { background: linear-gradient(90deg, #BD9D8C80, transparent); }
.ornament-dot { width: 5px; height: 5px; border-radius: 50%; background: #BD9D8C; }
```

### Primary button (burgundy gradient)

```css
background: linear-gradient(135deg, #622F32 0%, #9A5B34 100%);
color: #fff;
box-shadow: 0 4px 18px rgba(98, 47, 50, 0.22), 0 1px 3px rgba(0,0,0,0.08);
/* hover */
transform: translateY(-3px);
box-shadow: 0 10px 28px rgba(98, 47, 50, 0.32), 0 3px 8px rgba(0,0,0,0.1);
```

### Outline button

```css
background: transparent;
color: #622F32;
border: 1.5px solid #622F3240;
/* hover */
background: rgba(98, 47, 50, 0.04);
border-color: #622F3280;
```

### Glass card (desktop link-in-bio)

```css
background: rgba(252, 250, 248, 0.7);
backdrop-filter: blur(8px);
border: 1px solid rgba(189, 157, 140, 0.18);
border-radius: 2rem;
box-shadow: 0 20px 60px rgba(98, 47, 50, 0.08), 0 4px 20px rgba(0,0,0,0.04);
```

### Profile photo ring

```css
.photo-wrapper {
  padding: 3px;
  border-radius: 50%;
  background: linear-gradient(135deg, #622F32 0%, #A47150 50%, #BD9D8C 100%);
  box-shadow: 0 8px 30px rgba(98, 47, 50, 0.2);
}
```

## Animations

Use `fadeUp` with staggered delays `.d1`–`.d7` (0.05s → 0.78s, step ≈ 0.12s).

```css
@keyframes fadeUp {
  from { opacity: 0; transform: translateY(18px); }
  to   { opacity: 1; transform: translateY(0); }
}
.anim { opacity: 0; animation: fadeUp 0.55s cubic-bezier(0.22, 1, 0.36, 1) forwards; }
```

For richer motion, use `motion` library (scroll reveals) + `tw-animate-css`.

## Typography Rules

- Headings: Cormorant, weight 500, tight letter-spacing (`0.01em`)
- Uppercase labels/specialty text: `font-size: 0.68rem; font-weight: 600; color: #9A5B34; letter-spacing: 0.22em; text-transform: uppercase`
- Body: Plus Jakarta Sans, color `#2A1617`
- Muted captions: color `#BD9D8C`, small size `0.7rem`, letter-spacing `0.04em`

## Shadow Scale

```
soft:   0 2px 10px rgba(98,47,50,0.06)
medium: 0 4px 18px rgba(98,47,50,0.22)
strong: 0 10px 30px rgba(98,47,50,0.32)
glass:  0 20px 60px rgba(98,47,50,0.08)
```

## Border Radius Scale

- Buttons: `1rem`
- Cards: `2rem`
- Circular (photo, dots): `50%`

## Easing

Standard: `cubic-bezier(0.22, 1, 0.36, 1)` — refined ease-out used for all transitions.

## Usage Checklist

When asked to "apply this theme":
1. Add the `@theme` block to `globals.css` (Tailwind v4) or equivalent CSS vars.
2. Wire Plus Jakarta Sans + Cormorant via `next/font` (or `<link>` for non-Next).
3. Set page background to `#FCFAF8` with the subtle radial ambience.
4. Use `primary` for main CTAs with the burgundy gradient; `surface` for muted UI.
5. Headings in Cormorant; body/UI in Jakarta.
6. Apply `fadeUp` + `.d1..d7` stagger to above-the-fold content.
7. Separate sections with the ornament divider, not plain `<hr>`.
