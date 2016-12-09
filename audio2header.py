#!/usr/bin/python3

"""
/dev/heart
==========

Kernel Module which creates a device to listen to Tuxs heart.

-> Script to convert a raw audio file into a C byte array.

@copyright: GPLv2 (see LICENSE), by Timo Furrer <tuxtimo@gmail.com>
"""

import sys


def convert(audio_file):
    """
    Convert the given audio file to a C compatible
    char array.
    """
    with open(audio_file, 'rb') as raw_audio_file:
        data = raw_audio_file.read()

    print(',\n'.join(hex(b) for b in bytearray(data)))


if __name__ == '__main__':
    convert(sys.argv[1])
