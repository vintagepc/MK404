```

USAGE: 

   ./MK404  [--markdown] [-b] [-d] ...  [-f <filename>] [-g <none|lite
            |fancy>] [--gdb] [--image-size <128M|1G|256M|2G|32M|512M|64M>]
            [-l] [-m] [-n] [--script <filename.txt>] [--scripthelp] [-s]
            [--sdimage <filename.img>] [-t <string>] ...  [--tracerate
            <integer>] [-v] ...  [-w] [--] [--version] [-h] <Prusa_MK3
            |Prusa_MK3MMU2|Prusa_MK3S|Prusa_MK3SMMU2>


Where: 

   --markdown
     Used to auto-generate the items in refs/ as markdown

   -b,  --bootloader
     Run bootloader on first start instead of going straight to the
     firmware.

   -d,  --debug  (accepted multiple times)
     Increases debugging output, where supported.

   -f <filename>,  --firmware <filename>
     hex/afx/elf Firmware file to load (default MK3S.afx)

   -g <none|lite|fancy>,  --graphics <none|lite|fancy>
     Whether to enable fancy (advanced) or lite (minimal advanced) visuals.
     If not specified, only the basic 2D visuals are shown.

   --gdb
     Enable SimAVR's GDB support

   --image-size <128M|1G|256M|2G|32M|512M|64M>
     Specify a size for a new SD image. You must specify an image with
     --sdimage

   -l,  --loadfw
     Directs the printer to load the default firmware file. (-f implies -l)
     If neither -l or -f are provided, the printer executes solely from its
     persisted flash.

   -m,  --mute
     Tell a printer to mute any audio it may produce.

   -n,  --no-hacks
     Disable any special hackery that might have been implemented for a
     board to run its manufacturer firmware, e.g. if you want to run stock
     marlin and have issues. Effects depend on the board and firmware.

   --script <filename.txt>
     Execute the given script. Use --scripthelp for syntax.

   --scripthelp
     Prints the available scripting commands for the current
     printer/context

   -s,  --serial
     Connect a printer's serial port to a PTY instead of printing its
     output to the console.

   --sdimage <filename.img>
     Use the given SD card .img file instead of the default

   -t <string>,  --trace <string>  (accepted multiple times)
     Enables VCD traces for the specified categories or IRQs. use '-t ?' to
     get a printout of available traces

   --tracerate <integer>
     Sets the logging frequency of the VCD trace (default 100uS)

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

   <Prusa_MK3|Prusa_MK3MMU2|Prusa_MK3S|Prusa_MK3SMMU2>
     Model name of the printer to run


   MK404 is an 8-bit AVR based 3D printer simulator for firmware debugging
   and tinkering.

   Copyright 2020 VintagePC <https://github.com/vintagepc/> with
   contributions from leptun, wavexx and 3d-gussner.

```
