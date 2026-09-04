# Auris

## The User

The user is visually impaired or they are blindfolded to replicate a visually impaired user.

## Environment

The user is present in a room with obstacles and the room contains various objects

## The goal

The user will locate their desired object

## Navigation Aid

Sounds are projected by the head-mounted wearable's hearing apparatus according to the desired object's position and the user's head position and orientation in which the user can follow.

## System modules

### The head-mounted wearable 

The user is equipped with a head-mounted wearable that aids their navigation in locating the desired object.

#### Functionality

1. Head position and orientation: the wearable is able to extract the position and orientation of the user's head.
2. Vision.
    1. Depth: The wearable is able to sense obstacles (e.g. table, wall) by its depth
    2. RGB: Supplies vision with real-time RGB imagery.
3. Broadcasts sounds. 

### Computing Unit

Processes data for computer vision, audio simulation and maps out the room with its obstacles and objects from the depth the head-mounted wearable sensed.

1. Computer Vision
    - From the mapped out, objects may have been sensed as well. Computer vision process is used for to identify objects from the mapped-out room.
2. Audio Simulation
    - Determines how the music will appropriately be projected according to the user's head position and orientation.
    - The sounds projected are a simulation as if the objects emit sounds in real life. For example, if the object is at the user's left side then they will hear specific sound assigned to that object to their left side.
3. Mapped-out room
    - As the user looks around, depth sensed by the wearable will be stored to map out the room
4. Object Localization
    - Identifies where the object is out of the depth sensed. If realized that the object is not where at the identified position, the user is prompted/signaled to look around more.
