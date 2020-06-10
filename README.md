# MK3SIM (AKA MK404 - PRINTER NOT FOUND)
A project/repo for simulating Einsy (and eventually, other) Prusa (and eventually, other) hardware. 

While this repo is private, it's something separate to share status updates, ideas, and related things for this topic. If I invited you to join, you're welcome to sit back and enjoy the ride, or contribute to hardware implementations and ideas as you desire.

*Summary status:* Mostly functional, should now be able to boot stock Prusa Marlin build for MK3S. 
Remaining To-Dos of note:
- Beeper
- Better visuals (in progress)

*Current state of affairs*: 
- ![CI Build](https://github.com/vintagepc/MK3SIM/workflows/CI%20Build/badge.svg)
- **The simulator can complete a self test!**


![](https://user-images.githubusercontent.com/53943260/80157964-63404880-8595-11ea-9bfe-55668a0d4807.png)

- Bootloader works
- LCD works 99% of the way. Brightness support has been fixed.
- Encoder and buttons are simulated
- Power panic (fake button) is wired up
- 2 UARTS are defined but not attached externally by default. Can be enabled by passing "-S0" to access the primary serial port. UART2 is used for the MMU.
- Thermistors are defined for the bed, PINDA, ambient and hotend. Bed/PINDA read higher than expected over 40C due to code in the official firmware (prusa3d#2601)
- Fans have been attached, and can be controlled by the PWM output (or manually overridden to simulate conditions). 
- Heater behaviour has been implemented. A Hotend heater is attached and appears functional. Same goes for the heatbed.
- PINDA simulation is present for both MBL and xyz cal. Toggle "sheet on bed" flag with the 'Y' key.
- Simulated SD card, some improvements pending (#71)
- TMC2130s are sufficiently simulated for general operations.
- Motor/positioning tracking present, but crude.
- SPI flash for language support works, but must be manually flashed in two stages due to lack of DTR on PTYs.

- Full boot (Serial bug fixed)

![](https://user-images.githubusercontent.com/53943260/78808917-1f91f000-7994-11ea-87ae-fd7fa096972b.png)

- The timer bug has been resolved using a customized build of SimAVR.
- Einsy eeprom is persisted between reboots.

# Getting Started:

To get stared, clone the MK3Sim repo. the 3rdParty/simavr folder may be empty, you will need to cd into the MK3Sim checkout and run `git submodule init` and `git submodule update` from within it to pull down the correct simavr dependency.

This is now a `cmake` project and independent of simAVR. You can follow normal cmake procedures, using your favourite IDE (or with cmake-gui). The SimAVR submodule will be built automatically and taken care of for you.

You will need to use a fairly recent version of GCC/G++ (I use 7.4.0). Older versions from the 4.8 era may not support some of the syntax used. 

Windows is not officially supported/maintained but current status (as of May 2020) is that you can build and execute the program using Cygwin with the appropriate dependencies.

### Command line arguments:
- Current arguments can be viewed with the -h flag, should this README become outdated.
```
USAGE: 

   ./Einsy  [-b] [-d] ...  [-f <filename>] [-g <lite|fancy>] [-l] [-n] [-s]
            [-v] ...  [-w] [--] [--version] [-h] <Prusa_MK3S
            |Prusa_MK3SMMU2>


Where: 

   -b,  --bootloader
     Run bootloader on first start instead of going straight to the
     firmware.

   -d,  --debug  (accepted multiple times)
     Increases debugging output, where supported.

   -f <filename>,  --firmware <filename>
     hex/afx Firmware file to load (default MK3S.afx)

   -g <lite|fancy>,  --graphics <lite|fancy>
     Whether to enable fancy (advanced) or lite (minimal advanced) visuals.
     If not specified, only the basic 2D visuals are shown.

   -l,  --loadfw
     Directs the printer to load the default firmware file. (-f implies -l)
     If neither -l or -f are provided, the printer executes solely from its
     persisted flash.

   -n,  --no-hacks
     Disable any special hackery that might have been implemented for a
     board to run its manufacturer firmware, e.g. if you want to run stock
     marlin and have issues. Effects depend on the board and firmware.

   -s,  --serial
     Connect a printer's serial port to a PTY instead of printing its
     output to the console.

   -v,  --verbose  (accepted multiple times)
     Increases verbosity of the output, where supported.

   -w,  --wait
     Wait after the printer (and any PTYs) are set up but before starting
     execution.

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.

   <Prusa_MK3S|Prusa_MK3SMMU2>
     Model name of the printer to run
```
### Mouse Functions:
#### LCD Window:
- Scroll wheel moves the encoder.
- Left button down/up controls click/release
#### Printer visualization Window (when not in nozzle-cam mode):
- Left button: Rotate
- Middle button: Pan
- Right button & scroll wheel: Zoom/Z travel

### Key functions:
- `w` and `s` move the encoder. 
- `enter` is the encoder button
- `q` quits.
- `p` simulates a power panic
- `c` inserts/removes the SD card
- `d` spews program counter output to console.
- `1` changes the LCD color scheme.
- `r` resets the printer (X button)
- `t` does a factory reset (reset & hold encoder)
- `h` just holds the encoder.
- `y` adds/removes the steel sheet from the heatbed for PINDA MBL or XYZ cal
- `f` toggles filament presence
- `z` pauses Einsy execution without stopping GL interactivity (so you can still pan/scroll/zoom the fancy graphics)
- `l` clears the current print visual from the print bed.
- `n` toggles "Nozzle cam" mode
- `` ` `` (backtick) resets the camera view
