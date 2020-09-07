#!/bin/bash
echo "CMD: $@"
	export GALLIUM_DRIVER=swr
	export LD_LIBRARY_PATH=$1
	shift
	xvfb-run "DISPLAY=:99 $@"
exit $?
