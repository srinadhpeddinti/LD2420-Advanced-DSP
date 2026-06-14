import serial
import time
import random

# Simulates LD2420 UART stream
def run_simulator(port):
    with serial.Serial(port, 115200, timeout=1) as ser:
        print(f"Injecting mock data into {port}...")
        while True:
            # Simulate a target walking at 250cm
            range_cm = int(250 + random.uniform(-10, 10))
            moving_energy = int(random.uniform(50, 100))
            static_energy = int(random.uniform(10, 30))
            
            # Format: ON  R: 250 M: 80 S: 20
            payload = f"ON  R:{range_cm:04d} M:{moving_energy:03d} S:{static_energy:03d}\n"
            ser.write(payload.encode('ascii'))
            time.sleep(0.1) # 10Hz

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("Usage: python radar_simulator.py <COM_PORT>")
        sys.exit(1)
    run_simulator(sys.argv[1])
