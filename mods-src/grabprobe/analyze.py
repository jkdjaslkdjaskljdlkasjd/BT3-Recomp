#!/usr/bin/env python3
"""Analyze a grabprobe_NNN.bin recording: find candidate grab-state, grab-timer and HP fields.

    python3 analyze.py grabprobe_000.bin [more.bin ...]

For every 4-byte-aligned offset in each fighter it reports:
  * STATE candidates  - an integer that is stable, changes to another value for a stretch of frames and
                        comes back (the "I'm grabbing / I'm being grabbed" flags);
  * TIMER candidates  - a value that counts steadily up or down during that stretch (the grab's duration);
  * HP candidates     - a float/int that only ever goes down, in steps (the victim's life bar).
The stretch the most fields agree on is printed first: that is almost certainly the grab.
"""
import struct, sys
from collections import Counter

def load(path):
    with open(path, 'rb') as f:
        magic, ver, n, size = struct.unpack('<4I', f.read(16))
        if magic != 0x50425247:
            sys.exit(f'{path}: not a grabprobe file')
        frames = []
        for _ in range(n):
            fr, p0, p1, a0, a1 = struct.unpack('<Q4I', f.read(24))
            m0 = f.read(size); m1 = f.read(size)
            frames.append((fr, (p0, p1), (a0, a1), (m0, m1)))
    return size, frames

def series(frames, who, off, fmt):
    return [struct.unpack_from(fmt, fr[3][who], off)[0] for fr in frames]

def runs(vals):
    """Return list of (start, end_exclusive, value) runs."""
    out, s = [], 0
    for i in range(1, len(vals) + 1):
        if i == len(vals) or vals[i] != vals[s]:
            out.append((s, i, vals[s])); s = i
    return out

def analyze(path):
    size, frames = load(path)
    n = len(frames)
    print(f'=== {path}: {n} frames, fighters at 0x{frames[-1][2][0]:x} / 0x{frames[-1][2][1]:x}')
    windows = Counter(); state, timer, hp = [], [], []
    for who in (0, 1):
        for off in range(0, size, 4):
            iv = series(frames, who, off, '<I')
            r = runs(iv)
            # STATE: base value, a few short excursions, back to base
            if 3 <= len(r) <= 9 and r[0][2] == r[-1][2] and r[0][1] - r[0][0] > 15 and r[-1][1] - r[-1][0] > 15:
                inner = (r[1][0], r[-2][1])
                if 10 <= inner[1] - inner[0] <= n - 30 and all(v < 0x10000 for *_, v in r):
                    state.append((who, off, inner, [v for *_, v in r]))
                    windows[(inner[0] // 5, inner[1] // 5)] += 1
            # TIMER: monotone +-1 steps for a stretch of >= 20 frames
            best = cur = 0
            for i in range(1, n):
                d = (iv[i] - iv[i - 1]) & 0xFFFFFFFF
                cur = cur + 1 if d in (1, 0xFFFFFFFF) else 0
                best = max(best, cur)
            if best >= 20 and len(r) < n * 0.9:
                timer.append((who, off, best))
            # HP: float that never rises and drops at least twice
            fv = series(frames, who, off, '<f')
            if all(v == v and 1.0 <= v <= 1e6 for v in fv):   # finite, life-like
                drops = sum(1 for i in range(1, n) if fv[i] < fv[i - 1])
                rises = sum(1 for i in range(1, n) if fv[i] > fv[i - 1])
                if drops >= 2 and rises == 0:
                    hp.append((who, off, fv[0], fv[-1], drops))
    if windows:
        (a, b), votes = windows.most_common(1)[0]
        print(f'Most likely grab window: frames ~{a*5}..{b*5} ({votes} fields agree)')
    print(f'\nSTATE candidates ({len(state)}):')
    for who, off, (a, b), vals in sorted(state, key=lambda t: t[2])[:60]:
        print(f'  P{who+1} +0x{off:04x}  frames {a}-{b}  values {vals}')
    print(f'\nTIMER candidates ({len(timer)}):')
    for who, off, best in sorted(timer, key=lambda t: -t[2])[:30]:
        print(f'  P{who+1} +0x{off:04x}  counts steadily for {best} frames')
    print(f'\nHP candidates ({len(hp)}):')
    for who, off, a, b, d in hp[:30]:
        print(f'  P{who+1} +0x{off:04x}  {a:.1f} -> {b:.1f}  ({d} drops)')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for p in sys.argv[1:]:
        analyze(p)
