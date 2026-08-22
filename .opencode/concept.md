## About the project

The project is a system that aids user's visual sense to locate desired objects with the use of simulated auditory cues. 

## The User

The project is centered around the idea of helping people (one of these people in this case is the user) who are visually impaired. Though, the project allows for a user to be blind-folded replicating a visually impaired user.

## User setup

The user is standing in the room. The room contains various objects.

## Project setup

The following is what have been identified that sets up the system as of the time being. 

1. **Mutiple RGB cameras** or **depth cameras**
    - These are used for object identification and location
    - Captures the user's head.
1. **Headset**
    - This are used to project audio cues that represents the objects in the room.
1. **Computer Unit**
    - This is used for computing processes. For example. computer vision is used to compute for the user's real-time head orientation from what Multiple cameras in (1) captured. 
1. **Computer networking devices.**
    - This is used for data transfer.
    - The system may use wired or wireless connections. Wireless is acceptable if the measured end-to-end latency still meets the system's latency standard; otherwise the system uses wired connections.

## User goal

The user finds desired objects blind-folded/visually impaired with the only use of the projected sounds through the headsets.

## System functions

1. The system uses **computer vision**.
    - To determine what the objects are and where they are located. 
    - To calculate the user's head rotation so that where the user is facing is indicated and position. 
2. Projects sound to the user according to the position and rotation of the user's head and the object's individual position and what the object is.
    - Sounds projected are simulation as if the objects project sounds in real life. For example, if a water bottle is at the user's top-left side of their head, a type of sound assigned to the water bottle will be projected to the top-left side of the user

## Development Stages

1. ###  Prototype

- Uses **non-specialized gears** for the system's development.
    - For example, cameras of phones may be used instead of specialized cameras.
    - If depth imaging is needed, phone cameras are used to test its capabilities limited to RGB to triangulate depth image.
- **Head Orientation**
    - Calculation of the head's orientation will be done through the cameras with computer vision.
- **Aruco markers** for pre object identification and head orientation.
    - Aruco markers are used instead in the place of actual objects.
    - Aruco marker on each side of the user's head for head orientation.

2. ### Production
- High-end / specialized gears will be used.
    - If cameras limited to RGB capabilities are enough, RGB Cameras will be used. 
- System identifies actual objects instead of the aruco markers
- If computer vision is not reliable for the head's orientation, a device attached to the headset for more acurate relative head orientation Calculation.
