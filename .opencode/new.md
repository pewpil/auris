## User 

The user is visually impaired or blindfolded to replicate a visually impaired person

## Environment

The room contains obstacles (tables, walls, etc.)

## User Goal 

The user is tasked to go from any point in the room to another point in the room.

## Navigation Aid

The user is equipped with a head-mounted wearable and a pointer where sound is projected to the wearable's audio apparatus which then the user can use to aid navigation.

## User equipment

### Head-mounted wearable

It is 3d printed along the components required and it is worn by the user.

#### Functionalities

1. **Head position and orientation**
- The wearable is able to extract the user's head position and orientation
2. **Audio simulation**
- The wearable is equipped with an audio apparatus for sound to be broadcasted to the user's ears

### Pointer

#### Functionalities

1. **Laser**
- The pointer shoots out an imaginary/invisible laser towards the obstacles
2. **Button**
- The pointer is only shot when the pointer's button is pushed. Laser is stopped shooting when the button is let go.

## System Goal

The sound projected is according to the user's head relative to where the laser shot landed. For example, where and how loud the audio is projected according to the following variables: where the laser shot lands (a), where the user is facing (b). If (a) is far and to their left, you would expect that the sound is projected on the user's left side faintly but loudly if (a) is not far. However, if (b) is right, they would hear it as if the sound is at their back since the laser landed would end up behind them.

The sound is also placed **vertically**: whether the shot lands above, at, or below the user's head level is conveyed (e.g., aiming down a staircase versus at a wall sign at eye height). The pointer is not fixed at chest height — it may be aimed up or down freely, and the placement follows the hit in 3-D.

Orientation is full 3-D — yaw, pitch, and roll — for both the head and the pointer. The projected sound must replicate how the laser hit would actually be heard in real life.
