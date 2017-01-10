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
- compute metrics like R=V/I and dVdI = delta(V)/delta(I)
- if this is the first few points of a ramp-up, calculate a benchmark resistance
- update counters for certain events in the metrics, like dVdI < 0
- if counters exceed critical values, ramp back to some fixed fraction of the present V
- increment V

The crux of the algorithm is what "events" we are counting that will trigger a ramp back. Event counters include:
- The number of consecutive negative dVdI events, typically with a critical value of 2 to 6 to trigger ramp back. 
- The number of consecutive occurrences of R > R_benchmark*(1+tol), where tol is some float input by the user and R_benchmark is the benchmark resistance for the ramp cycle. Typically this has a critical value of 3 to 5 to trigger ramp back.
- The number of consecutive occurrences of (R_jct + R_series) > (R_jct_bench + R_series)*(1+tol), where R_jct is calculated from the present resistance minus the initial resistance of the device, R_series is some average series resistance that devices of this type generally present input by the user, and tol is some float input by the user.  
- The number of consecutive dR > dR_user where dR_user is some user input value.

The reason we look for a critical number of consecutive events, rather than just triggering on a single event, is that noise in the device structure and in the measurement system can give faulty counts. As EM proceeds, the algorithm allows the critical count values to change so that e.g. we look for 3 consecutive negative dVdI events early on in the EM, but 5 later on. This is because the junction's tendency for catastrophic thermal run-away, as well as its tendency for structural rearrangement, change as EM proceeds so the optimal parameters for producing controlled EM also change somewhat.

## Implementation of Basic EM Algorithm
Parameters of the algorithm are set from a text file. A menu lets the user choose which events to monitor for ramp back decision. The core of the algorithm is a the voltage stepping loop, it has one normal exit condition - resistance increasing past the target, and several error exit conditions - a keyboard interrupt, reaching the max voltage, or current range issues.

- **Start in the 10 mA sensing range** (means you need to choose a good starting V relative to initial device R) and 2 V voltage sourcing range.
- Dwell at `volt_start` for 1 second then ramp up. Throw away first 5 data points and use the average of the next 5 to compute `initial_res` for the device. 
- Set step counter to 0.
- Execute until `resistance_timely >= initial_res + target_resistance` OR F12 interrupt OR max voltage reached OR `curr_range_Flag` raised:
	- Step the voltage and get a new reading
	- Call `CalcMetrics` to get new `resistance_timely`, differential resistance and change in resistance (`delta_res`)
	- Call `CalcBenchmark` to compute a running average to get a benchmark resistance when we are at the beginning of a ramp cycle (step counter <= 8)
		- If step counter is between 0 and 5 (very beginning of ramp cycle) then set the `benchmark_resistance = resistance_timely`.
		- If step counter is between 5 and 10 then update `benchmark_resistance` as a running average of `resistance_timely`.
		- If step counter > 10 then do nothing 
	- Call `UpdateTolerances` to check what junction resistance regime we are in so we can use the correct critical count values and tolerances for updating our counters
	- Call `UpdateCounters` to update counters for various events of interest
		- If step counter is >= 10 then update the counters, otherwise don't.
	- Call `CheckRampBack` to decide whether we need a ramp back (the counters that we use for this decision depend on what the user chose as the "type" Of EM during initialization)
		- If we need to ramp back, reset all counters and set voltage back by 30%
			- If we ramped back and the previous current i.e. current at the top of the last ramp was < 1 mA then **lower the current sensing range (this should in theory only occur once per device)**
	- If the current range has been decreased to 1 mA but now the current value is higher, then raise `curr_range_Flag`. This happens for devices who don't conform to the critical power behavior and can have increasing critical current with junction resistance. 
	- If junction resistance is at least 60 ohms for the first time, use gnuplot to make a snapshot of the EM trace.
	- Increment step counter.

## Constant-Voltage EM Algorithm
This is a variant of controlled EM where we dwell at a fixed voltage and rapidly sample the resistance of the device in order to observe it's breaking rate as a function of power. We begin with normal EM to break the junction to about +20 ohms: this uses the same core voltage stepping loop as above, but the exit condition is modified so that it only exits when Rjct >= 20 ohms AND we have just executed a ramp back. The voltage is then set to 90% of the value at the top of the ramp, rather than 70% as in normal EM, and held constant while the current is sampled. At this constant higher voltage EM will typically proceed very very slowly at first, but at an exponentially increasing rate, and we observe this until the EM has broken the junction to ~ 200 ohms. 

The full implementation is as follows:

- **Start in the 10 mA sensing range** (means you need to choose a good starting V relative to initial device R) and 2 V voltage sourcing range.
- Dwell at `volt_start` for 1 second then ramp up. Throw away first 5 data points and use the average of the next 5 to compute `initial_res` for the device. 
- Set step counter to 0.
- Execute until (`resistance_timely >= initial_res + target_resistance` AND counter is 1) OR until F12 interrupt OR max voltage reached OR `curr_range_Flag` raised:
	- Step the voltage and get a new reading
	- Call `CalcMetrics` to get new `resistance_timely`, differential resistance and change in resistance (`delta_res`)
	- Call `CalcBenchmark` to compute a running average to get a benchmark resistance when we are at the beginning of a ramp cycle (step counter <= 8)
		- If step counter is between 0 and 5 (very beginning of ramp cycle) then set the `benchmark_resistance = resistance_timely`.
		- If step counter is between 5 and 10 then update `benchmark_resistance` as a running average of `resistance_timely`.
		- If step counter > 10 then do nothing 
	- Call `UpdateTolerances` to check what junction resistance regime we are in so we can use the correct critical count values and tolerances for updating our counters
	- Call `UpdateCounters` to update counters for various events of interest
		- If step counter is >= 10 then update the counters, otherwise don't.
	- Call `CheckRampBack` to decide whether we need a ramp back (the counters that we use for this decision depend on what the user chose as the "type" Of EM during initialization)
		- If we need to ramp back, reset all counters and set voltage back by 30%
			- If we ramped back and the previous current i.e. current at the top of the last ramp was < 1 mA then **lower the current sensing range (this should in theory only occur once per device)**
	- If the current range has been decreased to 1 mA but now the current value is higher, then raise `curr_range_Flag`. This happens for devices who don't conform to the critical power behavior and can have increasing critical current with junction resistance. 
	- If junction resistance is at least 60 ohms for the first time, use gnuplot to make a snapshot of the EM trace.
	- Increment step counter.
- If the prevous loop exited on the junction resistance condition rather than error condition then update the voltage to be 90% of the value at the top of the ramp cycle on which the loop exited. 
- Execute until `resistance_timely >= initial_res + target_resistance` OR F12 interrupt:
	- Measure time elapsed since loop began and write to file
	- Measure current and call `CalcResTimely` to calculate resistance and write to file


# Approach to Voltage and Current Range Issues
The devices we are working with are extremely sensitive to current spikes and a voltage or current range adjustment by the Keithley produces such a spike. Unfortunately the EM algorithm tends to sweep out a broad range of voltages and currents over the course of breaking a junction. 

The relevant ranges for our devices are:
- Voltage (source): max 21 V, max 2 V and max 0.2 V
- Current (sense): max 10 mA, max 1 mA and max 0.1 mA

For a given voltage range, it is possible to source into the next lowest range, but the error increases. For instance, in the 2 V range you *can* source 0.1 V, but it will have a larger error than if you used the 0.2 V range. It is NOT possible to source above the maximum voltage for a range (throws an error).

For a given current range, it is possible to sense into the next highest range, but the error increases. For instance, in the 0.1 mA range you *can* sense 0.5 mA, but it will have a larger error than if you used the 1 mA range. It is NOT possible to sense below the minimum current for a range i.e. the maximum current of the range below it (no error, but will simply continue outputting the minimum value).

As a specific example, consider sourcing various voltages across a 350 ohm nominal resistor and measuring the current using the 1 mA current range. If the measured current is 0.6 mA then the calculated resistance (R = V/I) will be around 349.5 whereas if the measured current is 3 mA then R will be almost exactly 350 ohms. This seems like a small deviation, but note that **the error increases somewhat exponentially as you drop below the minimum current**

For intact devices in the 30x10 and 50x400 geometries at 95 K the initial resistances are in the range 350 to 650 Ohms. When breaking to R_jct ~ 150 Ohms, the following holds:
- Voltages < 0.2 V are never accessed
- Voltages > 2 V are sometimes necessary
- Currents < 1 mA are sometimes necessary
- The critical current for the first EM onset is always > 1 mA
- The shape of the EM curve is such that the top of a ramp cycle is the maximum current drawn and voltage sourced for the subsequent breaking (assuming the device "behaves" and doesn't rearrange).

Considering the above points, my **solution to the Current Range** is as follows:
- Hardcode the initial current range as 1 mA
- Set the starting voltage at 0.5 V - this is high enough that we draw at least 0.8 mA to minimize error in the current measurement (even for 650 ohm devices), but it is still low enough to be below the first EM onset.
- Within the algorithm, at the beginning of each ramp cycle check whether the current has dropped below 0.9 mA. If it has, then slowly ramp the voltage to zero, disconnect the device, execute a range change, reconnect the device, slowly ramp back up, and proceed from where we left off (note, it does not record readings in the log during this process).

For devices that require > 2 V for initial breaking, my **solution to the Voltage Range** is as follows:
- In the wrapper function for EM first attempt EM in the 2 V range. 
- If the device exits this attempt at 2 V i.e. hits max voltage, then execute the High-Voltage EM algorithm.
	- change the range to 21 V (with device disconnected)
	- start the voltage ramp at 1.8 V
	- Loop is identical to standard EM with the following changes:
		- There is no max-voltage exit condition
		- At the top of each ramp cycle, if the voltage is < 1.95 V then safely execute a range change down to 2 V.
		- Add a check in every voltage step to make sure we don't have voltage > 1.99 V when voltage range = 2 V, if so then exit the EM.
