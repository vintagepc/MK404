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

Comments are prefixed with `#`

Available actions depend on the hardware being used. You can use `--scripthelp` together with additional
setup arguments (like printer model) to see what is available within that context.

## Example actions available for the default (MK3S) printer:

[See Scripting.md](https://github.com/vintagepc/MK404/wiki/Scripting)
