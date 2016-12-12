# molT-master
Master C++ program that controls several electronic instruments for data acquisition in electromigration experiments. 

Instrumentation is Agilent Switchbox 33120A, Keithley Source-Measuring Unit K2400, and HP Semiconductor Parameter Analyzer 4146B. Communications protocol is IEEE-488 (GPIB). Project is developed in VisualStudio Express, output is a 32-bit console program.

## Usage
`main()` prints all the command line options for executing different instrumentation and data acquisition algorithms. The user inputs a single integer which corresponds to the desired algorithm in a switch case structure, the algorithm executes, then the menu is displayed again and waits for the next input. Relevant parameters for certain algorithms can be edited in text files and then read into the program.

## Experimental System
The experimental system we are dealing with is a packaged chip inserted into a socket on the end of a cyrogenic probe. Devices are defined in metal on the chip and run out to large pads on the chip perimeter. Wire bonds connect the chip pads to pads on the package, and the package pads are short with macroscopic metal pins on the bottom of the package. The package socket on the probe serves to connect these pins to long leads running up the probe, which terminate at several feed-throughs outside the cryostat. BNC cable bundles are plugged into these feed-throughs and then connected to the 36 output ports of an Agilent Switchbox. The input ports of the switchbox are connected to the relevant instruments so that at any time a single device can be addressed by any of the instruments. 

## Program Overview
Every time the program runs it insists on receiving an input file which contains the port-specifications of all the devices on the chip - this is a list of which switchbox output ports are being used for each device (source, drain, and two pads for the gate, if applicable). It looks like this, where "00,00" indicates no gate electrode):
08,10,00,00
09,10,00,00
11,10,00,00

The switchbox `getPorts` method takes this port spec file and creates a data structure attribute which contains "pad strings" for connecting each device's output ports to the input ports for specific instruments. The data structure also holds the information about which devices share a "common drain" pad.

The display method of the menu receives an integer input from the user specifying which algorithm to run. Each algorithm has a corresponding wrapper function defined in the menu object because algorithms typically involve multiple instruments. The building blocks of the different algorithms are defined as methods in the corresponding instrument classes, and each block is typically used in multiple algorithms. 


## Overview of Basic EM Algorithm
execute until R reaches target R:
- source V and measure I
- compute R=V/I, dR = delta(R) from previous point, and dVdI = delta(V)/delta(I) from previous point
- if this is the first few points of a ramp cycle, calculate R_benchmark as the average present R
- update counters for certain events in the metrics, like dVdI < 0
- if counters exceed critical values, ramp back to some fixed fraction of the present V
- increment V

The crux of the algorithm is what "events" we are counting that will trigger a ramp back. Event counters include:
- The number of consecutive negative dVdI events, typically with a critical value of 2 to 6 to trigger ramp back. 
- The number of consecutive occurrences of R > R_benchmark*(1+tol), where tol is some float input by the user. Typically this has a critical value of 3 to 5 to trigger ramp back.
- The number of consecutive occurrences of (R_jct + R_series) > (R_jct_bench + R_series)*(1+tol), where R_jct is calculated from the present resistance minus the initial resistance of the device, R_series is some average series resistance that devices of this type generally present input by the user, and tol is some float input by the user.  
- The number of consecutive dR > dR_user where dR_user is some user input value.

The reason we look for a critical number of consecutive events, rather than just triggering on a single event, is that noise in the device structure and in the measurement system can give faulty counts. As EM proceeds, the algorithm allows the critical count values to change so that e.g. we look for 3 consecutive negative dVdI events early on in the EM, but 5 later on. This is because the junction's tendency for catastrophic thermal run-away, as well as its tendency for structural rearrangement, change as EM proceeds so the optimal parameters for producing controlled EM also change somewhat.

## Implementation of Basic EM Algorithm
- **Start in the 10 mA sensing range** (means you need to choose a good starting V relative to initial device R) and 2 V voltage sourcing range.
- Dwell at `volt_start` for 1 second then ramp up. Throw away first 5 data points and use the average of the next 5 to compute `initial_res` for the device. 
- Set step counter to 0.
- Execute until F12 interrupt OR `resistance_timely >= initial_res + target_resistance` OR max voltage reached:
	- Step the voltage and get a new reading
	- Call `CalcMetrics` to get new `resistance_timely` and other relevant metrics
	- Call `CalcBenchmark` to compute a running average to get a benchmark resistance when we are at the beginning of a ramp cycle (step counter <= 8)
		- If step counter is between 0 and 5 (very beginning of ramp cycle) then set the `benchmark_resistance = resistance_timely`.
		- If step counter is between 5 and 10 then update `benchmark_resistance` as a running average of `resistance_timely`.
		- If step counter > 10 then do nothing 
	- Call `UpdateTolerances` to check what junction resistance regime we are in so we can use the correct critical count values and tolerances for updating our counters
	- Call `UpdateCounters` to update counters for various events of interest
	- Call `CheckRampBack` to decide whether we need a ramp back (the counters that we use for this decision depend on what the user chose as the "type" Of EM during initialization)
		- If we need to ramp back, reset all counters and set voltage back by 30%
		- If we ramped back and the current at the top of the last cycle was < 1 mA then **lower the current sensing range (this should in theory only occur once per device)**
	- Increment step counter.

## Constant-Voltage EM Algorithm
This is a variant of controlled EM where we dwell at a fixed voltage and rapidly sample the resistance of the device in order to observe it's breaking rate as a function of power. We begin with normal EM to break the junction to about +15 ohms, and we use this part of the trace to fit initial series resistance and initial junction resistance. Once Rjct > +15 ohms we wait for the next ramp back event and instead of resetting the voltage to 70% of its value we reset it to 90% of its value and just dwell at that fixed voltage. EM will typically proceed very very slowly at first, but at an exponentially increasing rate, and we observe this until the EM has broken the junction to ~ 200 ohms. 

