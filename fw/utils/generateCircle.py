#!/usr/bin/env python

import math

radius = 65535/2.4
fi = 0
while fi < 2 * math.pi:
    fi += 2 * math.pi / 500
    x = int(radius * math.sin(fi))
    y = int(radius * math.cos(fi))
    print(str(x) + " " + str(y))