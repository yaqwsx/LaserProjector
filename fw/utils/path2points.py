#!/usr/bin/env python

import sys
from svg.path import parse_path

segments = 800
size = 30000

path = parse_path(open(sys.argv[1], 'r').read())
points = [ path.point( x / float(segments) ) for x in range(segments) ]
points = [ (x.real, x.imag) for x in points]

max_x = min_x = points[0][0]
max_y = min_y = points[0][1]
for x, y in points:
    max_x = max(max_x, x)
    min_x = min(min_x, x)
    max_y = max(max_y, y)
    min_y = min(min_y, y)

transX = ( min_x + max_x ) / 2
transY = ( min_y + max_y ) / 2
divider = max( max_x - min_x, max_y - min_y )

for x, y in points:
    xx = int(size / 2 * (x - transX) / divider)
    yy = int(size / 2 * (y - transY) / divider)
    print(str(xx) + " " + str(yy))
