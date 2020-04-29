# MK3SIM
A project/repo for simulating Einsy hardware. 

I figured that I'd just start something separate to share status updates, ideas, and related things for this topic. If I invited you you're welcome to sit back and enjoy the ride, or contribute to hardware implementations.

*Summary status:* Mostly functional, should now be able to boot stock Prusa Marlin build for MK3S. 
Remaining To-Dos of note:
- Filament sensor (Works well enough for testing, better features to add)
- Beeper
- Better visuals


*Current state of affairs*:
- **The simulator can complete a self test!**

![](https://user-images.githubusercontent.com/53943260/80157964-63404880-8595-11ea-9bfe-55668a0d4807.png)

- Bootloader works
- LCD works 99% of the way. Brightness support has been fixed.
- Encoder and buttons are simulated
- Power panic (fake button) is wired up
- 4 UARTS are defined but not attached externally by default. Can be enabled by passing "-S0"
- Thermistors are defined for the bed, PINDA, ambient and hotend. Bed/PINDA read higher than expected over 40C due to code in the official firmware (prusa3d#2601)
- Fans have been attached, and can be controlled by the PWM output (or manually overridden to simulate conditions). 
- Heater behaviour has been implemented. A Hotend heater is attached and appears functional. Same goes for the heatbed.
- PINDA simulation is present for both MBL and xyz cal. Toggle "sheet on bed" flag with the 'Y' key.
- Simulated SD card, some improvements pending (#71)
- TMC2130s are sufficiently simulated for general operations.
- Motor/positioning tracking present, but crude.
- SPI flash is present. Still some issues to work out with using it and writing it.

- Full boot (Serial bug fixed)

![](https://user-images.githubusercontent.com/53943260/78808917-1f91f000-7994-11ea-87ae-fd7fa096972b.png)

- The timer bug has been resolved using a customized build of SimAVR.
- bootloader can be run with `-b` but is off by default for faster boot times.
- Einsy eeprom is persisted between reboots.

