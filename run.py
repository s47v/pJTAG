import serial
import time
import argparse
import re
import sys
import os


SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200
LOG_FILE = 'pico_output.log'
GLITCH_LENGTH_STR = ""
GLITCH_DELAY_STR = ""
MAX_LOG_SIZE = 4294967296 #4GB 
PASSWORD = b''


def validate_range_format(name, value):
    if not re.match(r'^\d+,\d+$', value):
        print(f"Error: Argument '{name}' must be in the format MIN,MAX (e.g., 20,500). You provided: '{value}'")
        sys.exit(1)

def read_password_file(pw_file):
    with open(pw_file, "r") as f:
        hex_string = f.read().strip()
    if len(hex_string) % 2 != 0:
        raise ValueError("Password must have an even number of hex digits")
    return bytes.fromhex(hex_string)

def rotate_log_file(log_file):
    index = 1
    while os.path.exists(f"{log_file}.{index}"):
        index += 1
    os.rename(log_file, f"{log_file}.{index}")
    print(f"Log rotated")
    return open(log_file, 'a')


parser = argparse.ArgumentParser()
parser.add_argument("-p", "--port", help = "serial port")
parser.add_argument("-o", "--Output", help = "Logfile")
parser.add_argument("-l", "--glitch_length", help = "range MIN,MAX")
parser.add_argument("-d", "--glitch_delay", help = "range MIN,MAX")
parser.add_argument("-x", "--password", help = "password file in format '111...888' (32 bytes)")
args = parser.parse_args()

if args.Output:
    LOG_FILE = args.Output
if args.port:
    SERIAL_PORT = args.port
if args.glitch_length:
    validate_range_format("glitch_length", args.glitch_length)
    GLITCH_LENGTH_STR = args.glitch_length
if args.glitch_delay:
    validate_range_format("glitch_delay", args.glitch_delay)
    GLITCH_DELAY_STR = args.glitch_delay
if args.password:
    PASSWORD = read_password_file(args.password)



def main():


    # Open serial connection and log file
    print(f"Connecting to Pico on {SERIAL_PORT}...")
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
        time.sleep(2)  # Let Pico initialize USB
        # Send the arguments to Pico
        ser.read_until(str.encode("Waiting on arguments...\n"))
        ser.write((GLITCH_LENGTH_STR + "\n").encode('utf-8'))
        ser.write((GLITCH_DELAY_STR + "\n").encode('utf-8'))
        if(len(PASSWORD) > 0):
            ser.write(b'P')
            ser.write(PASSWORD)
        
        

        with open(LOG_FILE, 'a') as f:
            try:
                while True:
                    if ser.in_waiting > 0:
                        line = ser.readline().decode('utf-8', errors='ignore').strip()
                        if line:
                            print(f"{line}")
                            f.write(f"{line}\n")  

                            if f.tell() >= MAX_LOG_SIZE:
                                f.close()
                                f = rotate_log_file(LOG_FILE)                          
                    else:
                        time.sleep(0.01)
            except KeyboardInterrupt:
                print("\nExit")
                ser.close()
                f.close()
  

if __name__ == "__main__":
    main()