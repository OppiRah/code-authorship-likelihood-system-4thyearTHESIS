"""Build ulccslogo.ico (the exe/installer/shortcut icon) from ulccslogo.png.

Run from the authorship-cpp folder:   python tools/make_icon.py

The seal PNG is a white mark on a transparent background (the sidebar
draws it on the dark app background). On its own it would vanish on a
light Explorer/desktop background, so the icon places the mark -- cropped
to its visible pixels, scaled, never stretched -- on a rounded tile in
the app's own background colour (GRAY_950, RGB 19,19,22 in
gui_common.h). Written as one multi-size ICO (16..256 px). Needs Pillow.
The committed .ico is the output; rerun only if the seal PNG changes.
"""
from PIL import Image, ImageDraw

SRC = "ulccslogo.png"
DST = "ulccslogo.ico"
TILE = (19, 19, 22, 255)          # GRAY_950, the app background
SIZES = [(16, 16), (20, 20), (24, 24), (32, 32), (40, 40), (48, 48),
         (64, 64), (128, 128), (256, 256)]

N = 1024                          # work large, downsample once
mark = Image.open(SRC).convert("RGBA")
mark = mark.crop(mark.getchannel("A").getbbox())
scale = (N * 0.80) / max(mark.size)
mark = mark.resize((round(mark.width * scale), round(mark.height * scale)),
                   Image.LANCZOS)

canvas = Image.new("RGBA", (N, N), (0, 0, 0, 0))
ImageDraw.Draw(canvas).rounded_rectangle((0, 0, N - 1, N - 1),
                                         radius=N // 6, fill=TILE)
canvas.alpha_composite(mark, ((N - mark.width) // 2, (N - mark.height) // 2))
canvas = canvas.resize((256, 256), Image.LANCZOS)

# bitmap_format="bmp": classic DIB entries, which every Windows icon
# consumer reads (PNG-compressed entries are only safe at 256 px).
canvas.save(DST, format="ICO", sizes=SIZES, bitmap_format="bmp")
print(f"wrote {DST}: {', '.join(f'{w}' for w, _ in SIZES)} px")
