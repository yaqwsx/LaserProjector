#!/usr/bin/env python

segments = 200
r = 30000

for x in range(-r/2, r/2, r / segments):
    if x < -r/4 or x > r/4:
        y = r / 2
    else:
        y = -r / 2
    print(str(x) + " " + str(y))