# 3D Glasses
Synchronizes active 3D glasses with a PC while head tracking for a lifelike 3D visualization.

## Overview
This project comprises hardware and software components.
The hardware side consists of an active 3D glass, with its controller replaced to synchronize with our PC through our proprietary software.
The controller can also help locate the position of the user's head by gathering information about two blinking LEDs on each side of the glasses.
The software part quickly renders an object from two perspectives, alternating to match the view of each eye.

## The Hardware
Image 1 shows the customized glasses and the controller.
The glasses are standard active 3D glasses with their controller replaced with our circuitry.
The controller is an AVR ATMEGA168 microcontroller. The programming code for this microcontroller is available in `main.c` in the `GlassesController` folder.
This controller enables only one side of the glasses to be transparent while blocking the other. It alternates this process in synchronization with the software. In other words, when the render for the left eye is displayed, the right side glass blocks the light, and vice versa.

![photo_2018-07-31_01-04-43](https://github.com/SamanMohseni/3DGlasses/assets/51726090/5545dd1d-7de3-486b-942a-0897ad904a3e)
*Image 1. The active 3D glasses, connected to the external controller/positioning device.*

There are two LEDs on the two sides of the glasses, alternatively blinking, to be located individually.
Three orthogonal light sensors, connected to a 16-bit Analog Digital Converter (ADC) unit, sense the luminosity from the two LEDs and send the information to the PC via a USB-to-UART converter.

## The Software
The software quickly alternates between rendering for the left and the right eyes. It shows the frame for the right eye while the left glass is blocked, and vice versa.
The software also reads the luminosity information from the three orthogonal light sensors, and using some mathematical formulations, it calculates the position of the luminant LED relative to the sensor position.
With the position of the sensor hardcoded with respect to the screen, the software can locate the user's eyes in relation to the screen.
This positional data enables the software to tailor the rendering for each eye.
The following GIF shows a short snippet of what this software displays on the screen. When wearing the glasses, only one of the alternating renders is visible to each eye. The render is customized according to the viewer's position, which may appear skewed. However, in this snippet, the user is positioned in the upper left of the screen and will see the spere and teapot unskewed.

![GIF](https://github.com/SamanMohseni/3DGlasses/assets/51726090/7744ef42-34ef-4dc9-a212-29856dd44a45)
*A short snippet of what the software displays on the screen (reduced frame rate).*

## Synchronizing Glasses with Software
I've tried several synchronization ways, including USB-to-UART sync or faster USB communication and control through FPGAs. None of them were satisfying.
Finally, the most effective solution I came up with was having the software draw some small rectangles at the screen's corner—white for one eye and black for the other (as seen in the bottom left corner of the GIF).
An additional sensor attached to the monitor detects these changes and prompts the glasses to switch accordingly.
