# MK3SIM
A project/repo for simulating Einsy hardware. 

Not much to see here yet, I figured that I'd just start something separate to share status updates, ideas, and related things for this topic. If I invited you you're welcome to sit back and enjoy the ride, or contribute to hardware implementations.

*NOTE:*
You currently cannot run an as-is Marlin build. See the following notes on the serial and timer bootloop issues.

*Current state of affairs*:
- Bootloader works:

![](images/bootloader.png)

- LCD works 95% of the way. There are still some blanking and custom character glitches. Brightness is supported but there are probably still wiring bugs.
- Encoder and buttons are simulated
- Power panic (fake button) is wired up
- 4 UARTS are defined but not attached externally.
- Thermistors are defined for the bed, PINDA, ambient and hotend. Bed/PINDA read higher than expected over 40C due to code in the official firmware (prusa3d#2601)
- Fans have been attached as a starting point. PWM still needs to be attached but they will read values if set directly.
- Firmware starts to boot, detects missing SPI flash (A stub has been wired in to serve as a starting point):

![](images/W25X20CL.png)

- Bootloop issue (#11) in MarlinSerial::write. Therefore, the MK3S.afx file has the offending write loop commented out for now. (You will not get serial output until this is fixed. But, you *can* still talk to the bootloader over serial, if you connect the uart-pty in setupSerial(). It's disconnected because it prints to console by default, and I was getting annoyed by having to close the terminal windows all the time.

- Bypassing serial results in a boot:

![](https://user-images.githubusercontent.com/53943260/78808917-1f91f000-7994-11ea-87ae-fd7fa096972b.png)

- The Timer bug (#1) has also been worked around in the provided .afx file by calling timer0_init() at the beginning of setup() in marlin_main.cpp. A further fix is in progress. 
