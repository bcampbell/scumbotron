#!/bin/env python3
from math import sin,cos,pi
from fractions import Fraction

steps = 24
r = 127

tx = []
ty = []

i = 0
while i < steps:
    theta = i * (2 * pi) / steps

    x = r * sin(theta)
    y = r * -cos(theta)

    tx.append(x)
    ty.append(y)

    i = i + 1


print("// x:")
for f in tx:
    print(f"  {int(f)}, ",)
print()
print("// y:")
for f in ty:
    print(f"  {int(f)}, ",)

print()
