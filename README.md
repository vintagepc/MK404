# MK3SIM
A project/repo for simulating Einsy hardware. 

Not much to see here yet, I figured that I'd just start something separate to share status updates, ideas, and related things for this topic. If I invited you you're welcome to sit back and enjoy the ride, or (once I actually get somewhere meaningful and push up some cleaned-up code) contribute to hardware implementations.

Current state of affairs:
- Bootloader works:

![](images/bootloader.png)

- LCD works (partially)
- Encoder and buttons are simulated
- Power panic (fake button) is wired up
- Firmware starts to boot, detects missing SPI flash:

![](images/W25X20CL.png)

- Bootloop issue (#11) in SerialPrintPGM.
