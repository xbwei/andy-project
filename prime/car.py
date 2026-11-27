from pybricks.hubs import PrimeHub
from pybricks.pupdevices import ForceSensor, Motor
from pybricks.parameters import Port, Direction, Icon
from pybricks.robotics import DriveBase
from pybricks.tools import StopWatch, wait

# Initialize the Hub
hub = PrimeHub()

# Display a steering wheel / forward arrow
hub.display.icon(Icon.ARROW_UP)

# Motors connected to Port A (Left) and Port B (Right)
# In standard DriveBase builds, one motor runs counterclockwise so both drive forward together
left_motor = Motor(Port.A, Direction.COUNTERCLOCKWISE)
right_motor = Motor(Port.B, Direction.CLOCKWISE)

# Front bumper button (ForceSensor) connected to Port C with wheel bumper
bumper = ForceSensor(Port.C)

# Standard LEGO SPIKE Prime wheels:
# - wheel_diameter: 56 mm (standard medium blue wheels)
# - axle_track: distance between the two wheels (about 112 mm by default)
car = DriveBase(left_motor, right_motor, wheel_diameter=56, axle_track=112)


def crash_alert():
    """Brakes the car immediately, displays an alert face, and plays a crash sound."""
    car.brake()
    hub.display.icon(Icon.SAD)
    print("CRASH! Front bumper triggered on Port C.")

    # Play a low-pitch descending crash sound effect
    hub.speaker.beep(frequency=320, duration=150)
    wait(30)
    hub.speaker.beep(frequency=220, duration=200)
    wait(30)
    hub.speaker.beep(frequency=130, duration=400)


def drive_until_crash(speed=200, max_distance=1000, timeout_ms=5000):
    """Drives forward until the front bumper hits an obstacle or safety limit is reached."""
    print("Driving forward... listening for bumper crash!")
    hub.display.icon(Icon.ARROW_UP)
    car.reset()
    watch = StopWatch()
    car.drive(speed, 0)

    # Monitor the bumper continuously while moving, guarded by max travel and timeout
    while not bumper.pressed():
        if car.distance() >= max_distance or watch.time() >= timeout_ms:
            print("Safety limit reached without collision. Safely braking car.")
            car.brake()
            hub.display.icon(Icon.HAPPY)
            return
        wait(10)

    # Trigger emergency stop, alert face, and crash sound
    crash_alert()


# Play a countdown beep!
hub.speaker.beep(frequency=440, duration=100)
wait(200)
hub.speaker.beep(frequency=440, duration=100)
wait(200)
hub.speaker.beep(frequency=880, duration=300)

print("Car is starting to drive!")

# Drive forward until hitting an obstacle or reaching safety limit
drive_until_crash(speed=200)

print("Program complete. Car safely stopped.")
