"""Native tracker test input helpers, usable only through the guarded emulator."""
import time

RAW = dict(zip('qwertyuiop', range(0x10, 0x1a)))
RAW.update(zip('asdfghjkl', range(0x20, 0x29)))
RAW.update(zip('zxcvbnm', range(0x31, 0x38)))
RAW.update(zip('1234567890', range(1, 11)))
RAW.update({' ': 0x40, '/': 0x3a, '.': 0x3c})


def text(emu, value):
    for char in value.lower():
        if char == ':':
            emu.command('SEND_KEY', 0x60, 1)
            try: emu.tap(0x29)
            finally: emu.command('SEND_KEY', 0x60, 0)
        else:
            if char not in RAW: raise ValueError('Unsupported test input character: ' + char)
            emu.tap(RAW[char])


def replace_field(emu, x, y, value, width=24):
    emu.click(x, y)
    for _ in range(width): emu.tap(0x4f)
    for _ in range(width): emu.tap(0x46)
    text(emu, value)
    emu.tap(0x44)
    time.sleep(.5)
