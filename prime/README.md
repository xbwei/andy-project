# LEGO SPIKE Prime Project

LEGO SPIKE Prime robotics project powered by [Pybricks](https://pybricks.com/) and Python.

## Getting Started

This project uses [Pybricks](https://pybricks.com/) to program the LEGO SPIKE Prime Hub using Python.

### Files
- `main.py`: Starter script that displays a happy face on the 5x5 LED matrix and plays a chime.
- `car.py`: Driving car script using Pybricks `DriveBase` with motors on Port A and Port B.
- `index.html`: Interactive web simulator for the robot car.

### Running with Pybricks CLI
Once `pybricksdev` is installed and your Prime hub is powered on:
```bash
# Run over Bluetooth (wireless) - replace <hub-name> with your Hub name or omit -n to auto-discover:
pybricksdev run ble -n <hub-name> prime/car.py

# Or run the starter script:
pybricksdev run ble -n <hub-name> prime/main.py
```
