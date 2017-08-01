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

cmd = sys.argv[3]

if cmd == "reset":
    buffer = struct.pack("<BHB", 128, 0, 10)
elif cmd == "translate":
    buffer = struct.pack("<BHBff", 128, 8, 11, float(sys.argv[4]), float(sys.argv[5]))
elif cmd == "rotate":
    buffer = struct.pack("<BHBf", 128, 4, 12, float(sys.argv[4]))
elif cmd == "shear":
    buffer = struct.pack("<BHBff", 128, 8, 13, float(sys.argv[4]), float(sys.argv[5]))
elif cmd == "trap":
    buffer = struct.pack("<BHBff", 128, 8, 14, float(sys.argv[4]), float(sys.argv[5]))
else:
    print("No command given")
    sys.exit(1)

print("Sending " + dump(buffer))
sock.send(buffer)
print("Sent")
time.sleep(2)
print(dump(sock.recv(150)))