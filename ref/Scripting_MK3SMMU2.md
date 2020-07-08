# Scripting options for the selected printer:
### Beeper::
 - `Mute()                        ` - `Mutes the beeper`
 - `ToggleMute()                  ` - `Toggles the beeper mute`
 - `Unmute()                      ` - `Unmutes the beeper`
### Board::
 - `Quit()                        ` - `Sends the quit signal to the AVR`
 - `Reset()                       ` - `Resets the board by resetting the AVR.`
 - `WaitMs(int)                   ` - `Waits the specified number of milliseconds (in AVR-clock time)`
### Board1::
 - `Quit()                        ` - `Sends the quit signal to the AVR`
 - `Reset()                       ` - `Resets the board by resetting the AVR.`
 - `WaitMs(int)                   ` - `Waits the specified number of milliseconds (in AVR-clock time)`
### E::
 - `Reset()                       ` - `Clears the diag flag immediately`
 - `Stall()                       ` - `Sets the diag flag immediately.`
 - `ToggleStall()                 ` - `Toggles the stallguard condition on the next step.`
### EEPROM::
 - `Poke(int, int)                ` - `Pokes a value into the EEPROM. Args are (address,value)`
### EEPROM1::
 - `Poke(int, int)                ` - `Pokes a value into the EEPROM. Args are (address,value)`
### Encoder::
 - `Press()                       ` - `Presses the encoder button`
 - `PressAndRelease()             ` - `Presses the encoder button`
 - `Release()                     ` - `Releases the encoder button`
 - `TwistCCW()                    ` - `Twists the encoder once cycle counterclockwise`
 - `TwistCW()                     ` - `Twists the encoder one cycle clockwise`
### Fan::
 - `Resume()                      ` - `Resumes fan from a stall condition`
 - `Stall()                       ` - `Stalls the fan`
### Fan1::
 - `Resume()                      ` - `Resumes fan from a stall condition`
 - `Stall()                       ` - `Stalls the fan`
### Heater_B::
 - `Resume()                      ` - `Resumes auto PWM control and clears the 'stopheating' flag`
 - `SetPWM(int)                   ` - `Sets the raw heater PWM value`
 - `StopHeating()                 ` - `Stops heating, as if a thermal runaway is happening due to loose heater or thermistor`
### Heater_H::
 - `Resume()                      ` - `Resumes auto PWM control and clears the 'stopheating' flag`
 - `SetPWM(int)                   ` - `Sets the raw heater PWM value`
 - `StopHeating()                 ` - `Stops heating, as if a thermal runaway is happening due to loose heater or thermistor`
### I::
 - `Reset()                       ` - `Clears the diag flag immediately`
 - `Stall()                       ` - `Sets the diag flag immediately.`
 - `ToggleStall()                 ` - `Toggles the stallguard condition on the next step.`
### IRSensor::
 - `Set(int)                      ` - `Sets the sensor state to a specific enum entry. (int value)`
 - `Toggle()                      ` - `Toggles the IR sensor state`
### LCD::
 - `Desync()                      ` - `Simulates data corruption by desyncing the 4-bit mode`
 - `WaitForText(string, int)      ` - `Waits for a given string to appear anywhere on the specified line. A line value of -1 means any line.`
### P::
 - `Reset()                       ` - `Clears the diag flag immediately`
 - `Stall()                       ` - `Sets the diag flag immediately.`
 - `ToggleStall()                 ` - `Toggles the stallguard condition on the next step.`
### PINDA::
 - `SetMBLPoint(int, float)       ` - `Sets the given MBL point (0-48) to the given Z value`
 - `SetXYPoint(int, float, float) ` - `Sets the (0-3)rd XY cal point position to x,y. (index, x,y)`
 - `ToggleSheet()                 ` - `Toggles the presence of the steel sheet`
### Power Panic::
 - `Press()                       ` - `Simulate pressing the button`
 - `PressAndRelease()             ` - `Simulate pressing and then releasing  the button`
 - `Release()                     ` - `Simulate releasing the button`
### S::
 - `Reset()                       ` - `Clears the diag flag immediately`
 - `Stall()                       ` - `Sets the diag flag immediately.`
 - `ToggleStall()                 ` - `Toggles the stallguard condition on the next step.`
### SDCard::
 - `Mount(string)                 ` - `Mounts the specified file on the SD card.`
 - `Remount()                     ` - `Remounts the last mounted file, if any.`
 - `Unmount()                     ` - `Unmounts the currently mounted file, if any.`
### ScriptHost::
 - `SetQuitOnTimeout(bool)        ` - `If 1, quits when a timeout occurs. Exit code will be non-zero.`
 - `SetTimeoutMs(int)             ` - `Sets a timeout for actions that wait for an event`
### Serial0::
 - `SendGCode(string)             ` - `Sends the specified string as G-Code.`
 - `WaitForLine(string)           ` - `Waits for the provided line to appear on the serial output.`
 - `WaitForLineContains(string)   ` - `Waits for the serial output to contain a line with the given string.`
### TelHost::
 - `StartTrace()                  ` - `Starts the telemetry trace. You must have set a category or set of items with the -t option`
 - `StopTrace()                   ` - `Stops a running telemetry trace.`
 - `WaitFor(string, int)          ` - `Waits for a specified telemetry value to occur`
### Thermistor::
 - `Disconnect()                  ` - `Disconnects the thermistor as though it has gone open circuit`
 - `Reconnct()                    ` - `Restores the normal thermistor state`
 - `Short()                       ` - `Short the thermistor out`
### Thermistor1::
 - `Disconnect()                  ` - `Disconnects the thermistor as though it has gone open circuit`
 - `Reconnct()                    ` - `Restores the normal thermistor state`
 - `Short()                       ` - `Short the thermistor out`
### Thermistor2::
 - `Disconnect()                  ` - `Disconnects the thermistor as though it has gone open circuit`
 - `Reconnct()                    ` - `Restores the normal thermistor state`
 - `Short()                       ` - `Short the thermistor out`
### Thermistor3::
 - `Disconnect()                  ` - `Disconnects the thermistor as though it has gone open circuit`
 - `Reconnct()                    ` - `Restores the normal thermistor state`
 - `Short()                       ` - `Short the thermistor out`
### X::
 - `Reset()                       ` - `Clears the diag flag immediately`
 - `Stall()                       ` - `Sets the diag flag immediately.`
 - `ToggleStall()                 ` - `Toggles the stallguard condition on the next step.`
### Y::
 - `Reset()                       ` - `Clears the diag flag immediately`
 - `Stall()                       ` - `Sets the diag flag immediately.`
 - `ToggleStall()                 ` - `Toggles the stallguard condition on the next step.`
### Z::
 - `Reset()                       ` - `Clears the diag flag immediately`
 - `Stall()                       ` - `Sets the diag flag immediately.`
 - `ToggleStall()                 ` - `Toggles the stallguard condition on the next step.`
