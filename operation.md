# Projector operation

On the physical layer the projection position and laser state is updated at 30
kHz (period 33 us). In ideal case, there are 1200 points per video frame at 25
fps. However, keep in mind that the points should be in roughly the same
distance in order to achieve the same line brightness (this can be solved by
variable laser intensity in the future). Also it is not a good idea to insert
blank frames when less points are used per frame - it yields in flickering.
Instead, the frame should be repeated.

Therefore, on the logical level the projector works with *frames*. Each frame is
a list of XY points with a laser state. There can be any reasonable number of
points in the list. The points are signed integers with 0 coordinate in the
middle. When a frame is activated, the list of points is continuously displayed
and repeated until a the frame is deactivated.

Projector holds a frame buffer. This buffer can be asynchronously filled with
frames. When a frame is popped out of the queue it is activated and stays
activated until a new frame comes. Therefore you send a new frame only when you
want to change it.

The projector keeps a list of named calibration values. One of these
calibrations can be activated and it will be applied on the active frame.
The calibration holds following items:

- X and Y translation
- rotation
- X and Y scaling
- X and Y shear
- X and Y trapezoidal transformation

# Communication protocol

Projector features multiple interfaces. All of them share the same communication
protocol and control the projector at the same time and it a user responsibility
to synchronize them:

- SPI slave
- TCP/IP socket server on port 4242. Up to 4 connections can be opened.

Communication is based on binary messages using little-endian following this
format:

```
| constant 0x80 | data size | command |        data        |
|    uint8_t    |  uint16_t | uint8_t | `data size length` |
```

The projector responds with the same command code to every command. Following
commands are implemented, other commands are ignored.

- **0** - ask for device identification
    - no data
    - response contains a null-terminated device id string
- **10** - reset transformation
    - payload is empty
- **11** - set/get XY translation
    - payload is empty (only gets values) or a pair of float translation
    - response contains 2 floats with previous values of transformation
- **12** - set/get rotation
    - payload is empty (only gets value) or a float with rotation in radians
    - response contains previous value
- **13** - set/get XY shear
- **14** - set/get XY trapezoiding
- **20** - ask for frame buffer state
    - payload is empty
    - response contains two `uint16_t` numbers - number of frames in the buffer
      and number of free space in the buffer
- **21** - push a new frame
    - payload contains `uint16_t` number of minimal repetition of a frame
      followed by a list of frame points in binary format
    - the format is `int16_t x | int16_t y | uint8_t laserBrightness`
    - note, that laser brightness is not implemented in hardware and nonzero
      brightness is interpreted as full brightness.
    - response is empty in case of success, otherwise null terminated error
      string.
- **30** - keep alive message
    - no payload, no response - send from projector to clients. Should be
      ignored.