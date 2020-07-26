![](assets/Logo.png)

# MK404 - PRINTER NOT FOUND (formerly MK3SIM)
A project/repo for simulating Einsy (and eventually, other) Prusa (and eventually, other) hardware.

*Summary status:* "Mostly" functional, it runs stock firmware very close to the real thing, and should now be able to boot stock Prusa Marlin build for MK3S. "Mostly" because while the overall system is probably usable for the majority of use cases, some aspects of the hardware are simulated only to the extent necessary to get the system working. For example, the TMC "stallguard" register is minimally implemented, and the microstep count is not.

Remaining To-Dos of note:
- [See open issues...](https://github.com/vintagepc/MK404/issues/)

*Current state of affairs and features*:
- ![CI Build](https://github.com/vintagepc/MK3SIM/workflows/CI%20Build/badge.svg) ![CPPCheck](https://github.com/vintagepc/MK3SIM/workflows/CPPCheck/badge.svg)
- **The simulator can complete a self test!**


![](https://user-images.githubusercontent.com/53943260/80157964-63404880-8595-11ea-9bfe-55668a0d4807.png)

- Fancy graphics:

![](images/Advanced_gfx.png)

- Bootloader works
- LCD works. Brightness support has been fixed.
- Encoder and buttons are simulated
- Power panic (fake button) is wired up
- 2 UARTS are defined but not attached externally by default. Can be enabled by passing "-s" to access the primary serial port. UART2 is used for the MMU.
- Thermistors are defined for the bed, PINDA, ambient and hotend. Bed/PINDA read higher than expected over 40C due to code in the official firmware (prusa3d#2601)
- Fans have been attached, and can be controlled by the PWM output (or manually overridden to simulate conditions).
- Heater behaviour has been implemented. A Hotend heater is attached and appears functional. Same goes for the heatbed.
- PINDA simulation is present for both MBL and xyz cal. Toggle "sheet on bed" flag with the 'Y' key.
- Simulated SD card
- TMC2130s are sufficiently simulated for general operations.
- Motor/positioning tracking present.
- SPI flash for language support works, but because PTYs lack DTR it must be done either in two stages, or using the example script to trigger a reset at the right time.
- The timer bug has been resolved using a customized build of SimAVR.
- Flash/eeprom is persisted between reboots.
- Virtual MMU support:

![](images/MMU2.png)

- The MMU supports multicolour printing:
![](https://user-images.githubusercontent.com/53943260/84335826-c432d880-ab63-11ea-9534-6cc61ae1a745.png)

# Getting Started:

To get stared, clone the MK404 repo. the 3rdParty/simavr folder may be empty, you will need to `cd` into the checkout and run `git submodule init` and `git submodule update` from within it to pull down the correct simavr dependency. This should also initialize the `tinyobjloader` and `TCLAP` dependencies.

This is now a `cmake` project and independent of simAVR. You can follow normal cmake procedures, using your favourite IDE (or with cmake-gui). The submodules will be built automatically and taken care of for you.

You will need to use a fairly recent version of GCC/G++ (I use 7.4.0). Older versions from the 4.8 era may not support some of the syntax used. Newer versions (G++ 10) may complain about new warnings that are not present in 7.4. You can set a CMAKE option to disable -Werror in this case.

#### Windows support:
Windows is not officially supported/maintained but current status (as of May 2020) is that you can build and execute the program using Cygwin with the appropriate dependencies. You will need to change some of the SimAVR code as described in http://fabricesalvaire.github.io/simavr/gitorious-get-started.html to get it to compile. MK404 depends on libelf, freeglut, GL, GLEW, bintools, SDL-audio, and pthread. Due to SimAVR's heavy dependency on POSIX features (some of which we also use) and the fact I run linux as my day-to-day OS, it is unlikely there will be a native Windows build in the forseeable future. If you encounter issues with the Windows build, you are welcome to file a bug report and I'll make a best effort to address it, but I cannot make any guarantees.

#### Tips:
By default, the flash and EEPROM will be blank on first launch or if you delete the associated .bin files.
You will need to choose and load a firmware file (.afx, .hex) at least once with `-f` or by flashing it from the bootloader `-b` with serial (`-s`) enabled.

You can make an SD card image and copy files to it using `mcopy`, or by placing them in the SDCard folder and running the appropriate step in the makefile.

### Command line arguments:
- Current arguments can be viewed with the -h flag, should this README become outdated.

[Argument reference](ref/CommandLine.md)

### Scripting:
- The simulator supports basic command line scripting. At some point we will probably also support more advanced VCD input options.

[General scripting info](scripts/Scripting.md)

[Script command reference](ref/Scripting.md)

### Telemetry:
- The simulator's VCD trace system is categorized so you can enable groups or individual trace items from the command line with the `-t` argument.

[Trace option reference](ref/TraceOptions.md)

### Mouse Functions:
#### LCD Window:
- Scroll wheel moves the encoder.
- Left or right button down/up controls click/release
- Middle button opens a context menu with actions, which may offer additional functionality beyond keyboard shortcuts. Items typically have similar functionality and labelling to their scripting counterparts.

#### Printer visualization Window (when not in nozzle-cam mode):
- Left button: Rotate
- Middle button: Pan
- Right button + move (or scroll wheel): Zoom/Z travel

### Key functions:
- `w` and `s` move the encoder.
- `enter` is the encoder button
- `q` quits.
- `p` simulates a power panic
- `c` inserts/removes the SD card
- `1` changes the LCD color scheme.
- `r` resets the printer (X button)
- `t` does a factory reset (reset & hold encoder)
- `h` just holds the encoder.
- `m` toggles audio muting (beeps)
- `y` adds/removes the steel sheet from the heatbed for PINDA MBL or XYZ cal
- `f` toggles filament presence
- `F` toggles filament presence at FINDA (MMU2 only)
- `j` toggles a filament jam (presence but no motion) on the PAT9125
- `z` pauses Einsy execution without stopping GL interactivity (so you can still pan/scroll/zoom the fancy graphics)
- `l` clears the current print visual from the print bed (may cause glitches if used mid print)
- `n` toggles "Nozzle cam" mode
- `a` restores FINDA (MMU2) and IR sensor to "auto" control
- `3/4/5` are MMU buttons 1, 2 and 3, respectively.
- `` ` `` (backtick) resets the camera view

# Contribution sticky notes:

If you are interested in submitting a fix or improvement, here are a few tidbits you may find useful to know:

- Reference documentation in ref/ (scripting, telemetry, usage readme) are automatically regenerated. The git-bot will push them after it finishes test building your changes for a PR. If you are outside the worktree and do not run workflows, run the appropriate makefile step to regenerate.
- Your changes are expected to compile with -Wall and -Werror on GCC-7.4.0 In addition, pull requests will be run through CPPCheck to look for additional possible pitfalls. These are the default options when building, and CPP check is provided as a convenience target in the makefile.
- As this is a multithreaded application, it is *strongly* suggested your run your code at least once with -fsanitize=thread enabled to look for and fix thread race conditions. This is especially important for the GL Draw() functions as these are called from a separate GL context, less so if your changes do not cross thread boundaries. If, when doing this, you find any not caused in your own changes, please don't hesitate to create an issue so it can be looked at. Run cmake with `-DENABLE_THREAD_SANITY=1` to enable this.
- Where possible, please follow established patterns and styles for maintainability. If you have a reason for not doing so, be sure to explain it with code comments. Remember, code is often written once but needs to be read many times, so the clearer you make it, or the more familiar the pattern already is, the better.
