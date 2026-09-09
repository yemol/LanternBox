#!/usr/bin/env python3
import glob
import sys
import time
import serial

ACTIONS = {
    "off": ("GNSS_SOFT_OFF", "GNSS_POWER_SET"),
    "on": ("GNSS_SOFT_ON", "GNSS_POWER_SET"),
    "status": ("GNSS_POWER?", "GNSS_POWER_STATE"),
}

def find_port():
    ports = sorted(glob.glob('/dev/cu.usbserial*'))
    if not ports:
        raise SystemExit('No /dev/cu.usbserial* port found')
    return ports[0]

def main():
    if len(sys.argv) != 2 or sys.argv[1].lower() not in ACTIONS:
        raise SystemExit('Usage: gnss_soft_switch.py {off|on|status}')
    action = sys.argv[1].lower()
    cmd, marker = ACTIONS[action]
    port = find_port()
    s = serial.Serial()
    s.port = port
    s.baudrate = 115200
    s.timeout = 0.2
    s.write_timeout = 1
    s.rtscts = False
    s.dsrdtr = False
    s.dtr = False
    s.rts = False
    s.open()
    time.sleep(1.0)
    try:
        s.reset_input_buffer()
    except Exception:
        pass
    s.write((cmd + '\n').encode('ascii'))
    s.flush()
    deadline = time.time() + 5
    while time.time() < deadline:
        try:
            raw = s.readline()
        except serial.SerialException as exc:
            s.close()
            raise SystemExit(f'Serial disconnected: {exc}')
        if not raw:
            continue
        line = raw.decode('utf-8', errors='replace').strip()
        if marker in line:
            print(line)
            s.close()
            return
    s.close()
    raise SystemExit(f'No {marker} response within 5 s')

if __name__ == '__main__':
    main()
