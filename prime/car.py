from pybricks.hubs import PrimeHub
from pybricks.pupdevices import Motor
from pybricks.parameters import Port, Direction, Icon
from pybricks.robotics import DriveBase
from pybricks.tools import wait

# Initialize the Hub
hub = PrimeHub()

# Display a steering wheel / arrow or smile
hub.display.icon(Icon.ARROW_UP)

# Motors connected to Port A (Left) and Port B (Right)
# In standard DriveBase builds, one motor runs counterclockwise so both drive forward together
left_motor = Motor(Port.A, Direction.COUNTERCLOCKWISE)
right_motor = Motor(Port.B, Direction.CLOCKWISE)

# Standard LEGO SPIKE Prime wheels:
# - wheel_diameter: 56 mm (standard medium blue wheels)
# - axle_track: distance between the two wheels (about 112 mm by default)
car = DriveBase(left_motor, right_motor, wheel_diameter=56, axle_track=112)

# Play a countdown beep!
hub.speaker.beep(frequency=440, duration=100)
wait(200)
hub.speaker.beep(frequency=440, duration=100)
wait(200)
hub.speaker.beep(frequency=880, duration=300)

print("Car is starting to drive!")

# 1. Drive straight forward 300 mm (about 1 foot)
car.straight(300)

# 2. Spin around 360 degrees (a full donut!)
car.turn(360)

# 3. Drive backward 300 mm back to start
car.straight(-300)

# Victory beep!
hub.display.icon(Icon.HAPPY)
hub.speaker.beep(frequency=659, duration=200)
print("Driving complete!")
