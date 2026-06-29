#!/usr/bin/env python3
import os
import sys
import time
import glob
import serial

PORT = "COM5"
BAUDRATE = 115200
DATA_DIR = os.path.join(os.path.dirname(__file__), "data")

def sync_file(ser, filepath, dest_filename):
    file_size = os.path.getsize(filepath)
    print(f"Syncing {dest_filename} ({file_size} bytes)...")
    
    # Send start header
    ser.write(f"TRANS:{dest_filename}:{file_size}\n".encode('utf-8'))
    
    # Wait for READY response
    start_time = time.time()
    ready = False
    while time.time() - start_time < 5:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line == f"READY:{dest_filename}":
                ready = True
                break
            elif line.startswith("ERROR:"):
                print(f"  Device error: {line}")
                return False
    
    if not ready:
        print("  Timeout waiting for device READY signal.")
        return False
        
    # Send file contents
    with open(filepath, "rb") as f:
        bytes_sent = 0
        while bytes_sent < file_size:
            chunk = f.read(512)
            if not chunk:
                break
            ser.write(chunk)
            bytes_sent += len(chunk)
            # Small delay to prevent serial buffer overflow on ESP32
            time.sleep(0.005)
            
            # Print progress bar
            pct = (bytes_sent * 100) // file_size
            sys.stdout.write(f"\r  Progress: [{'#' * (pct // 5)}{' ' * (20 - pct // 5)}] {pct}% ({bytes_sent}/{file_size})")
            sys.stdout.flush()
    print()
    
    # Wait for success response
    start_time = time.time()
    while time.time() - start_time < 5:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line == "OK:Success":
                print("  Successfully transferred!")
                return True
            elif line.startswith("ERROR:"):
                print(f"\n  Transfer failed: {line}")
                return False
                
    print("\n  Timeout waiting for transfer completion confirmation.")
    return False

def main():
    if not os.path.exists(DATA_DIR):
        print(f"Error: data directory not found at {DATA_DIR}")
        return 1
        
    print(f"Opening serial port {PORT} at {BAUDRATE} baud...")
    try:
        # Open port with a short timeout
        ser = serial.Serial(PORT, BAUDRATE, timeout=2)
        # Flush initial junk
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(1) # Let serial establish
    except Exception as e:
        print(f"Failed to open serial port: {e}")
        return 1
        
    # 1. Sync questions.csv
    csv_path = os.path.join(DATA_DIR, "questions.csv")
    if os.path.exists(csv_path):
        if not sync_file(ser, csv_path, "questions.csv"):
            print("Failed to sync questions.csv. Aborting.")
            ser.close()
            return 1
    else:
        print("Warning: questions.csv not found in data folder!")
        
    # 2. Sync all .wav files in data/hanzi
    wav_files = glob.glob(os.path.join(DATA_DIR, "hanzi", "*.wav"))
    print(f"Found {len(wav_files)} voice audio files to sync.")
    
    success_count = 0
    for wav_path in wav_files:
        filename = os.path.basename(wav_path)
        # We write them directly into the /hanzi folder on SD card
        if sync_file(ser, wav_path, filename):
            success_count += 1
            # Add a small delay between files to let the device settle
            time.sleep(0.5)
        else:
            print(f"Failed to sync {filename}, continuing with next files...")
            
    print(f"\nSync complete. Successfully transferred {success_count}/{len(wav_files)} voice files.")
    ser.close()
    return 0

if __name__ == "__main__":
    sys.exit(main())
