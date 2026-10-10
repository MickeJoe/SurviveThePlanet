"""Create a sharp transparent HUD coin from its editable vector source."""
from pathlib import Path
import math
import struct
import zlib

root = Path(r'C:\UE5\SurviveThePlanet 5.8')
assert (root / 'SurviveThePlanet.uproject').is_file()
folder = root / 'ContentSource/UI'
folder.mkdir(parents=True, exist_ok=True)
svg = '''<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 128 128">
  <path d="M64 10 L110 37 L110 91 L64 118 L18 91 L18 37 Z" fill="#201b0b" fill-opacity="0.65" stroke="#ffc32b" stroke-width="7" stroke-linejoin="round"/>
  <path d="M83 43 L72 35 L53 35 L42 47 L42 81 L53 93 L72 93 L83 85" fill="none" stroke="#ffd45b" stroke-width="10" stroke-linejoin="round" stroke-linecap="round"/>
</svg>
'''
(folder / 'T_Credits.svg').write_text(svg, encoding='utf-8')
hexagon = [(64,10),(110,37),(110,91),(64,118),(18,91),(18,37),(64,10)]
letter = [(83,43),(72,35),(53,35),(42,47),(42,81),(53,93),(72,93),(83,85)]

def distance(x, y, a, b):
    dx, dy = b[0]-a[0], b[1]-a[1]
    t = max(0, min(1, ((x-a[0])*dx+(y-a[1])*dy)/(dx*dx+dy*dy)))
    return math.hypot(x-a[0]-t*dx, y-a[1]-t*dy)

def inside(x, y):
    return all((b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]) >= 0 for a,b in zip(hexagon, hexagon[1:]))

scanlines = bytearray()
for y in range(128):
    scanlines.append(0)
    for x in range(128):
        red = green = blue = alpha = 0.0
        for sy in range(4):
            for sx in range(4):
                px, py = x+(sx+.5)/4, y+(sy+.5)/4
                rgba = (32,27,11,166) if inside(px,py) else (0,0,0,0)
                if min(distance(px,py,a,b) for a,b in zip(hexagon,hexagon[1:])) <= 3.5:
                    rgba = (255,195,43,255)
                if min(distance(px,py,a,b) for a,b in zip(letter,letter[1:])) <= 5:
                    rgba = (255,212,91,255)
                weight = rgba[3]/255
                red += rgba[0]*weight; green += rgba[1]*weight; blue += rgba[2]*weight
                alpha += rgba[3]
        if alpha:
            scanlines.extend((round(red*255/alpha),round(green*255/alpha),round(blue*255/alpha),round(alpha/16)))
        else:
            scanlines.extend((0,0,0,0))

def chunk(kind, data):
    return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)

png = b'\x89PNG\r\n\x1a\n'
png += chunk(b'IHDR',struct.pack('>IIBBBBB',128,128,8,6,0,0,0))
png += chunk(b'IDAT',zlib.compress(scanlines,9))+chunk(b'IEND',b'')
(folder / 'T_Credits.png').write_bytes(png)
print('Created credits icon PNG and editable SVG source.')
