# MK3SIM
A project/repo for simulating Einsy hardware. 

I figured that I'd just start something separate to share status updates, ideas, and related things for this topic. If I invited you you're welcome to sit back and enjoy the ride, or contribute to hardware implementations.

*Summary status:* Mostly functional, should now be able to boot stock Prusa Marlin build for MK3S. 
Remaining To-Dos of note:
- PINDA
- SD Card
- Filament sensor
- TMC2130
- Beeper
- Motors/positioning
- Better visuals


*Current state of affairs*:
- Bootloader works:

![](images/bootloader.png)

- LCD works 99% of the way. Brightness support has been fixed.
- Encoder and buttons are simulated
- Power panic (fake button) is wired up
- 4 UARTS are defined but not attached externally.
- Thermistors are defined for the bed, PINDA, ambient and hotend. Bed/PINDA read higher than expected over 40C due to code in the official firmware (prusa3d#2601)
- Fans have been attached, and can be controlled by the PWM output (or manually overridden to simulate conditions). 
- Heater behaviour has been implemented. A Hotend heater is attached and appears functional, but the bed heating still has some issues to work out (#45)

- Full boot (Serial bug fixed)

![](https://user-images.githubusercontent.com/53943260/78808917-1f91f000-7994-11ea-87ae-fd7fa096972b.png)

- The timer bug has been resolved using a customized build of SimAVR.
- bootloader can be run with `-b` but is off by default for faster boot times.
- Einsy eeprom is persisted between reboots.
