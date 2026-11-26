# LEGO SPIKE Prime Project

LEGO SPIKE Prime robotics project powered by [Pybricks](https://pybricks.com/) and Python.

## Prerequisites

1. **Install Pybricks Firmware on the Hub**:
   Before running scripts via the CLI, the Hub must have Pybricks firmware installed. Follow the official [Pybricks installation guide](https://code.pybricks.com/) in Chrome or Edge: connect the Hub via USB, click the firmware icon, and follow the step-by-step instructions. (Official LEGO firmware can be restored anytime).
2. **Install `pybricksdev` CLI**:
   ```bash
   pip install pybricksdev
   ```

## Files
- `main.py`: Starter script that displays a happy face on the 5x5 LED matrix and plays a chime.
- `car.py`: Driving car script using Pybricks `DriveBase` with motors on Port A and Port B.
- `index.html`: Interactive web simulator for the robot car.

## Running with Pybricks CLI
Once Pybricks firmware is installed on your Prime hub and the hub is powered on:
```bash
# Run over Bluetooth (wireless) - replace <hub-name> with your Hub name or omit -n to auto-discover:
pybricksdev run ble -n <hub-name> prime/car.py

# Or run the starter script:
pybricksdev run ble -n <hub-name> prime/main.py
```
