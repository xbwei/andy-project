from pybricks.hubs import PrimeHub
from pybricks.parameters import Icon
from pybricks.tools import wait

# Initialize the LEGO SPIKE Prime Hub
hub = PrimeHub()

# Display a happy face on the 5x5 light matrix
hub.display.icon(Icon.HAPPY)

# Play a cheerful starter sound
hub.speaker.beep(frequency=523, duration=150)  # C5
wait(50)
hub.speaker.beep(frequency=659, duration=150)  # E5
wait(50)
hub.speaker.beep(frequency=784, duration=250)  # G5

print("Hello from LEGO SPIKE Prime!")
