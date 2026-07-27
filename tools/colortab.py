import math
import png
import sys

def clamp(n, minval, maxval):
    return max(minval, min(n, maxval))

def hsv2rgb(h, s, v, a) -> tuple:
    if s:
        if h == 1.0: h = 0.0
        i = int(h*6.0); f = h*6.0 - i
        
        w = v * (1.0 - s)
        q = v * (1.0 - s * f)
        t = v * (1.0 - s * (1.0 - f))
        
        if i==0: return (v, t, w, a)
        if i==1: return (q, v, w, a)
        if i==2: return (w, v, t, a)
        if i==3: return (w, q, v, a)
        if i==4: return (t, w, v, a)
        if i==5: return (v, w, q, a)
    return (v, v, v, a)

def make_image(arr, name, w, h):
    img = []
    print(f"producing {w}x{h} image {name}")
    for y in range(0, h):
        row = []
        for x in range(0, w):
            off = y * w + x
            r, g, b = arr[off] if off < len(arr) else (0, 0, 0)
            row += [r & 0xFF, g & 0xFF, b & 0xFF]
        img.append(row)
    png.from_array(img, 'RGB').save(name)

# palette init, copied from demos but could be done from an image or smth
palette = []

print("initializing palette")

# shades
#for i in range(0, 16):
#    # goes from 0-1 by 16
#    v = int(math.log(i + 0.1) / math.e * 255)
#    palette.append((v, v, v))

# couple rows of fading but color, then some pastels
for r in range(0, 16):
    s = 1 + math.log(r + 1) * 0.25
    v = 1 + math.fmod(r * 0.3, 4)
    for h in range(0, 16):
        r, g, b, _ = hsv2rgb(h / 16.0, 1.0 / s, 1.0 / v, 1.0)
        palette.append((int(r * 255), int(g * 255), int(b * 255)))

make_image(palette, "palette.png", 16, 16)

COLORTAB_BITS = int(sys.argv[1]) if len(sys.argv) > 1 else 6
COLORTAB_SHIFT = 8 - COLORTAB_BITS
COLORTAB_PERCOLOR = (1 << COLORTAB_BITS)
COLORTAB_SIZE = COLORTAB_PERCOLOR * COLORTAB_PERCOLOR * COLORTAB_PERCOLOR

# preallocate for indexing
colortab = [None] * COLORTAB_SIZE
print(f"calculating {COLORTAB_BITS}-bit color table ({COLORTAB_SIZE} entries)")

def colortab_index(r, g, b):
    r = clamp(r, 0, 255)
    g = clamp(g, 0, 255)
    b = clamp(b, 0, 255)
    return ((r >> COLORTAB_SHIFT) << (2 * COLORTAB_BITS)) | ((g >> COLORTAB_SHIFT) << COLORTAB_BITS) | (b >> COLORTAB_SHIFT)

def colortab_set(r, g, b, v):
    colortab[colortab_index(r, g, b)] = v

n = 0
max_n = COLORTAB_SIZE * len(palette)
for r in range(0, COLORTAB_PERCOLOR):
    for g in range(0, COLORTAB_PERCOLOR):
        for b in range(0, COLORTAB_PERCOLOR):
            last_dist = 0xFFFFFFFF
            for c in palette:
                # find squared euclidean distance
                sr = (r << COLORTAB_SHIFT)
                sg = (g << COLORTAB_SHIFT)
                sb = (b << COLORTAB_SHIFT)
                cr, cg, cb = c
                dr = cr - sr
                dg = cg - sg
                db = cb - sb
                dist = dr * dr + dg * dg + db * db
                if dist < last_dist:
                    last_dist = dist
                    colortab_set(sr, sg, sb, c)
                    # only print here to save (some) time
                    print(f"\r{n / float(max_n) * 100:.3f}%", end="    ")
                n += 1
print()

size = math.ceil(math.sqrt(len(colortab)))
w = size
h = size
make_image(colortab, f"colortab{COLORTAB_BITS}.png", w, h)
