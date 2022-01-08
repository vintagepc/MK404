#!/usr/bin/python3

# A very basic example for interfacing with scripting via stdio in Python.
import subprocess;
import sys;

process=subprocess.Popen(['stdbuf','-oL','./MK404','Prusa_MK3S', '-g','none','--test'],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
						 universal_newlines=True)
print("Started")
enabled = False;
logged = False;
closed = False;
done = False;
for line in iter(process.stdout.readline, b''):
	if line.rstrip():
		print('>>> -{}-'.format(line.rstrip()))
	if '** ScriptHost: stdin READY **' in line:
		enabled = True;
	if 'Starting atmega2560 execution...' in line:
		if (not enabled):
			sys.exit(1); # failed, scripthost wasn't ready.
		print('Sending command\n');
		process.stdin.write('ScriptHost::Log(test message)\n');
		process.stdin.flush();
	if 'ScriptLog: test message' in line:
		logged = True;
	if 'ScriptHost: stdin EOF' in line:
		closed = True;
	if 'Script FINISHED' in line and not done:
		done = True;
		process.stdin.write('Board::Quit()\n');
		process.stdin.flush();
		process.stdin.close();
	if 'ScriptHost PTY closed.' in line:
		break;
print("Ended");
if (not closed or not done):
	exit(1);
exit(0);
