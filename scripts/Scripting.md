## Scripting

Scripting is a way to automate a sequence of actions and interations with the printer. It can be used to set up a
specific scenario for debugging, or automate the reproduction of a situation that requires precise timing.

The latter is possible because scripts are timed on AVR clock cycles. For example, wait commands wait in "AVR time"
(that is, a number of AVR cycles based on its clock frequency) instead of host time. Script actions are also executed
on this timeframe - for example, if you wait for one condition, and then take an action, the action will be implemented
on the clock cycle immediately after the condition is met. For most cases, this should be more than sufficient to not
have to worry about missing a timing.

Scripts have rudimentary validation on argument values to make sure they can be converted to the right type, and that
the function you're calling actually exists. Some actions may further restrict the allowable range of input values,
which will issue a line error when that line is run with an invalid value.

All lines are of the format Context::Action(args...) where:
- Context is typically the item you want to interact with
- Action is the action to take, and
- args... are the list of comma separated arguments.

Available actions depend on the hardware being used. You can use `--scripthelp` together with additional
setup arguments (like printer model) to see what is available within that context.

## Example actions available for the default (MK3S) printer:

```
Scripting options for the current context:
	Board::
		Quit()                        Sends the quit signal to the AVR
		Reset()                       Resets the board by resetting the AVR.
		WaitMs(int)                   Waits the specified number of milliseconds (in AVR-clock time)
	E::
		Reset()                       Clears the diag flag immediately
		Stall()                       Sets the diag flag immediately.
		ToggleStall()                 Toggles the stallguard condition on the next step.
	EEPROM::
		Poke(int, int)                Pokes a value into the EEPROM. Args are (address,value)
	Encoder::
		Press()                       Presses the encoder button
		PressAndRelease()             Presses the encoder button
		Release()                     Releases the encoder button
		TwistCCW()                    Twists the encoder once cycle counterclockwise
		TwistCW()                     Twists the encoder one cycle clockwise
	Fan::
		Resume()                      Resumes fan from a stall condition
		Stall()                       Stalls the fan
	Fan1::
		Resume()                      Resumes fan from a stall condition
		Stall()                       Stalls the fan
	Heater_B::
		Resume()                      Resumes auto PWM control and clears the 'stopheating' flag
		SetPWM(int)                   Sets the raw heater PWM value
		StopHeating()                 Stops heating, as if a thermal runaway is happening due to loose heater or thermistor
	Heater_H::
		Resume()                      Resumes auto PWM control and clears the 'stopheating' flag
		SetPWM(int)                   Sets the raw heater PWM value
		StopHeating()                 Stops heating, as if a thermal runaway is happening due to loose heater or thermistor
	IRSensor::
		Set(int)                      Sets the sensor state to a specific enum entry. (int value)
		Toggle()                      Toggles the IR sensor state
	LCD::
		Desync()                      Simulates data corruption by desyncing the 4-bit mode
		WaitForText(string, int)      Waits for a given string to appear anywhere on the specified line. A line value of -1 means any line.
	PINDA::
		SetMBLPoint(int, float)       Sets the given MBL point (0-48) to the given Z value
		SetXYPoint(int, float, float) Sets the (0-3)rd XY cal point position to x,y. (index, x,y)
		ToggleSheet()                 Toggles the presence of the steel sheet
	Power Panic::
		Press()                       Simulate pressing the button
		PressAndRelease()             Simulate pressing and then releasing  the button
		Release()                     Simulate releasing the button
	SDCard::
		Mount(string)                 Mounts the specified file on the SD card.
		Remount()                     Remounts the last mounted file, if any.
		Unmount()                     Unmounts the currently mounted file, if any.
	ScriptHost::
		SetQuitOnTimeout(bool)        If 1, quits when a timeout occurs. Exit code will be non-zero.
		SetTimeoutMs(int)             Sets a timeout for actions that wait for an event
	Serial0::
		WaitForLine(string)           Waits for the provided line to appear on the serial output.
		WaitForLineContains(string)   Waits for the serial output to contain a line with the given string.
	Thermistor::
		Disconnect()                  Disconnects the thermistor as though it has gone open circuit
		Reconnct()                    Restores the normal thermistor state
		Short()                       Short the thermistor out
	Thermistor1::
		Disconnect()                  Disconnects the thermistor as though it has gone open circuit
		Reconnct()                    Restores the normal thermistor state
		Short()                       Short the thermistor out
	Thermistor2::
		Disconnect()                  Disconnects the thermistor as though it has gone open circuit
		Reconnct()                    Restores the normal thermistor state
		Short()                       Short the thermistor out
	Thermistor3::
		Disconnect()                  Disconnects the thermistor as though it has gone open circuit
		Reconnct()                    Restores the normal thermistor state
		Short()                       Short the thermistor out
	X::
		Reset()                       Clears the diag flag immediately
		Stall()                       Sets the diag flag immediately.
		ToggleStall()                 Toggles the stallguard condition on the next step.
	Y::
		Reset()                       Clears the diag flag immediately
		Stall()                       Sets the diag flag immediately.
		ToggleStall()                 Toggles the stallguard condition on the next step.
	Z::
		Reset()                       Clears the diag flag immediately
		Stall()                       Sets the diag flag immediately.
		ToggleStall()                 Toggles the stallguard condition on the next step.
```
