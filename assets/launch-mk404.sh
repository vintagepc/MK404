#!/bin/bash
# Launch script for the MK404 instance inside the flatpak.

DATA_DIR=$HOME/MK404

echo "Flatpak Working directory: $DATA_DIR"

if [ $(ls -A "$DATA_DIR" | wc -l) -eq 0 ]; then
	cp -v /app/bin/*.hex "$DATA_DIR"
	cp -v /app/bin/*.afx "$DATA_DIR"
fi

cd /app/bin

exec /app/bin/MK404 "$@"
