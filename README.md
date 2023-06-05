# BitBlitz
Game demonstrating how to use a PCF8574 to read a button, and show a LED on the same line, at the same time.

Using an attiny85, for its lack of pins.  Soft i2c, wire-library style.
The leds turn off inperceptibly, as software polls for button changes by driving the PCF lines high.

This is all hand-soldered, using a double sided 5x7cm perfboard.
There are m2 heat inserts on the upper half of the clam.  If there is any demand, will upload a self-tapping version, and/or the FreeCAD source and pictures of the bare board.

Had called this "LightsOut" but that repo name is already taken on Github.  Oh well.
Demo of the game on YT, pileofstuff's Mailbag Monday 167, here: https://www.youtube.com/watch?v=57yn3J7gcEk

Very minimal schematic ... There are pullups on the SDA/SCL lines, of course.
![image](https://github.com/GnuReligion/BitBlitz/assets/32754836/42b12b4f-6d78-4466-b81c-abdef178e587)
