G28 W
M73 P0 R0
M73 Q0 S0
M201 X1000 Y1000 Z1000 E5000 ; sets maximum accelerations mm/sec^2
M203 X200 Y200 Z12 E120 ; sets maximum feedrates mm/sec
M204 P1250 R1250 T1250 ; sets acceleration (P T) and retract acceleration (R) mm/sec^2
M205 X8.00 Y8.00 Z0.40 E4.50 ; sets the jerk limits mm/sec
M205 S0 T0 ; sets the minimum extruding and travel feed rate mm/sec
M107 ; disable fan
M862.3 P "MK3S" ; printer model check
M862.1 P0.4 ; nozzle diameter check
M115 U3.8.1 ; tell printer latest fw version
G90 ; use absolute coordinates
M83 ; extruder relative mode
M302 P1
G92 E0.0 ; reset extrusion distance
G1 Y-3.0 F1000.0 ; go outside print area
G1 E2 F1000 ; de-retract and push ooze
G1 X20.0 E6  F1000.0 ; fat 20mm intro line @ 0.30
G1 X60.0 E3.2  F1000.0 ; thin +40mm intro line @ 0.08
G1 X100.0 E6  F1000.0 ; fat +40mm intro line @ 0.15
G1 E-0.8 F2100; retract to avoid stringing
G1 X99.0 E0 F1000.0 ; -1mm intro line @ 0.00
G1 X110.0 E0 F1000.0 ; +10mm intro line @ 0.00
G1 E0.6 F1500; de-retract
G92 E0.0 ; reset extrusion distance
; Final print adjustments
;M221 S95
G21 ; set units to millimeters
G90 ; use absolute coordinates
M83 ; use relative distances for extrusion
M900 K0.08; Filament gcode
G92 E0.0
G1 E-0.40000 F4800.00000 ; retract
G1 Z0.300 F12000.000 ; lift Z
G1 X121.039 Y101.039 ; move to first perimeter point
G1 Z0.200 ; restore layer Z
G1 E0.40000 F1500.00000 ; unretract
M204 S1000 ; adjust acceleration
G1 F1800.000
G1 X128.961 Y101.039 E0.26814 ; perimeter
G1 X128.961 Y108.961 E0.26814 ; perimeter
G1 X121.039 Y108.961 E0.26814 ; perimeter
G1 X121.039 Y101.099 E0.26611 ; perimeter
M204 S1250 ; adjust acceleration
G1 X120.632 Y100.632 F12000.000 ; move to first perimeter point
M204 S1000 ; adjust acceleration
G1 F1800.000
G1 X129.368 Y100.632 E0.29570 ; perimeter
G1 X129.368 Y109.368 E0.29570 ; perimeter
G1 X120.632 Y109.368 E0.29570 ; perimeter
G1 X120.632 Y100.692 E0.29367 ; perimeter
M204 S1250 ; adjust acceleration
G1 X120.225 Y100.225 F12000.000 ; move to first perimeter point
M204 S1000 ; adjust acceleration
G1 F1800.000
G1 X129.775 Y100.225 E0.32326 ; perimeter
G1 X129.775 Y109.775 E0.32326 ; perimeter
G1 X120.225 Y109.775 E0.32326 ; perimeter
G1 X120.225 Y100.285 E0.32123 ; perimeter
M204 S1250 ; adjust acceleration
G1 X120.611 Y100.329 F12000.000 ; move inwards before travel
G1 E-0.40000 F4800.00000 ; retract
G1 Z0.500 F12000.000 ; lift Z
G1 X128.839 Y102.011 ; move to first infill point
G1 Z0.200 ; restore layer Z
G1 E0.40000 F1500.00000 ; unretract
M204 S1000 ; adjust acceleration
G1 F1800.000
G4 S0
M73 P25 R0
G1 X128.172 Y101.344 E0.03291 ; infill
G1 X127.578 Y101.344 E0.02072 ; infill
G1 X128.656 Y102.422 E0.05318 ; infill
G1 X128.656 Y103.015 E0.02072 ; infill
G1 X126.985 Y101.344 E0.08249 ; infill
G1 X126.391 Y101.344 E0.02072 ; infill
G1 X128.656 Y103.609 E0.11180 ; infill
G1 X128.656 Y104.203 E0.02072 ; infill
G1 X125.797 Y101.344 E0.14111 ; infill
G1 X125.203 Y101.344 E0.02072 ; infill
G1 X128.656 Y104.797 E0.17042 ; infill
G1 X128.656 Y105.390 E0.02072 ; infill
G1 X124.610 Y101.344 E0.19973 ; infill
G1 X124.016 Y101.344 E0.02072 ; infill
G1 X128.656 Y105.984 E0.22904 ; infill
G1 X128.656 Y106.578 E0.02072 ; infill
G1 X123.422 Y101.344 E0.25834 ; infill
G1 X122.829 Y101.344 E0.02072 ; infill
G1 X128.656 Y107.171 E0.28765 ; infill
G1 X128.656 Y107.765 E0.02072 ; infill
G1 X122.235 Y101.344 E0.31696 ; infill
G1 X121.641 Y101.344 E0.02072 ; infill
G1 X128.656 Y108.359 E0.34627 ; infill
G4 S0
M73 P50 R0
G1 X128.656 Y108.656 E0.01036 ; infill
G1 X128.359 Y108.656 E0.01036 ; infill
G1 X121.344 Y101.641 E0.34627 ; infill
G1 X121.344 Y102.235 E0.02072 ; infill
G1 X127.765 Y108.656 E0.31696 ; infill
G1 X127.171 Y108.656 E0.02072 ; infill
G1 X121.344 Y102.829 E0.28765 ; infill
G1 X121.344 Y103.422 E0.02072 ; infill
G1 X126.578 Y108.656 E0.25834 ; infill
G1 X125.984 Y108.656 E0.02072 ; infill
G1 X121.344 Y104.016 E0.22903 ; infill
G1 X121.344 Y104.610 E0.02072 ; infill
G1 X125.390 Y108.656 E0.19972 ; infill
G1 X124.796 Y108.656 E0.02072 ; infill
G1 X121.344 Y105.204 E0.17041 ; infill
G1 X121.344 Y105.797 E0.02072 ; infill
G1 X124.203 Y108.656 E0.14110 ; infill
G1 X123.609 Y108.656 E0.02072 ; infill
G1 X121.344 Y106.391 E0.11179 ; infill
G1 X121.344 Y106.985 E0.02072 ; infill
G1 X123.015 Y108.656 E0.08248 ; infill
G1 X122.422 Y108.656 E0.02072 ; infill
G1 X121.344 Y107.578 E0.05317 ; infill
G1 X121.344 Y108.172 E0.02072 ; infill
G1 X122.011 Y108.839 E0.03291 ; infill
M204 S1250 ; adjust acceleration
G1 E-0.40000 F4800.00000 ; retract
G1 Z0.500 F12000.000 ; lift Z
G4 S0
M73 P75 R0
G4 ; wait
G92 E0 ; prepare to retract
G1 E-1 F2100; retract to avoid ooze
G4 S0
M73 P100 R0
M221 S100 ; reset extruder factor to 100%
M900 K0 ; reset linear acceleration
M104 S0 ; turn off temperature
M140 S0 ; turn off heatbed
M107 ; turn off fan
M84 ; disable motors
