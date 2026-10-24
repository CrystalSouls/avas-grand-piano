#!/usr/bin/env python3
"""Generates the kawaii artwork (Assets/ava_girl.svg and Assets/background.svg).

Run it from the project folder:   python3 tools/make_art.py
The plugin embeds the two SVG files, so after changing the art just rebuild.
Only plain SVG features are used (paths, circles, ellipses, gradients) because
the plugin draws them with JUCE's built-in SVG reader.
"""
import math
import random
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "Assets"
OUT.mkdir(exist_ok=True)


# ----------------------------------------------------------------- helpers
def star5(cx, cy, ro, ri, rot=-90):
    pts = []
    for i in range(10):
        r = ro if i % 2 == 0 else ri
        a = math.radians(rot + i * 36)
        pts.append(f"{cx + r * math.cos(a):.1f},{cy + r * math.sin(a):.1f}")
    return " ".join(pts)


def sparkle(cx, cy, s, fill, op=1.0):
    k = s * 0.14
    return (f'<path d="M{cx:.1f} {cy - s:.1f} Q{cx + k:.1f} {cy - k:.1f} {cx + s:.1f} {cy:.1f} '
            f'Q{cx + k:.1f} {cy + k:.1f} {cx:.1f} {cy + s:.1f} Q{cx - k:.1f} {cy + k:.1f} {cx - s:.1f} {cy:.1f} '
            f'Q{cx - k:.1f} {cy - k:.1f} {cx:.1f} {cy - s:.1f} Z" fill="{fill}" fill-opacity="{op}"/>')


def heart(cx, cy, s, fill, op=1.0, rot=0):
    return (f'<path transform="translate({cx:.1f} {cy:.1f}) rotate({rot}) scale({s})" '
            f'd="M0 0.6 C-1.2 -0.2 -0.8 -1.2 0 -0.6 C0.8 -1.2 1.2 -0.2 0 0.6 Z" '
            f'fill="{fill}" fill-opacity="{op}"/>')


def bow(cx, cy, rot, scale=1.0, main="#ff5e9f", dark="#ff3d8a"):
    return f'''<g transform="translate({cx} {cy}) rotate({rot}) scale({scale})">
  <path d="M0 0 C-14 -24 -42 -22 -44 -2 C-42 20 -14 16 0 0 Z" fill="{main}"/>
  <path d="M0 0 C14 -24 42 -22 44 -2 C42 20 14 16 0 0 Z" fill="{main}"/>
  <path d="M-8 -4 C-22 -14 -34 -12 -36 -4" fill="none" stroke="#ffffff" stroke-opacity="0.55" stroke-width="3" stroke-linecap="round"/>
  <path d="M8 -4 C22 -14 34 -12 36 -4" fill="none" stroke="#ffffff" stroke-opacity="0.55" stroke-width="3" stroke-linecap="round"/>
  <circle cx="-26" cy="4" r="3" fill="#ffffff" fill-opacity="0.85"/>
  <circle cx="26" cy="4" r="3" fill="#ffffff" fill-opacity="0.85"/>
  <circle cx="0" cy="0" r="9" fill="{dark}"/>
  <circle cx="-2.5" cy="-3" r="3" fill="#ffffff" fill-opacity="0.7"/>
</g>'''


def eye(x, y, outer):
    """outer = -1 for the left eye (flick goes left), +1 for the right eye."""
    fx = 34 * outer
    return f'''<g transform="translate({x} {y})">
  <ellipse cx="0" cy="0" rx="25" ry="31" fill="#ffffff"/>
  <ellipse cx="0" cy="3" rx="21" ry="27" fill="url(#iris)"/>
  <ellipse cx="0" cy="3" rx="21" ry="27" fill="none" stroke="#5a3fa8" stroke-width="2"/>
  <ellipse cx="0" cy="6" rx="10" ry="14" fill="#2f2150"/>
  <circle cx="-8" cy="-9" r="8" fill="#ffffff"/>
  <circle cx="9" cy="12" r="4.2" fill="#ffffff" fill-opacity="0.92"/>
  <circle cx="-13" cy="11" r="2" fill="#ffffff" fill-opacity="0.75"/>
  <path d="M-27 -12 C-20 -35 20 -37 27 -14" fill="none" stroke="#3d2a52" stroke-width="6.5" stroke-linecap="round"/>
  <path d="M{27 * outer} -13 L{fx} -22" fill="none" stroke="#3d2a52" stroke-width="4.5" stroke-linecap="round"/>
  <path d="M-17 31 C-6 37 6 37 17 31" fill="none" stroke="#ffb0c6" stroke-width="2.5" stroke-linecap="round" stroke-opacity="0.9"/>
</g>'''


# ----------------------------------------------------------------- Ava
def make_girl():
    parts = []
    A = parts.append
    A('<svg xmlns="http://www.w3.org/2000/svg" width="320" height="440" viewBox="0 0 320 440">')
    A('<rect width="320" height="440" fill="#000000" fill-opacity="0"/>')  # pins the drawing bounds
    A('''<defs>
  <linearGradient id="hair" gradientUnits="userSpaceOnUse" x1="160" y1="40" x2="160" y2="380">
    <stop offset="0" stop-color="#ff8fc9"/><stop offset="0.55" stop-color="#f58bd8"/><stop offset="1" stop-color="#c3a0ff"/>
  </linearGradient>
  <linearGradient id="hairFront" gradientUnits="userSpaceOnUse" x1="160" y1="44" x2="160" y2="160">
    <stop offset="0" stop-color="#ffa6d6"/><stop offset="1" stop-color="#ff82c0"/>
  </linearGradient>
  <linearGradient id="skin" gradientUnits="userSpaceOnUse" x1="160" y1="78" x2="160" y2="262">
    <stop offset="0" stop-color="#fff4ee"/><stop offset="1" stop-color="#ffd8ca"/>
  </linearGradient>
  <linearGradient id="iris" gradientUnits="userSpaceOnUse" x1="0" y1="-24" x2="0" y2="30">
    <stop offset="0" stop-color="#6a4cff"/><stop offset="0.6" stop-color="#b56bff"/><stop offset="1" stop-color="#ff7ad1"/>
  </linearGradient>
  <linearGradient id="dress" gradientUnits="userSpaceOnUse" x1="160" y1="290" x2="160" y2="440">
    <stop offset="0" stop-color="#ffc9e4"/><stop offset="1" stop-color="#ff9fcf"/>
  </linearGradient>
</defs>''')

    # ---- twin tails (behind everything)
    A('<path d="M70 118 C22 140 6 240 32 330 C42 366 68 372 76 340 C68 296 80 226 114 170 Z" fill="url(#hair)"/>')
    A('<path d="M250 118 C298 140 314 240 288 330 C278 366 252 372 244 340 C252 296 240 226 206 170 Z" fill="url(#hair)"/>')
    A('<path d="M40 190 C28 245 36 300 52 336" fill="none" stroke="#ffffff" stroke-opacity="0.4" stroke-width="6" stroke-linecap="round"/>')
    A('<path d="M280 190 C292 245 284 300 268 336" fill="none" stroke="#ffffff" stroke-opacity="0.4" stroke-width="6" stroke-linecap="round"/>')
    A('<path d="M58 300 C56 330 62 350 70 352" fill="none" stroke="#a97be6" stroke-opacity="0.35" stroke-width="5" stroke-linecap="round"/>')
    A('<path d="M262 300 C264 330 258 350 250 352" fill="none" stroke="#a97be6" stroke-opacity="0.35" stroke-width="5" stroke-linecap="round"/>')

    # ---- back hair mass
    A('<ellipse cx="160" cy="168" rx="104" ry="98" fill="url(#hair)"/>')

    # ---- body / dress
    A('<path d="M62 440 C62 340 100 296 160 292 C220 296 258 340 258 440 Z" fill="url(#dress)"/>')
    A('<ellipse cx="76" cy="376" rx="31" ry="40" fill="url(#dress)" stroke="#ffffff" stroke-width="3.5"/>')
    A('<ellipse cx="244" cy="376" rx="31" ry="40" fill="url(#dress)" stroke="#ffffff" stroke-width="3.5"/>')
    A('<path d="M60 392 C70 400 84 400 94 392" fill="none" stroke="#ffffff" stroke-width="3" stroke-linecap="round" stroke-opacity="0.9"/>')
    A('<path d="M226 392 C236 400 250 400 260 392" fill="none" stroke="#ffffff" stroke-width="3" stroke-linecap="round" stroke-opacity="0.9"/>')
    # hands resting on the keys
    A('<circle cx="118" cy="432" r="17" fill="#ffe4d8"/>')
    A('<circle cx="202" cy="432" r="17" fill="#ffe4d8"/>')
    A('<path d="M104 424 C108 418 114 416 120 418" fill="none" stroke="#ffc4b4" stroke-width="2" stroke-linecap="round"/>')
    A('<path d="M200 418 C206 416 212 418 216 424" fill="none" stroke="#ffc4b4" stroke-width="2" stroke-linecap="round"/>')
    # neck
    A('<path d="M142 246 L142 296 L178 296 L178 246 Z" fill="#ffd2c2"/>')
    # sailor collar + ribbon
    A('<path d="M98 298 C122 286 198 286 222 298 L160 358 Z" fill="#ffffff"/>')
    A('<path d="M108 300 L160 350 L212 300" fill="none" stroke="#ff9fcf" stroke-width="4" stroke-linejoin="round" stroke-linecap="round"/>')
    A('<path d="M150 358 L136 398 L152 390 L158 402 Z" fill="#ff5e9f"/>')
    A('<path d="M170 358 L184 398 L168 390 L162 402 Z" fill="#ff3d8a"/>')
    A('<path d="M160 352 C132 322 108 346 126 366 C140 376 154 364 160 352 Z" fill="#ff5e9f"/>')
    A('<path d="M160 352 C188 322 212 346 194 366 C180 376 166 364 160 352 Z" fill="#ff5e9f"/>')
    A('<circle cx="160" cy="354" r="9" fill="#ff3d8a"/>')
    A('<circle cx="157" cy="351" r="3" fill="#ffffff" fill-opacity="0.7"/>')

    # ---- head
    A('<path d="M66 165 C66 98 118 78 160 78 C202 78 254 98 254 165 C254 218 214 262 160 262 C106 262 66 218 66 165 Z" fill="url(#skin)"/>')
    # side locks over the cheeks
    A('<path d="M64 140 C46 192 56 248 86 294 C98 252 94 206 104 150 Z" fill="url(#hair)"/>')
    A('<path d="M256 140 C274 192 264 248 234 294 C222 252 226 206 216 150 Z" fill="url(#hair)"/>')

    # blush, nose, mouth, brows
    A('<ellipse cx="92" cy="226" rx="19" ry="10" fill="#ff8fae" fill-opacity="0.55"/>')
    A('<ellipse cx="228" cy="226" rx="19" ry="10" fill="#ff8fae" fill-opacity="0.55"/>')
    for dx in (-8, 0, 8):
        A(f'<path d="M{92 + dx - 3} 229 L{92 + dx + 3} 222" stroke="#ff6f9a" stroke-opacity="0.6" stroke-width="2" stroke-linecap="round"/>')
        A(f'<path d="M{228 + dx - 3} 229 L{228 + dx + 3} 222" stroke="#ff6f9a" stroke-opacity="0.6" stroke-width="2" stroke-linecap="round"/>')
    A('<path d="M157 222 Q160 226 163 222" fill="none" stroke="#f0a898" stroke-width="2.5" stroke-linecap="round"/>')
    A('<path d="M146 233 Q160 256 174 233 Q160 240 146 233 Z" fill="#e8546f" stroke="#e8546f" stroke-width="2" stroke-linejoin="round"/>')
    A('<path d="M153 242 Q160 250 167 242 Q160 246 153 242 Z" fill="#ff9db4"/>')

    # eyes
    A(eye(116, 192, -1))
    A(eye(204, 192, +1))

    # eyebrows
    A('<path d="M92 150 Q114 140 138 148" fill="none" stroke="#d6559c" stroke-width="4.5" stroke-linecap="round"/>')
    A('<path d="M182 148 Q206 140 228 150" fill="none" stroke="#d6559c" stroke-width="4.5" stroke-linecap="round"/>')

    # ---- bangs
    A('<path d="M62 152 C56 84 112 44 160 44 C208 44 264 84 258 152 C250 132 244 122 236 112 '
      'C232 132 224 146 214 158 C212 134 206 118 196 108 C192 128 184 142 176 152 '
      'C172 134 166 122 160 114 C154 122 148 134 144 152 C136 142 128 128 124 108 '
      'C114 118 108 134 106 158 C96 146 88 132 84 112 C76 122 70 132 62 152 Z" fill="url(#hairFront)"/>')
    A('<path d="M92 84 C124 58 196 58 228 84" fill="none" stroke="#ffffff" stroke-opacity="0.5" stroke-width="7" stroke-linecap="round"/>')
    A('<path d="M104 98 C118 88 134 84 148 82" fill="none" stroke="#ffffff" stroke-opacity="0.45" stroke-width="4" stroke-linecap="round"/>')
    # ahoge
    A('<path d="M158 50 C148 20 172 6 190 26 C178 20 168 30 164 52 Z" fill="#ff8fc9"/>')

    # ---- hair accessories
    A(bow(66, 120, -16))
    A(bow(254, 120, 16))
    A(f'<polygon points="{star5(224, 92, 14, 6)}" fill="#ffd966" stroke="#ffffff" stroke-width="2.5" stroke-linejoin="round"/>')
    A(f'<polygon points="{star5(221, 89, 5, 2.2)}" fill="#fff3b0"/>')
    A(heart(96, 92, 12, "#ffffff"))
    A(heart(96, 92, 8.5, "#ff7eb6"))

    # ---- sparkles around Ava
    A(sparkle(24, 52, 14, "#ffffff", 0.95))
    A(sparkle(298, 46, 11, "#fff0a8", 0.95))
    A(sparkle(300, 250, 9, "#ffffff", 0.9))
    A(sparkle(18, 252, 8, "#fff0a8", 0.9))
    A(heart(300, 140, 9, "#ff7eb6", 0.9, 14))
    A(heart(20, 140, 8, "#b69cff", 0.9, -14))
    A('</svg>')
    return "\n".join(parts)


# ----------------------------------------------------------------- background
def make_background(w=900, h=640):
    rnd = random.Random(23)
    parts = []
    A = parts.append
    A(f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">')
    A(f'''<defs>
  <linearGradient id="sky" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="0" y2="{h}">
    <stop offset="0" stop-color="#ffcfe9"/><stop offset="0.5" stop-color="#e6c8ff"/><stop offset="1" stop-color="#bfe2ff"/>
  </linearGradient>
  <radialGradient id="glow" gradientUnits="userSpaceOnUse" cx="230" cy="250" r="330">
    <stop offset="0" stop-color="#ffffff" stop-opacity="0.75"/><stop offset="1" stop-color="#ffffff" stop-opacity="0"/>
  </radialGradient>
</defs>''')
    A(f'<rect width="{w}" height="{h}" fill="url(#sky)"/>')  # also pins the drawing bounds
    A(f'<rect width="{w}" height="{h}" fill="url(#glow)"/>')

    # soft bokeh circles
    for _ in range(18):
        r = rnd.uniform(14, 52)
        x, y = rnd.uniform(r, w - r), rnd.uniform(r, h * 0.8)
        A(f'<circle cx="{x:.0f}" cy="{y:.0f}" r="{r:.0f}" fill="#ffffff" fill-opacity="{rnd.uniform(0.10, 0.22):.2f}"/>')

    def cloud(x, y, s):
        return (f'<g transform="translate({x} {y}) scale({s})" fill="#ffffff" fill-opacity="0.8">'
                '<circle cx="0" cy="0" r="26"/><circle cx="32" cy="-14" r="33"/><circle cx="70" cy="0" r="25"/>'
                '<circle cx="98" cy="8" r="18"/><rect x="-26" y="2" width="142" height="24" rx="12"/></g>')

    A(cloud(48, 120, 1.0))
    A(cloud(690, 70, 1.1))
    A(cloud(780, 240, 0.8))
    A(cloud(330, 56, 0.6))
    A(cloud(560, 470, 0.7))

    # stars, sparkles and hearts
    cols = ["#ffffff", "#fff0a8", "#ffd1ec", "#d9c4ff"]
    for _ in range(34):
        A(sparkle(rnd.uniform(16, w - 16), rnd.uniform(16, 470), rnd.uniform(4, 11), rnd.choice(cols), rnd.uniform(0.55, 1.0)))
    hcols = ["#ff7eb6", "#ff9ccf", "#b69cff", "#ffffff", "#ffb3d9"]
    for _ in range(16):
        A(heart(rnd.uniform(16, w - 16), rnd.uniform(20, 470), rnd.uniform(5, 11), rnd.choice(hcols), rnd.uniform(0.5, 0.9), rnd.randint(-20, 20)))
    # little polka dots
    for _ in range(50):
        A(f'<circle cx="{rnd.uniform(0, w):.0f}" cy="{rnd.uniform(0, 480):.0f}" r="{rnd.uniform(1.5, 3.6):.1f}" fill="#ffffff" fill-opacity="{rnd.uniform(0.4, 0.8):.2f}"/>')
    A('</svg>')
    return "\n".join(parts)


if __name__ == "__main__":
    (OUT / "ava_girl.svg").write_text(make_girl())
    (OUT / "background.svg").write_text(make_background())
    print("wrote", OUT / "ava_girl.svg", "and", OUT / "background.svg")
