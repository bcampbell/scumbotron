#!/bin/env python3
# generate a simple sincos lookup table
# (just cut&paste data into misc.c)

from math import sin,cos,pi

steps = 24
r = 127

tx = []
ty = []

i = 0
while i < steps:
    theta = i * (2 * pi) / steps

    s = r * sin(theta)
    c = r * cos(theta)
    print(f"  {int(s)}, {int(c)} ",)
    i = i + 1
print()

