#!/usr/bin/env python

import socket
import sys
import struct
import time

def dump(y):
    ret = ""
    for x in y:
        ret += str(ord(x)) + " "
    return ret

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

server_address = (sys.argv[1], int(sys.argv[2]))
sock.connect(server_address)
print("Connected")

points = [ map(int, x.split(" ")) for x in open(sys.argv[3]).readlines() ]
print(points)

buffer = struct.pack("<BHBH", 128, 2 + 5 * len(points), 21, 0)
for x, y in points:
    buffer += struct.pack("<hhB", x, y, 1)
print("Sending " + dump(buffer))
sock.send(buffer)
print("Sent")
time.sleep(1)
print(dump(sock.recv(150)))