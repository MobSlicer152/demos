import gif
import matplotlib.pyplot as plt
import numpy as np
import scipy as sp

reader = gif.Reader()
with open("darkfountain.gif", "rb") as f:
    reader.feed(f.read())
    
def rgb2hsv(r, g, b):
    r /= 255
    g /= 255
    b /= 255
    maxc = max(r, g, b)
    minc = min(r, g, b)
    v = maxc
    if minc == maxc:
        return 0.0, 0.0, v
    s = (maxc - minc) / maxc
    rc = (maxc - r) / (maxc - minc)
    gc = (maxc - g) / (maxc - minc)
    bc = (maxc - b) / (maxc - minc)
    if r == maxc:
        h = 0.0 + bc - gc
    elif g == maxc:
        h = 2.0 + rc - bc
    else:
        h = 4.0 + gc - rc
    h = (h / 6.0) % 1.0
    return [h * 360, s, v]
    
def get_color(reader, img, x, y):
    i = img.get_pixels()[x + y * img.width]
    c = reader.color_table[i]
    return c

def get_val(c):
    return rgb2hsv(c[0], c[1], c[2])[2]
    
def plot(ax, v, c):
    x = list(range(0, len(v)))
    y = np.array(v)
    print(f"stdev={np.std(y)}, mean={np.mean(y)}")
#    popt, pcov = sp.optimize.curve_fit(x, y)
    ax.scatter(x, y, c=c)
    
fg = (200, 50)
bg = (470, 20)

fg_colors = []
fg_values = []
bg_colors = []
bg_values = []
for block in reader.blocks:
    if isinstance(block, gif.Image):
        fg_color = get_color(reader, block, fg[0], fg[1])
        fg_colors.append(np.array(fg_color) / 255 * 0.7)
        fg_values.append(get_val(fg_color))
        bg_color = get_color(reader, block, bg[0], bg[1])
        bg_colors.append(np.array(bg_color) / 255 * 0.7)
        bg_values.append(get_val(bg_color))

fig, ax = plt.subplots()
ax.set_autoscaley_on(True)
plot(ax, fg_values, fg_colors)
plot(ax, bg_values, bg_colors)
plt.show()
