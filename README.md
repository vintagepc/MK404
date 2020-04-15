# MK3SIM
A project/repo for simulating Einsy hardware. 

Not much to see here yet, I figured that I'd just start something separate to share status updates, ideas, and related things for this topic. If I invited you you're welcome to sit back and enjoy the ride, or contribute to hardware implementations.

*NOTE:*
You currently cannot run an as-is Marlin build. See the following notes on the serial and timer bootloop issues.

*Current state of affairs*:
- Bootloader works:

![](images/bootloader.png)

- LCD works 99% of the way. Brightness support has been fixed.
- Encoder and buttons are simulated
- Power panic (fake button) is wired up
- 4 UARTS are defined but not attached externally.
- Thermistors are defined for the bed, PINDA, ambient and hotend. Bed/PINDA read higher than expected over 40C due to code in the official firmware (prusa3d#2601)
- Fans have been attached, and can be controlled by the PWM output (or manually overridden to simulate conditions). 
- Firmware starts to boot, detects missing SPI flash (A stub has been wired in to serve as a starting point):
- 
![](images/W25X20CL.png)

- Bootloop issue (#11) in MarlinSerial::write. Therefore, the MK3S.afx file has the offending write loop commented out for now. (You will not get serial output until this is fixed. But, you *can* still talk to the bootloader over serial, if you connect the uart-pty in setupSerial(). It's disconnected because it prints to console by default, and I was getting annoyed by having to close the terminal windows all the time.

- Bypassing serial results in a boot:

![](https://user-images.githubusercontent.com/53943260/78808917-1f91f000-7994-11ea-87ae-fd7fa096972b.png)

- The timer bug has been resolved using a customized build of SimAVR.
- bootloader can be run with `-b` but is off by default for faster boot times.
- Einsy eeprom is persisted between reboots.
