# MK3SIM
Efforts to simulate MK3 Hardware

Not much to see here yet, I figured that I'd just start something separate to share status updates and collaborate on this when we get that far.

Current state of affairs:
- Bootloader works:

![](images/bootloader.png)

- LCD works (partially)
- Encoder and buttons are simulated
- Power panic (fake button) is wired up
- Firmware starts to boot, detects missing SPI flash:

![](images/W25X20CL.png)

- Bootloop issue (#11) in SerialPrintPGM.
