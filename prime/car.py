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

# 1 yard = 36 inches = 914 mm
ONE_YARD_MM = 914


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


def drive_until_crash_or_distance(speed=200, max_distance=ONE_YARD_MM, timeout_ms=6000):
    """Drives forward up to 1 yard (914 mm) while monitoring the bumper for collisions."""
    print("Driving forward up to 1 yard... listening for bumper crash!")
    hub.display.icon(Icon.ARROW_UP)
    car.reset()
    watch = StopWatch()
    car.drive(speed, 0)

    # Monitor the bumper continuously while moving, stopping on crash or reaching 1 yard
    while not bumper.pressed():
        if car.distance() >= max_distance:
            print("Reached 1 yard! Safely braking car.")
            car.brake()
            hub.display.icon(Icon.HAPPY)
            # Cheerful chime for reaching 1 yard target
            hub.speaker.beep(frequency=523, duration=150)
            wait(50)
            hub.speaker.beep(frequency=659, duration=150)
            wait(50)
            hub.speaker.beep(frequency=784, duration=250)
            return
        if watch.time() >= timeout_ms:
            print("Safety timeout reached. Safely braking car.")
            car.brake()
            hub.display.icon(Icon.HAPPY)
            return
        wait(10)

    # Trigger emergency stop, alert face, and crash sound
    crash_alert()


# Alias for compatibility with previous callers
drive_until_crash = drive_until_crash_or_distance

# Play a countdown beep!
hub.speaker.beep(frequency=440, duration=100)
wait(200)
hub.speaker.beep(frequency=440, duration=100)
wait(200)
hub.speaker.beep(frequency=880, duration=300)

print("Car is starting to drive!")

# Drive forward up to 1 yard (914 mm) or until hitting an obstacle
drive_until_crash_or_distance(speed=200, max_distance=ONE_YARD_MM)

print("Program complete. Car safely stopped.")
