#!/bin/env python3
from math import sin,cos,pi
from fractions import Fraction

steps = 24
r = 8

fx = []
fy = []

i = 0
while i < steps:
    theta = i * (2 * pi) / steps

    x = r * sin(theta)
    y = -r * cos(theta)

    fx.append(Fraction(x).limit_denominator(100))
    fy.append(Fraction(y).limit_denominator(100))


    i = i + 1


print("// x:")
for f in fx:
    print(f"  ((FX_ONE*{f.numerator})/{f.denominator}), ",)
print()
print("// y:")
for f in fy:
    print(f"  ((FX_ONE*{f.numerator})/{f.denominator}), ",)

print()
