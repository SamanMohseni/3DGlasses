# 3D Glasses
Synchronizes active 3D glasses with a PC, while head tracking for a lifelike 3D image generation.

## Overview
This project consists of both hardware and software parts.
The hardware part consists of an active 3D glass, with it's controller replaced to syncronize with our PC through our custom-written software.
The controller can also help locate the position of user's head by gathering information about two blinking LEDs on each side of the glasses.
The software part quickly renders an object into two quickly alternating viewpoints, each intended to be shown to one of user eyes.

## The Hardware
Image 1 shows the custom-made glass and controller.
The glass is a normal active 3D glasses with it's controller removed and connected to our circuit instead.
The controller is an AVR ATMEGA168 microcontroller. The programming code for this microcontroller in available in `main.c` in `GlassesController` folder.
This controller let's only on side of the glasses to be transparent at a time, and blocks the other.
It alternates this process in sync with the software. That is, when the render for left eye is shown, the right side glass blocks the light and vica versa.

There are two LEDs on each side of the glasses, alternatively blinking, to be positioned individually.
3 perpendicular light sensors are connected to a 16-bit Analog Digital Converter (ADC) unit. These light sensors sense the luminasity from the two LEDs and the send the information the PC.
A USB-to-UART converter is used for this purpose.

## The Software
The software quickly alternates between rendering for left and right eyes.
While the left glass is blocked, the frame for the right is shown and vica versa.
The software also reads the luminasity information for the 3 perpendicular light sensors and using some mathematical computations, calculates position of that LED relative to sensor position.
With the position of sensor hardcoded with respect to screen, the software can locate user eyes with respect to monitor.
Using these information, the software can render specifically for each eye.

## Synchronizing Glasses with Software
I've tryed several synchronization ways, such as synchronizing over USB-to-UART and using faster USB communications on FPGAs. Non of them were satisfying.
Finally, the best solution I came up with was having the software draw some small rectangle on the corner of screen which is white for one eye and black for the other.
An exta light sensot attached to the monitor quickly recognizes the change and alternates the active glass.
