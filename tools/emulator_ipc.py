#!/usr/bin/env python3
"""Small bounded client for the dedicated ProTracker Amiberry test instance."""
import argparse
from pathlib import Path
import socket
import time


class Emulator:
    def __init__(self, path='/tmp/amiberry.sock'):
        self.path = path
        if 'Config=ProTracker isolated baseline' not in self.command('GET_STATUS'):
            raise RuntimeError('Refusing to control an unrelated emulator configuration')

    def command(self, *args):
        fields = [str(arg) for arg in args]
        if any(any(c in field for c in '\r\n\t') for field in fields):
            raise ValueError('Invalid IPC field')
        with socket.socket(socket.AF_UNIX) as client:
            client.settimeout(3)
            client.connect(self.path)
            client.sendall(('\t'.join(fields) + '\n').encode())
            data = bytearray()
            while b'\n' not in data:
                part = client.recv(65536)
                if not part:
                    break
                data.extend(part)
                if len(data) > 1024 * 1024:
                    raise RuntimeError('IPC response too large')
        reply = data.decode().strip()
        if not reply.startswith('OK'):
            raise RuntimeError(reply)
        return reply

    def tap(self, code):
        self.command('SEND_KEY', code, 1)
        try:
            time.sleep(.12)
        finally:
            self.command('SEND_KEY', code, 0)
        time.sleep(.15)

    def click(self, x, y):
        # Native tracker coords, MOUSE_SPEED=11/16. Keep each host delta small
        # to avoid hardware mouse-counter overflow between guest polls.
        for _ in range(12):
            self.command('SEND_MOUSE', -60, -60, 0)
            time.sleep(.06)
        dx, dy = round(x * 16 / 11), round(y * 16 / 11)
        while dx or dy:
            sx, sy = min(dx, 50), min(dy, 50)
            self.command('SEND_MOUSE', sx, sy, 0)
            dx -= sx
            dy -= sy
            time.sleep(.07)
        self.command('SEND_MOUSE', 0, 0, 1)
        try:
            time.sleep(.15)
        finally:
            self.command('SEND_MOUSE', 0, 0, 0)
        time.sleep(.3)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--socket', default='/tmp/amiberry.sock')
    p.add_argument('args', nargs='+')
    ns = p.parse_args()
    emu = Emulator(ns.socket)
    if ns.args[0] == 'click':
        x, y = map(int, ns.args[1:])
        if not (0 <= x <= 319 and 0 <= y <= 255):
            p.error('Native coordinates outside 320x256')
        emu.click(x, y)
    elif ns.args[0] == 'key':
        emu.tap(int(ns.args[1], 0))
    else:
        print(emu.command(*ns.args))


if __name__ == '__main__':
    main()
