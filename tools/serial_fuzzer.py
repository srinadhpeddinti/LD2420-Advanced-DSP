import serial
import time
import random
import string

def run_fuzzer(port):
    with serial.Serial(port, 115200, timeout=1) as ser:
        print(f"Fuzzing AsciiProtocolParser on {port}...")
        while True:
            # Generate total garbage
            garbage_len = random.randint(1, 200)
            garbage = bytes(random.getrandbits(8) for _ in range(garbage_len))
            ser.write(garbage)
            
            # Inject partial strings
            bad_string = "ON  R:-99 M:XXX S:00\n"
            ser.write(bad_string.encode('ascii'))
            
            time.sleep(random.uniform(0.01, 0.05))

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("Usage: python serial_fuzzer.py <COM_PORT>")
        sys.exit(1)
    run_fuzzer(sys.argv[1])
