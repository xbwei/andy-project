# LEGO SPIKE Prime Project

Welcome to Andy's LEGO SPIKE Prime robotics project!

## Getting Started

This project uses [Pybricks](https://pybricks.com/) to program the LEGO SPIKE Prime Hub using Python.

### Files
- `main.py`: Starter script that displays a happy face on the 5x5 LED matrix and plays a chime.
- `car.py`: Driving car script using Pybricks `DriveBase` with motors on Port A and Port B.

### Running with Pybricks CLI
Once `pybricksdev` is installed and your Prime hub is powered on:
```bash
# Run over Bluetooth (wireless):
pybricksdev run ble -n Andy-prime prime/car.py

# Or run the starter script:
pybricksdev run ble -n Andy-prime prime/main.py
```
