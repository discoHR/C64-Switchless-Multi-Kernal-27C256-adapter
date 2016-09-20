README.txt (2016-09-10)

Reset modification for Bwack's C64 Switchless Multi Kernal Switch.

Bwack's project link:
  Github:   https://github.com/bwack/C64-Switchless-Multi-Kernal-27C256-adapter

This microcontroller program flips through the four kernals at the press of
restore key, in other words its a 'switchless' design.

Hold the restore key to enter the select mode. You will notice a fast flash
on the LED. Release the restore key when the flashing stops and press it
shortly as many times as you want to switch to the other kernals.
The LED colour will indicate which kernal is selected.
The C64 will reset to selected kernal shortly after you stop pressing the
restore key.

The Reset button (if present) behaves similar to the restore key but
unlike the restore key, the reset button is always in select mode.

There are two LED colour themes but due to hardware limitations, they don't
differ much. You can hold the restore key during power-on to toggle between
the two themes.
Theme #1: red, green, blue, cyan
Theme #2: red, lime, magenta, cyan


Changelog:

2016-09-20
- kernal index is saved only when resetting after timeout, not while browsing

2016-09-19
- finally fixed build issues, CFG file was missing
- minor changes in delays
- changed LED colours (red is always on)
- added LED flashing
- using case sensitivity in the code
- reformatted the code
- added two colour themes (use restore on power-on to change)

2016-09-18
- last used kernal is now correctly selected by default

2016-09-10
- forked BWACK's project
- blindly modified behavior of reset button to always be in select mode
