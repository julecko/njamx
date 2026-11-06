#!/usr/bin/env python3
"""One-off generator for the njamx app icon (a colorful radio-jamming
glyph: broadcast arcs cut by a lightning bolt). Run manually with Pillow
installed; outputs njamx.ico (multi-res) and njamx.png (256x256) into
this directory. Not part of the CMake build."""

import math
import os

from PIL import Image, ImageDraw, ImageFilter

SS = 4  # supersampling factor for anti-aliasing
BASE = 1024
SIZE = BASE * SS

OUT_DIR = os.path.dirname(os.path.abspath(__file__))


def lerp(a, b, t):
    return a + (b - a) * t


def lerp_color(c1, c2, t):
    return tuple(int(lerp(a, b, t)) for a, b in zip(c1, c2))


def make_base():
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))

    # Rounded-square gradient background (purple -> blue -> cyan), radial
    # from the top-left-ish light source for a glossy modern-app-icon feel.
    bg = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    px = bg.load()
    c_center = (64, 120, 255, 255)   # electric blue
    c_edge = (88, 28, 140, 255)      # deep purple
    cx, cy = SIZE * 0.38, SIZE * 0.32
    max_d = math.hypot(SIZE - cx, SIZE - cy)
    step = 4  # sample every few px for speed, then upscale-safe since SS is big
    for y in range(0, SIZE, step):
        for x in range(0, SIZE, step):
            d = math.hypot(x - cx, y - cy) / max_d
            d = min(1.0, d)
            col = lerp_color(c_center, c_edge, d)
            for yy in range(y, min(y + step, SIZE)):
                for xx in range(x, min(x + step, SIZE)):
                    px[xx, yy] = col

    mask = Image.new("L", (SIZE, SIZE), 0)
    mdraw = ImageDraw.Draw(mask)
    radius = int(SIZE * 0.22)
    mdraw.rounded_rectangle([SIZE * 0.04, SIZE * 0.04, SIZE * 0.96, SIZE * 0.96],
                            radius=radius, fill=255)

    img = Image.composite(bg, Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0)), mask)
    draw = ImageDraw.Draw(img)

    # Subtle inner highlight (glossy sheen) top half.
    sheen = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    sdraw = ImageDraw.Draw(sheen)
    sdraw.ellipse([SIZE * 0.02, -SIZE * 0.35, SIZE * 0.98, SIZE * 0.55],
                 fill=(255, 255, 255, 28))
    sheen_masked = Image.composite(sheen, Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0)), mask)
    img = Image.alpha_composite(img, sheen_masked)
    draw = ImageDraw.Draw(img)

    # --- Broadcast/signal arcs (white, glowing) rising from a dot ---
    ox, oy = SIZE * 0.50, SIZE * 0.74  # origin of the signal (antenna dot)
    arc_color = (255, 255, 255, 235)
    glow_color = (255, 255, 255, 60)

    def signal_arcs(shift_x=0, shift_y=0, scale=1.0, color=arc_color, width_scale=1.0):
        for i, r in enumerate([0.16, 0.26, 0.36]):
            rr = r * SIZE * scale
            bbox = [ox - rr + shift_x, oy - rr + shift_y, ox + rr + shift_x, oy + rr + shift_y]
            w = int(SIZE * 0.028 * width_scale)
            draw.arc(bbox, start=200, end=340, fill=color, width=max(w, 2))

    # soft glow pass first (wider, translucent)
    signal_arcs(color=glow_color, width_scale=2.6)
    signal_arcs()

    # antenna dot
    dot_r = SIZE * 0.045
    draw.ellipse([ox - dot_r, oy - dot_r, ox + dot_r, oy + dot_r], fill=(255, 255, 255, 255))
    draw.ellipse([ox - dot_r * 0.5, oy - dot_r * 0.5, ox + dot_r * 0.5, oy + dot_r * 0.5],
                fill=(90, 160, 255, 255))

    # --- Jamming lightning bolt slashing across the arcs, bold + colorful ---
    bolt = [
        (0.68, 0.14), (0.52, 0.42), (0.63, 0.42),
        (0.40, 0.78), (0.58, 0.46), (0.47, 0.46),
    ]
    bolt_pts = [(SIZE * x, SIZE * y) for x, y in bolt]

    # outer glow: same shape, blurred, sitting underneath the sharp bolt
    glow = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    gdraw = ImageDraw.Draw(glow)
    gdraw.polygon(bolt_pts, fill=(255, 140, 0, 160))
    glow = glow.filter(ImageFilter.GaussianBlur(SIZE * 0.012))
    img = Image.alpha_composite(img, glow)
    draw = ImageDraw.Draw(img)

    # bolt body: warm yellow-orange gradient look via two overlaid polygons
    draw.polygon(bolt_pts, fill=(255, 196, 20, 255))
    inset = [(SIZE * x, SIZE * y) for x, y in
             [(0.645, 0.17), (0.545, 0.40), (0.615, 0.40), (0.47, 0.64)]]
    draw.polygon(inset, fill=(255, 90, 40, 255))

    # a couple of bright accent sparks near the bolt tip for extra color/energy
    spark_color = (255, 60, 90, 230)
    draw.ellipse([SIZE * 0.70, SIZE * 0.10, SIZE * 0.76, SIZE * 0.16], fill=spark_color)
    draw.ellipse([SIZE * 0.30, SIZE * 0.70, SIZE * 0.35, SIZE * 0.75], fill=(80, 220, 255, 220))

    return img


def main():
    img = make_base().resize((BASE, BASE), Image.LANCZOS)

    png_path = os.path.join(OUT_DIR, "njamx.png")
    img.save(png_path)
    print("wrote", png_path)

    ico_path = os.path.join(OUT_DIR, "njamx.ico")
    sizes = [16, 24, 32, 48, 64, 128, 256]
    img.save(ico_path, sizes=[(s, s) for s in sizes])
    print("wrote", ico_path)


if __name__ == "__main__":
    main()
