#!/bin/bash
echo "CMD: $@"
	export GALLIUM_DRIVER=swr
	export DISPLAY=:99
	export LD_LIBRARY_PATH=$1
	shift
	xvfb-run "$@"
exit $?
