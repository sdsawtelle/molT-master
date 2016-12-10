# molT-master
Master C++ program that controls several electronic instruments for data acquisition in electromigration experiments. 

Instrumentation is Agilent switchbox, keithley SMU and HP SPA. Project is developed in VisualStudio Express, output is a 32-bit console program.

## Usage
`main()` creates instances of keithley, spa and switchbox classes and finally an instance of menu class. Then it calls the display method of the menu which prints to screen all the command line options for executing different instrumentation and data acquisition algorithms. The user inputs a single integer which corresponds to the desired algorithm in a switch case structure, the algorithm executes, then the menu is displayed again and waits for the next input. 

## Experimental System
The experimental system we are dealing with is a packaged chip inserted into a socket on the end of a cyrogenic probe. Devices are defined in metal on the chip and run out to large pads on the chip perimeter. Wire bonds connect the chip pads to pads on the package, and the package pads are short with macroscopic metal pins on the bottom of the package. The package socket on the probe serves to connect these pins to long leads running up the probe, which terminate at several feed-throughs outside the cryostat. BNC cable bundles are plugged into these feed-throughs and then connected to the 36 output ports of an Agilent Switchbox. The input ports of the switchbox are connected to the relevant instruments so that at any time a single device can be addressed by any of the instruments. 


## Program Overview

Every time the program runs it insists on receiving an input file which contains the port-specifications of all the devices on the chip - this is a list of which switchbox output ports are being used for each device (source, drain, and two pads for the gate, if applicable). It looks like this, where "00,00" indicates no gate electrode):
08,10,00,00
09,10,00,00
11,10,00,00

The switchbox `getPorts` method takes this port spec file and creates a data structure attribute which contains "pad strings" for connecting each device's output ports to the input ports for specific instruments. The data structure also holds the information about which devices share a "common drain" pad.

The display method of the menu receives an integer input from the user specifying which algorithm to run. Each algorithm has a corresponding wrapper function defined in the menu object because algorithms typically involve multiple instruments. The building blocks of the different algorithms are defined as methods in the corresponding instrument classes, and each block is typically used in multiple algorithms. 



## Electromigration Algorithms
Over the course of my thesis I have used a very wide variety of electromigration algorithms, each one usually implemented as its own function because they differ so much. I haven't done a good job handling all these variants, the EM functionality really should be more modular. At any rate, the most basic EM algorithm goes likes this:

### Basic EM Algorithm
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
- The number of consecutive occurrences of R > (R_benchmark + delta), where delta is some small resistance change input by the user. 
- The number of consecutive occurrences of R > (R_jct + R_series)*(1+tol), where R_jct is calculated from the present resistance minus the initial resistance of the device, R_series is some average series resistance that devices of this type generally present input by the user, and tol is some float input by the user.  
- The number of consecutive dR > dR_user where dR_user is some user input value.

The reason we look for a critical number of consecutive events, rather than just triggering on a single event, is that noise in the device structure and in the measurement system can give faulty counts. As EM proceeds, the algorithm allows the critical count values to change so that e.g. we look for 3 consecutive negative dVdI events early on in the EM, but 5 later on. This is because the junction's tendency for catastrophic thermal run-away, as well as its tendency for structural rearrangement, change as EM proceeds so the optimal parameters for producing controlled EM also change somewhat.

### EM algorithm Variants
In this project I have some variants on the basic algorithm above:
- NORMAL EM is the original basic algorithm
- STABILIZATION EM just steps in small (or zero) voltage steps and measures current until seeing R = 15 kohms.
- CURVATURE EM is a complicated algorithm that steps in very small voltage stepsand dwells for a while at each voltage to see if EM has been triggered.If so, then we just sit at that voltage and watch it proceed.
- ALTERNATING POLARITY EM reverses the polarity of the voltage between source and drain on each ramp back
- PULSED EM pulses increasingly large voltages, going back to zero in between each one.

## Other Functionality
- Dwelling: sweeps the voltage up to some fixed voltage and then dwells there until target resistance is reached - very similar to STABILIZATION EM. Don't really use this ever anymore.
- Healing Gaps: applies increasingly large voltages to wire gaps to see if we can reverse the mass flow and close the gap. Never got this to work.



