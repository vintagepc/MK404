#!/usr/bin/python3

# A very basic example for interfacing with scripting via stdio in Python.
import subprocess;
import sys;
import codecs;

process=subprocess.Popen(['stdbuf','-oL','./MK404','Prusa_MK3S', '-g','none','--test'],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
						 universal_newlines=True)
print("Started")
enabled = False;
logged = False;
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
	if 'Script FINISHED' in line:
		process.stdin.write('Board::Quit()\n');
		process.stdin.flush();
	if 'ScriptHost PTY closed.' in line:
		break;
print("Ended");
exit(0);
