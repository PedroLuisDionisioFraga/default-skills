"use client";

import { useId, useMemo } from "react";

type Variant = "hero" | "edge" | "corner" | "panel";

interface Props {
  variant?: Variant;
  className?: string;
  seed?: number;
  animated?: boolean;
  intensity?: "subtle" | "normal" | "bold";
}

interface Move {
  dir: "R" | "L" | "U" | "D";
  len: number;
}

interface Trace {
  d: string;
  start: [number, number];
  end: [number, number];
  animated: boolean;
  accent: boolean;
  duration: number;
  delay: number;
}

function createRng(seed: number) {
  let s = seed | 0;
  return () => {
    s = (s + 0x6d2b79f5) | 0;
    let t = Math.imul(s ^ (s >>> 15), 1 | s);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

function buildTrace(
  sx: number,
  sy: number,
  moves: Move[],
  r: number
): { d: string; end: [number, number] } {
  let x = sx;
  let y = sy;
  let d = `M${x} ${y}`;
  for (let i = 0; i < moves.length; i++) {
    const m = moves[i];
    const n = moves[i + 1];
    const dx = m.dir === "R" ? 1 : m.dir === "L" ? -1 : 0;
    const dy = m.dir === "D" ? 1 : m.dir === "U" ? -1 : 0;
    if (n) {
      const ndx = n.dir === "R" ? 1 : n.dir === "L" ? -1 : 0;
      const ndy = n.dir === "D" ? 1 : n.dir === "U" ? -1 : 0;
      const straight = Math.max(0, m.len - r);
      x += dx * straight;
      y += dy * straight;
      d += ` L${x} ${y}`;
      const ex = x + dx * r + ndx * r;
      const ey = y + dy * r + ndy * r;
      const cross = dx * ndy - dy * ndx;
      const sweep = cross > 0 ? 1 : 0;
      d += ` A${r} ${r} 0 0 ${sweep} ${ex} ${ey}`;
      x = ex;
      y = ey;
    } else {
      x += dx * m.len;
      y += dy * m.len;
      d += ` L${x} ${y}`;
    }
  }
  return { d, end: [x, y] };
}

function generateTraces(
  width: number,
  height: number,
  count: number,
  seed: number,
  grid: number,
  cornerR: number
): Trace[] {
  const rnd = createRng(seed);
  const out: Trace[] = [];
  const cols = Math.floor(width / grid);
  const rows = Math.floor(height / grid);

  for (let i = 0; i < count; i++) {
    const edge = Math.floor(rnd() * 4);
    let sx = 0;
    let sy = 0;
    let firstDir: Move["dir"] = "R";
    if (edge === 0) {
      sx = 0;
      sy = (1 + Math.floor(rnd() * (rows - 2))) * grid;
      firstDir = "R";
    } else if (edge === 1) {
      sx = width;
      sy = (1 + Math.floor(rnd() * (rows - 2))) * grid;
      firstDir = "L";
    } else if (edge === 2) {
      sx = (1 + Math.floor(rnd() * (cols - 2))) * grid;
      sy = 0;
      firstDir = "D";
    } else {
      sx = (1 + Math.floor(rnd() * (cols - 2))) * grid;
      sy = height;
      firstDir = "U";
    }

    const segCount = 3 + Math.floor(rnd() * 4);
    const moves: Move[] = [];
    let prev = firstDir;
    for (let j = 0; j < segCount; j++) {
      let dir = prev;
      if (j > 0) {
        const horiz = prev === "R" || prev === "L";
        dir = horiz ? (rnd() < 0.5 ? "U" : "D") : (rnd() < 0.5 ? "L" : "R");
      }
      const len = (2 + Math.floor(rnd() * 5)) * grid;
      moves.push({ dir, len });
      prev = dir;
    }

    const t = buildTrace(sx, sy, moves, cornerR);
    out.push({
      d: t.d,
      start: [sx, sy],
      end: t.end,
      animated: i % 3 !== 0,
      accent: rnd() < 0.45,
      duration: 7 + rnd() * 6,
      delay: -rnd() * 10,
    });
  }

  return out;
}

function config(variant: Variant) {
  switch (variant) {
    case "hero":
      return { w: 1600, h: 900, count: 12, grid: 60, r: 14, showGrid: true };
    case "edge":
      return { w: 1600, h: 220, count: 6, grid: 44, r: 12, showGrid: false };
    case "corner":
      return { w: 520, h: 520, count: 5, grid: 48, r: 12, showGrid: false };
    case "panel":
      return { w: 800, h: 500, count: 6, grid: 50, r: 12, showGrid: false };
  }
}

export function CircuitTrace({
  variant = "hero",
  className,
  seed = 7,
  animated = true,
  intensity = "normal",
}: Props) {
  const uid = useId().replace(/[^a-zA-Z0-9]/g, "");
  const { w, h, count, grid, r, showGrid } = config(variant);

  const traces = useMemo(
    () => generateTraces(w, h, count, seed, grid, r),
    [w, h, count, seed, grid, r]
  );

  const nodes = useMemo(() => {
    const seen = new Set<string>();
    const pts: Array<{ x: number; y: number; accent: boolean }> = [];
    for (const t of traces) {
      const candidates: Array<[[number, number], boolean]> = [
        [t.start, t.accent],
        [t.end, t.accent],
      ];
      for (const [p, a] of candidates) {
        const inside = p[0] > 16 && p[0] < w - 16 && p[1] > 16 && p[1] < h - 16;
        if (!inside) continue;
        const key = `${p[0]},${p[1]}`;
        if (seen.has(key)) continue;
        seen.add(key);
        pts.push({ x: p[0], y: p[1], accent: a });
      }
    }
    return pts;
  }, [traces, w, h]);

  const intensityMap = {
    subtle: { base: 0.1, accent: 0.2, pulseOp: 0.7, nodeOp: 0.45 },
    normal: { base: 0.16, accent: 0.32, pulseOp: 0.9, nodeOp: 0.6 },
    bold: { base: 0.24, accent: 0.45, pulseOp: 1, nodeOp: 0.85 },
  }[intensity];

  const gridId = `ct-grid-${uid}`;
  const glowId = `ct-glow-${uid}`;
  const maskId = `ct-fade-${uid}`;

  return (
    <div
      aria-hidden="true"
      className={[
        "pointer-events-none absolute inset-0 overflow-hidden",
        className,
      ]
        .filter(Boolean)
        .join(" ")}
    >
      <svg
        viewBox={`0 0 ${w} ${h}`}
        preserveAspectRatio="xMidYMid slice"
        className="absolute inset-0 h-full w-full"
      >
        <defs>
          <pattern
            id={gridId}
            x="0"
            y="0"
            width={grid}
            height={grid}
            patternUnits="userSpaceOnUse"
          >
            <circle cx="1" cy="1" r="1" fill="var(--fg-subtle)" opacity="0.35" />
          </pattern>
          <filter id={glowId} x="-50%" y="-50%" width="200%" height="200%">
            <feGaussianBlur stdDeviation="2.5" result="blur" />
            <feMerge>
              <feMergeNode in="blur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
          <radialGradient id={maskId} cx="50%" cy="50%" r="70%">
            <stop offset="0%" stopColor="white" stopOpacity="1" />
            <stop offset="70%" stopColor="white" stopOpacity="0.85" />
            <stop offset="100%" stopColor="white" stopOpacity="0.15" />
          </radialGradient>
          <mask id={`ct-mask-${uid}`}>
            <rect
              width={w}
              height={h}
              fill={variant === "hero" ? `url(#${maskId})` : "white"}
            />
          </mask>
        </defs>

        <g mask={`url(#ct-mask-${uid})`}>
          {showGrid && (
            <rect width={w} height={h} fill={`url(#${gridId})`} opacity="0.5" />
          )}

          <g fill="none" strokeLinecap="round" strokeLinejoin="round">
            {traces.map((t, i) => (
              <path
                key={`s-${i}`}
                d={t.d}
                stroke={
                  t.accent ? "var(--brand-accent)" : "var(--brand-primary)"
                }
                strokeWidth="1"
                opacity={t.accent ? intensityMap.accent : intensityMap.base}
              />
            ))}
          </g>

          {animated && (
            <g
              fill="none"
              strokeLinecap="round"
              filter={`url(#${glowId})`}
              style={{ opacity: intensityMap.pulseOp }}
            >
              {traces
                .filter((t) => t.animated)
                .map((t, i) => (
                  <path
                    key={`p-${i}`}
                    d={t.d}
                    stroke={
                      t.accent
                        ? "var(--brand-accent)"
                        : "var(--brand-primary)"
                    }
                    strokeWidth="2"
                    strokeDasharray="70 2200"
                    className="circuit-pulse"
                    style={{
                      animationDuration: `${t.duration}s`,
                      animationDelay: `${t.delay}s`,
                    }}
                  />
                ))}
            </g>
          )}

          <g>
            {nodes.map((n, i) => (
              <g key={`n-${i}`} transform={`translate(${n.x} ${n.y})`}>
                <rect
                  x="-5"
                  y="-5"
                  width="10"
                  height="10"
                  rx="1.5"
                  fill={
                    n.accent ? "var(--brand-accent)" : "var(--brand-primary)"
                  }
                  opacity={intensityMap.nodeOp}
                />
                <rect
                  x="-2.25"
                  y="-2.25"
                  width="4.5"
                  height="4.5"
                  rx="0.75"
                  fill="var(--bg)"
                />
              </g>
            ))}
          </g>
        </g>
      </svg>
    </div>
  );
}