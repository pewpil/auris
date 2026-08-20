## About the project

The project is a system that aids user's visual sense to locate desired objects with the use of simulated auditory cues. 

## The User

The project is centered around the idea of helping people (one of these people in this case is the user) who are visually impaired. Though, the project allows for a user to be blind-folded replicating a visually impaired user.

## User setup

The user is seated. In front of them, is a table where various objects are on it.

## Project setup

The following is what have been identified that sets up the system as of the time being. 

1. **Mutiple cameras**
    - These are used for object identification and location
    - Captures the user's head.
    - **Supplementary items:**
    1. **Tripod**
        - Helps the cameras to be positioned 
2. **Headset**
    - This are used to project audio cues that represents the objects on the table.
3. **Computer System Unit**
    - This is used for computing processes. For example. computer vision is used to compute for the user's real-time head rotation from what Multiple cameras in (1) captured. 
4. **Computer networking devices.** Not yet finalized
    - This is used for data transfer.
    - For the time being, (1), (2) and (3) are wirelessly connected. Though this may be changed to wired connections due to delay.

## System functions

1. The system uses **computer vision**.
    - To determine what the objects are and where they are located. 
    - To calculate the user's head rotation so that where the user is facing is indicated and position. 
2. Projects sound to the user according to the position and rotation of the user's head and the object's individual position and what the object is.
    - Sounds projected are simulation as if the objects project sounds in real life. For example, if a water bottle is at the user's top-left side of their head, a type of sound assigned to the water bottle will be projected to the top-left side of the user
