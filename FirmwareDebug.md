Quick howto on debug/tracing problems in the actual compiled firmware:
- You need the intermediate ELF binary. Enable verbose output in Arduino IDE and copy the .ino.elf file from /tmp to your working dir.
- Dump its contents. ( `avr-objdump -d <file.elf> > file.txt` ) 
- Run SimAVR/the firmware with the program counter logging enabled (set gbPrintPC to 1 or push `d` while running). Capture to text with `tee`
- When the problem (waitlock, etc) occurs, quit. You can now use the logged program counter value to step backwards through the disassembly and find where the problem lies.

