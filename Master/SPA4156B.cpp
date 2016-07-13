#include "SPA4156B.h"
#include <iostream>
#include <windows.h>
#include "ni488.h"
#include <cstdlib>

// constructor, every instance of SPA4156B that is created will automatically run configure.
SPA4156B::SPA4156B(){
	configure();
}

// this is the destructor method, see comments in SPA4156B.h
SPA4156B::~SPA4156B(){
	//dtor
}


// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- SETUP AND CONFIGURING FUNCTIONS -----------------------------------
// ---------------------------------------------------------------------------------------------------------


int SPA4156B::configure(){

	int ERROR1 = 0; // error information. if any happens, this variable will go to 1;


	//Now we will initialize the sourcemeters
	printf("Initializing SPA4156...\n");

	this->pna = ibdev(0, PRIMARY_ADDR_OF_PNA_SPA, NO_SECONDARY_ADDR, TIMEOUT, EOTMODE, EOSMODE); //ibdev gives back ud! Unique iDentifier based on info given in

	//clear GPIB interface
	ibclr(pna);
	//if there is an error, display error message and get the error flag up! We will catch it later, at the very end.

	if (ibsta & ERR){
		printf("Could not send IFC\n");
		ERROR1 = 1;
	}

	// set the GPIB board as controller in charge. Set or clear the Remote Enable line.
	ibsre(0, 1);

	if (ibsta & ERR){    //catching error
		printf("Unable to open handle to PNA /n ibsta = 0x%x iberr = %d/n", ibsta, iberr);     // if we get it, print a message about it. Debug purposes
		std::cout << iberr;
		ERROR1 = 1;
	}

	// reset machine to defaults
	GPIBWrite(pna, "*RST");

	if (ibsta & ERR){
		printf("GPIB indicates an error, stopping program\n");
		std::cout << iberr;
		ERROR1 = 1;
	}

	// we are catching the error if it happend earlier
	if (ERROR1 == 1){
		std::cout << "I am pausing the program! There is an error somewhere! Quit the program and check it! Do not press any other key, otherwise you will just unpause. BUM!\n";
		system("Pause"); //pausing execution if I have an error!
	}
	else printf("Initialization of SPA4156 was successful.\n");
	return 0;

}

void SPA4156B::setParams(int mode){
	
	switch (mode){
		 // mode = 1 for sampling (uses SMU1 and SMU2)
		case 1:{
			float voltage;
			std::cout << "What is the wait time between sampling? (in seconds) \n";
			std::cin >> wait_time;
			std::cout << "What is the voltage to sample at (in V)? \n";
			std::cin >> voltage;
			configSample(voltage);
			break;}

		// mode 2 = sweeping IVs (uses SMU1 and SMU2) 
		case 2:{
			float Vstart;
			std::cout << "What is the starting sweep value? (in V) \n";
			std::cin >> Vstart;
			float Vstop;
			std::cout << "What is the stopping sweep value? (in V) \n";
			std::cin >> Vstop;
			float Vstep;
			std::cout << "What is the voltage step size? (in V) \n";
			std::cin >> Vstep;
			configSweep(Vstart, Vstop, Vstep);
			break;}

		// mode 3 = nanowire and gateline yield monitoring (uses SMU1 and SMU2)
		case 3:{
			std::cout << "We will use a voltage value of 50 mV for sampling yield. \n";
			configSample(0.05);
			break; }


		// mode 4 = gated IV's i.e. 3-terminal sweeps with leakage current recorded (uses all SMU1,2 and 3)
		case 4:{
				// For subordinate sweep measurement, you set up a secondary sweep source(VAR2)
				// in addition to a primary sweep source(VAR1).After primary sweep is completed,
				// the output of secondary sweep source is incremented or decremented by the
				// specified step value, then the primary sweep source is swept again
				float Vstart;
				std::cout << "What is the starting sweep value for Ids measurement? (in V) \n";
				std::cin >> Vstart;
				float Vstop;
				std::cout << "What is the stopping sweep value for Ids measurement? (in V) \n";
				std::cin >> Vstop;
				float Vstep;
				std::cout << "What is the voltage step size for Ids measurement? (in V) \n";
				std::cin >> Vstep;

				float Vstart_gate;
				std::cout << "What is the gate voltage to begin biasing the gate at? (in V) \n";				std::cin >> Vstart_gate;
				float Vstep_gate;
				std::cout << "What is the voltage step size for incrementing the gate voltage? (in V) \n";
				std::cin >> Vstep_gate;
				float Vstop_gate;
				std::cout << "What is the final stopping voltage for biasing the gate? (in V) \n";
				std::cin >> Vstop_gate;

				configGatedSweeps(Vstart, Vstop, Vstep, Vstart_gate, Vstep_gate, Vstop_gate);
				break; }
		// mode 5 = gated IV's i.e. 3-terminal sweeps but don't record leakage current (uses all SMU1,2 and 3)
		case 5:{
				// For subordinate sweep measurement, you set up a secondary sweep source(VAR2)
				// in addition to a primary sweep source(VAR1).After primary sweep is completed,
				// the output of secondary sweep source is incremented or decremented by the
				// specified step value, then the primary sweep source is swept again
				float Vstart;
				std::cout << "What is the starting sweep value for Ids measurement? (in V) \n";
				std::cin >> Vstart;
				float Vstop;
				std::cout << "What is the stopping sweep value for Ids measurement? (in V) \n";
				std::cin >> Vstop;
				float Vstep;
				std::cout << "What is the voltage step size for Ids measurement? (in V) \n";
				std::cin >> Vstep;

				float Vstart_gate;
				std::cout << "What is the gate voltage to begin biasing the gate at? (in V) \n";				
				std::cin >> Vstart_gate;
				float Vstep_gate;
				std::cout << "What is the voltage step size for incrementing the gate voltage? (in V) \n";
				std::cin >> Vstep_gate;
				float Vstop_gate;
				std::cout << "What is the final stopping voltage for biasing the gate? (in V) \n";
				std::cin >> Vstop_gate;

				configGatedSweepsNoIG(Vstart, Vstop, Vstep, Vstart_gate, Vstep_gate, Vstop_gate);
				break; }

		default:
			std::cout << "Error in setting SPA parameters \n";
			break;
	}

}

void SPA4156B::setGatedIVParamsLater(float* Vstart, float* Vstop, float* Vstep, float* Vstart_gate, float* Vstep_gate, float* Vstop_gate){
			   
	int choice;
	std::cout << ">>>>>>>>>>> Use Default Parameters for Gated IV? ('0' for No, '1' for Yes)\n";
	std::cin >> choice;

	if (choice){
		*Vstart = -0.24;
		*Vstop = 0.24;
		*Vstep = 0.01;
		*Vstart_gate = -4.0;
		*Vstep_gate = 4.0;
		*Vstop_gate = 2;
	}
	else{
		std::cout << "What is the starting Vds sweep value for Ids measurement? (in V) \n";
		std::cin >> *Vstart;
		std::cout << "What is the stopping Vds sweep value for Ids measurement? (in V) \n";
		std::cin >> *Vstop;
		std::cout << "What is the Vds step size for Ids measurement? (in V) \n";
		std::cin >> *Vstep;

		std::cout << "What is the gate voltage to begin biasing the gate at? (in V) \n";
		std::cin >> *Vstart_gate;
		std::cout << "What is the voltage step size for incrementing the gate voltage? (in V) \n";
		std::cin >> *Vstep_gate;
		std::cout << "What is the final stopping voltage for biasing the gate? (in V) \n";
		std::cin >> *Vstop_gate;
	}

}

void SPA4156B::setIVParamsLater(float* Vstart, float* Vstop, float* Vstep){

	int choice;
	std::cout << ">>>>>>>>>>> Use Default Parameters for IV? ('0' for No, '1' for Yes)\n";
	std::cin >> choice;

	if (choice){
		*Vstart = -0.24;
		*Vstop = 0.24;
		*Vstep = 0.002;
	}
	else{
		std::cout << "What is the starting sweep value for Ids measurement? (in V) \n";
		std::cin >> *Vstart;
		std::cout << "What is the stopping sweep value for Ids measurement? (in V) \n";
		std::cin >> *Vstop;
		std::cout << "What is the voltage step size for Ids measurement? (in V) \n";
		std::cin >> *Vstep;
	}

}

void SPA4156B::setSwBrParamsLater(int* useEMVFlag, float* minStart, float* maxStart, float* targetR, float* stepBias, float* stepV){

	int choice;
	std::cout << ">>>>>>>>>>> Use Default Parameters for Sweep Breaking? ('0' for No, '1' for Yes)\n";
	std::cin >> choice;

	if (choice){
		*useEMVFlag = 1; //Use exit voltages from prior EM runs to set Sweep breaking minstartV and maxstartV parameters
		*targetR = 600000.0;
		*stepBias = 0.005;
		*stepV = 0.001;
	}
	else{
		*useEMVFlag = 0;
		std::cout << "What is the start voltage for all sweeps i.e. min bias (in Volts)? \n";
		std::cin >> *minStart;
		std::cout << "What is the starting max bias for sweeps (in Volts)? \n";
		std::cin >> *maxStart;
		std::cout << "What is the target resistance (in Ohms)? \n";
		std::cin >> *targetR;
		std::cout << "What is the step size for max bias on successive sweeps (in Volts)? \n";
		std::cin >> *stepBias;
		std::cout << "What is the step size for voltage during sweeps (in Volts)? \n";
		std::cin >> *stepV;
	}

}

void SPA4156B::configSample(float voltage){

	// initialize portVoltages for the monitoring functions
	for (int i = 0; i < 36; i++){
		portVoltages[i] = 1;
	}

	// Most of the commands that control and set the 4155B / 56B will also update the instrument screen,but it takes time.You can enable or disable this time consuming instrument screen update as follows :
	// GPIBWrite(pna, ":DISP OFF");

	//This command deletes the settings of all units (SMU,VSU,VMU,PGU,GNDU).
	GPIBWrite(pna, ":PAGE:CHAN:ALL:DIS");
	Sleep(200);
	//These commands set the INAME (current) and VNAME (voltage) of SMU<n>
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:VNAME 'V1'");
	Sleep(200);
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:INAME 'I1'");
	Sleep(200);
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:VNAME 'V2'");
	Sleep(200);
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:INAME 'I2'");
	Sleep(200);
	//This command sets the sb_output MODE of SMU<n>. (V  Voltage sb_output mode)(COMMon  Common)
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:MODE V");
	Sleep(200);
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:MODE COMM");
	Sleep(200);
	//This command sets the function(FCTN) of SMU<n>. (CONStant  Constant) (VAR1  VAR1 function(available for sweep and QSCV))
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:FUNC CONS");
	Sleep(200);
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:FUNC CONS");
	Sleep(200);
	// This command sets the MEASUREMENT MODE
	GPIBWrite(pna, ":PAGE:CHAN:MODE SAMP");
	Sleep(200);
	Sleep(100);

	//This command sets the value of 'LONG' integration, then selects LONG as the the INTEGRATION TIME mode.
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM:LONG 6");
	Sleep(200);
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM LONG");
	Sleep(200);
	//This command selects the ranging MODE of SMU<n>
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU1:RANG:MODE AUTO");
	Sleep(200);
	Sleep(200);
	
	// This command selects the sb_output sequence mode for sampling measurement. You use this command only if the measurement mode is sampling. "SIM" means all source unit starts sb_output at same timing. 
	GPIBWrite(pna, ":PAGE:MEAS:OSEQ:MODE SIM");
	Sleep(200);
	//This command changes the present display page to MEASURE : SAMPLING SETUP page
	GPIBWrite(pna, ":PAGE:MEAS:SAMP");
	Sleep(200);
	//This command sets the MODE for sampling measurement. The sampling mode determines the sampling interval.
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:MODE LIN");
	Sleep(200);
	// This command sets the constant SOURCE value of SMU<n> for the sampling measurement. The mode of the specified SMU must be V or I.
	char samplevoltage[60];
	sprintf(samplevoltage, ":PAGE:MEAS:SAMP:CONS:SMU1 %f", voltage);
	GPIBWrite(pna, samplevoltage);
	voltageFlag = 1;
	Sleep(200);

	// This command sets the INITIAL INTERVAL for sampling measurement(in seconds)
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:IINT 0.002");
	Sleep(200);
	// This command sets the HOLD TIME of sampling measurement.This is the wait time between turning on the 'sb_outputs' and taking the first sampling point measurement. Min is 30ms
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:HTIM 0.005");
	Sleep(200);
	// This command sets the TOTAL SAMPLING TIME for sampling measurement (in seconds). Auto (disables total sampling time stop event, and enables the number of samples stop event.)
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:PER:AUTO ON");
	Sleep(200);
	// This command sets the NUMBER OF SAMPLES for sampling measurement.
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:POIN 1");
	Sleep(200);
	//GPIBWrite(pna, "*WAI");
	Sleep(200);
	
}

void SPA4156B::constconfigSample(){
	//This command sets the value of 'LONG' integration, then selects LONG as the the INTEGRATION TIME mode.
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM:LONG 6");
	Sleep(200);
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM LONG");
	Sleep(200);
	// This command sets the INITIAL INTERVAL for sampling measurement(in seconds)
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:IINT 0.002");
	Sleep(200);
	// This command sets the NUMBER OF SAMPLES for sampling measurement.
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:POIN 500");
	Sleep(200);
}

void SPA4156B::configSweep(float Vstart, float Vstop, float Vstep){
	// Most of the commands that control and set the 4155B / 56B will also update the instrument screen,but it takes time.You can enable or disable this time consuming instrument screen update as follows :
	GPIBWrite(pna, ":DISP OFF");

	//This command deletes the settings of all units (SMU,VSU,VMU,PGU,GNDU).
	GPIBWrite(pna, ":PAGE:CHAN:ALL:DIS");

	//These commands set the INAME (current) and VNAME (voltage) of SMU<n>
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:VNAME 'V1'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:INAME 'I1'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:VNAME 'V2'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:INAME 'I2'");

	//This command sets the sb_output MODE of SMU<n>. (V  Voltage sb_output mode)(COMMon  Common)
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:MODE V");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:MODE COMM");

	//This command sets the function(FCTN) of SMU<n>. (CONStant  Constant) (VAR1  VAR1 function(available for sweep and QSCV))
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:FUNC VAR1");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:FUNC CONS");

	// This command sets the MEASUREMENT MODE
	GPIBWrite(pna, ":PAGE:CHAN:MODE SWE");

	//This command sets the value of 'LONG' integration, then selects LONG as the the INTEGRATION TIME mode.
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM:LONG 4");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM LONG");

	//This command selects the ranging MODE of SMU<n>
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU1:RANG:MODE AUTO");

	//This command sets to a single sweep with linear steps
	GPIBWrite(pna, ":PAGE:MEAS:VAR1:MODE SING");
	GPIBWrite(pna, ":PAGE:MEAS:VAR1:SPAC LIN");

	// set the sweep parameters
	char starting[60];
	sprintf(starting, ":PAGE:MEAS:VAR1:START %f", Vstart);
	GPIBWrite(pna, starting);

	char stopping[60];
	sprintf(stopping, ":PAGE:MEAS:VAR1:STOP %f", Vstop);
	GPIBWrite(pna, stopping);

	char stepping[60];
	sprintf(stepping, ":PAGE:MEAS:VAR1:STEP %f", Vstep);
	GPIBWrite(pna, stepping);

	GPIBWrite(pna, ":PAGE:DISP:SET:MODE LIST"); // set the display mode to list instead of graphics
	GPIBWrite(pna, ":PAGE:DISP:SET:LIST:DEL:ALL"); // delete all variable names from list
	GPIBWrite(pna, ":PAGE:DISP:SET:LIST:SEL 'V1','I1'"); // add specified variables to list


	}

void SPA4156B::configGatedSweeps(float Vstart, float Vstop, float Vstep, float Vstart_gate, float Vstep_gate, float Vstop_gate){
	
	// For subordinate sweep measurement, you set up a secondary sweep source(VAR2)
	// in addition to a primary sweep source(VAR1).After primary sweep is completed,
	// the output of secondary sweep source is incremented or decremented by the
	// specified step value, then the primary sweep source is swept again
	
	// Most of the commands that control and set the 4155B / 56B will also update the instrument screen,but it takes time.You can enable or disable this time consuming instrument screen update as follows :
	// GPIBWrite(pna, ":DISP OFF");

	GPIBWrite(pna, "*RST");

	//This command deletes the settings of all units (SMU,VSU,VMU,PGU,GNDU).
	GPIBWrite(pna, ":PAGE:CHAN:ALL:DIS");


	//These commands set the INAME (current) and VNAME (voltage) of SMU<n>
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:VNAME 'V1'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:INAME 'I1'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:VNAME 'V2'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:INAME 'I2'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU3:VNAME 'V3'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU3:INAME 'I3'");

	//This command sets the sb_output MODE of SMU<n>. (V  Voltage sb_output mode)(COMMon  Common)
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:MODE V");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:MODE COMM");
	GPIBWrite(pna, ":PAGE:CHAN:SMU3:MODE V");
	// SMU2 should always be the common drains, and those will be GND


	//This command sets the function(FCTN) of SMU<n>. (CONStant  Constant) (VAR1  VAR1 function(available for sweep and QSCV))
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:FUNC VAR1");
	GPIBWrite(pna, ":PAGE:CHAN:SMU3:FUNC VAR2");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:FUNC CONS");
	
	// VAR1 and VAR2 modes for subordinate sweep routine (sweeping var1 then incrementing var2 and sweeping it again)
	
	// This command sets the MEASUREMENT MODE
	GPIBWrite(pna, ":PAGE:CHAN:MODE SWE");

	//This command sets the value of 'LONG' integration, then selects LONG as the the INTEGRATION TIME mode.
	//GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM:LONG 4");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM MED");

	//This command selects the ranging MODE of SMU<n>
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU1:RANG:MODE AUTO");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU2:RANG:MODE AUTO");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU3:RANG:MODE AUTO");

	//This command sets the Ids sweeps to a single sweep with linear steps
	GPIBWrite(pna, ":PAGE:MEAS:VAR1:MODE SING");
	GPIBWrite(pna, ":PAGE:MEAS:VAR1:SPAC LIN");

	// set compliances - was getting error -118 that Var2 power output was too large for unit when i didn't actively set compliance (default was 2 A)
	GPIBWrite(pna, ":PAGE:MEAS:VAR1:COMP 0.1");
	GPIBWrite(pna, ":PAGE:MEAS:VAR2:COMP 0.01");

	// set the sweep parameters for the Ids sweeps
	char starting[60];
	sprintf(starting, ":PAGE:MEAS:VAR1:START %f", Vstart);
	GPIBWrite(pna, starting);

	char stopping[60];
	sprintf(stopping, ":PAGE:MEAS:VAR1:STOP %f", Vstop);
	GPIBWrite(pna, stopping);

	char stepping[60];
	sprintf(stepping, ":PAGE:MEAS:VAR1:STEP %f", Vstep);
	GPIBWrite(pna, stepping);

	//This command sets the Gate bias incrementing style
	GPIBWrite(pna, ":PAGE:MEAS:VAR2:MODE SING");
	GPIBWrite(pna, ":PAGE:MEAS:VAR2:SPAC LIN");

	// set the sweep parameters for the Gate bias incrementing
	char starting_gate[60];
	sprintf(starting_gate, ":PAGE:MEAS:VAR2:START %f", Vstart_gate);
	GPIBWrite(pna, starting_gate);

	char stepping_gate[60];
	sprintf(stepping_gate, ":PAGE:MEAS:VAR2:STEP %f", Vstep_gate);
	GPIBWrite(pna, stepping_gate);

	int Nsteps_gate = floor(abs((Vstop_gate - Vstart_gate) / Vstep_gate))+1;
	std::cout << "Number of steps for the gate voltage is " << Nsteps_gate << "\n";
	char steps[60];
	sprintf(steps, ":PAGE:MEAS:VAR2:POINTS %i", Nsteps_gate);
	GPIBWrite(pna, steps);

	GPIBWrite(pna, ":PAGE:DISP:SET:MODE LIST"); // set the display mode to list instead of graphics
	GPIBWrite(pna, ":PAGE:DISP:SET:LIST:DEL:ALL"); // delete all variable names from list
	GPIBWrite(pna, ":PAGE:DISP:LIST 'V1','I1','V3','I3'"); // add specified variables to list
}

void SPA4156B::configGatedSweepsNoIG(float Vstart, float Vstop, float Vstep, float Vstart_gate, float Vstep_gate, float Vstop_gate){

	// For subordinate sweep measurement, you set up a secondary sweep source(VAR2)
	// in addition to a primary sweep source(VAR1).After primary sweep is completed,
	// the output of secondary sweep source is incremented or decremented by the
	// specified step value, then the primary sweep source is swept again

	// Most of the commands that control and set the 4155B / 56B will also update the instrument screen,but it takes time.You can enable or disable this time consuming instrument screen update as follows :
	// GPIBWrite(pna, ":DISP OFF");

	GPIBWrite(pna, "*RST");

	//This command deletes the settings of all units (SMU,VSU,VMU,PGU,GNDU).
	GPIBWrite(pna, ":PAGE:CHAN:ALL:DIS");


	//These commands set the INAME (current) and VNAME (voltage) of SMU<n>
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:VNAME 'V1'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:INAME 'I1'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:VNAME 'V2'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:INAME 'I2'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU3:VNAME 'V3'");
	GPIBWrite(pna, ":PAGE:CHAN:SMU3:INAME 'I3'");

	//This command sets the sb_output MODE of SMU<n>. (V  Voltage sb_output mode)(COMMon  Common)
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:MODE V");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:MODE COMM");
	GPIBWrite(pna, ":PAGE:CHAN:SMU3:MODE V");
	// SMU2 should always be the common drains, and those will be GND


	//This command sets the function(FCTN) of SMU<n>. (CONStant  Constant) (VAR1  VAR1 function(available for sweep and QSCV))
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:FUNC VAR1");
	GPIBWrite(pna, ":PAGE:CHAN:SMU3:FUNC VAR2");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:FUNC CONS");

	// VAR1 and VAR2 modes for subordinate sweep routine (sweeping var1 then incrementing var2 and sweeping it again)

	// This command sets the MEASUREMENT MODE
	GPIBWrite(pna, ":PAGE:CHAN:MODE SWE");

	//This command sets the value of 'LONG' integration, then selects LONG as the the INTEGRATION TIME mode.
	//GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM:LONG 4");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM MED");

	//This command selects the ranging MODE of SMU<n>
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU1:RANG:MODE AUTO");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU2:RANG:MODE AUTO");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU3:RANG:MODE AUTO");

	//This command sets the Ids sweeps to a single sweep with linear steps
	GPIBWrite(pna, ":PAGE:MEAS:VAR1:MODE SING");
	GPIBWrite(pna, ":PAGE:MEAS:VAR1:SPAC LIN");

	// set compliances - was getting error -118 that Var2 power output was too large for unit when i didn't actively set compliance (default was 2 A)
	GPIBWrite(pna, ":PAGE:MEAS:VAR1:COMP 0.1");
	GPIBWrite(pna, ":PAGE:MEAS:VAR2:COMP 0.01");

	// set the sweep parameters for the Ids sweeps
	char starting[60];
	sprintf(starting, ":PAGE:MEAS:VAR1:START %f", Vstart);
	GPIBWrite(pna, starting);

	char stopping[60];
	sprintf(stopping, ":PAGE:MEAS:VAR1:STOP %f", Vstop);
	GPIBWrite(pna, stopping);

	char stepping[60];
	sprintf(stepping, ":PAGE:MEAS:VAR1:STEP %f", Vstep);
	GPIBWrite(pna, stepping);

	//This command sets the Gate bias incrementing style
	GPIBWrite(pna, ":PAGE:MEAS:VAR2:MODE SING");
	GPIBWrite(pna, ":PAGE:MEAS:VAR2:SPAC LIN");

	// set the sweep parameters for the Gate bias incrementing
	char starting_gate[60];
	sprintf(starting_gate, ":PAGE:MEAS:VAR2:START %f", Vstart_gate);
	GPIBWrite(pna, starting_gate);

	char stepping_gate[60];
	sprintf(stepping_gate, ":PAGE:MEAS:VAR2:STEP %f", Vstep_gate);
	GPIBWrite(pna, stepping_gate);

	int Nsteps_gate = floor(abs((Vstop_gate - Vstart_gate) / Vstep_gate)) + 1;
	std::cout << "Number of steps for the gate voltage is " << Nsteps_gate << "\n";
	char steps[60];
	sprintf(steps, ":PAGE:MEAS:VAR2:POINTS %i", Nsteps_gate);
	GPIBWrite(pna, steps);

	GPIBWrite(pna, ":PAGE:DISP:SET:MODE LIST"); // set the display mode to list instead of graphics
	GPIBWrite(pna, ":PAGE:DISP:SET:LIST:DEL:ALL"); // delete all variable names from list
	GPIBWrite(pna, ":PAGE:DISP:LIST 'V1','I1','V3'"); // add specified variables to list
}


// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- EXECUTING MEASUREMENT FUNCTIONS -----------------------------------
// ---------------------------------------------------------------------------------------------------------


int SPA4156B::sampleSingle(int devnum, FILE* outputs[36], int cycle){
	FILE* sb_output = outputs[devnum]; // set the output file stream to the current device we are working on
	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning

	//150427 removing this functionality as even at 250 kohms we should still read nA at 1mV
	// check whether we need to switch to a different bias voltage for this device and then do so, modifying voltageFlag as needed
	/*if (portVoltages[devnum] != voltageFlag){
		if (portVoltages[devnum] == 1) {
			GPIBWrite(pna, ":PAGE:MEAS:SAMP:CONS:SMU1 0.001");
			voltageFlag = 1;
		}
		else if (portVoltages[devnum] == 2){
			GPIBWrite(pna, ":PAGE:MEAS:SAMP:CONS:SMU1 0.01");
			voltageFlag = 2;
		}
		else if (portVoltages[devnum] == 3){
			GPIBWrite(pna, ":PAGE:MEAS:SAMP:CONS:SMU1 0.1");
			voltageFlag = 3;
		}
	}
*/
	//execute a single measurement
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");// execute the measurement then wait until instrument returns an 'IDLE' status such that measurement is complete
	int measureFlag = 1;
	while (measureFlag){
		//Sleep(1000);
		GPIBWrite(pna, "PAGE:SCON:STAT?");
		GPIBRead(pna, buffer);
		if (buffer[0] == 'I'){
			measureFlag = 0;
		}
	}

	// changed this to from SingleGateReading since seem identical
	GetReading(buffer, temporary, sb_output);
	// std::cout << "buffer is " << buffer << "\n";

	// now we assess whether the device is low, medium or high resistance, and modify the array that stores voltages to be used for each device
	// N.B. the intial value of portVoltages[i] is 1 and devices don't typically decrease in resistance, so not checking for that.
	float current = strtof(buffer, NULL);
	std::cout << "current for device # "<< devnum << " is " << current << "\n";

	int deadFlag = 0;
	//if (current < 0.000000005){
	//	std::cout << "Device # " << devnum << " is dead - removing from group.\n";
	//	deadFlag = 1;
	//}

	return deadFlag;

	//if ((current < 0.000000001) && (current > 0.0000000001)){
	//	portVoltages[devnum] = 2;
	//	//std::cout << "Changing device #" << devnum << " to sample bias of 10mV \n";
	//}
	//else if (((current < 0.00000000001) | (current > 1 && cycle > 2))){ // note: current > 1 & cycle>3 catches the invalid data 9.91e307 that sometimes happens for really low current levels, but excludes the first few data pts which sometimes give this invalid reading
	//	portVoltages[devnum] = 3;
	//	//std::cout << "Changing device #" << devnum << " to sample bias of 100mV \n";
	//}
}

int SPA4156B::constsampleSingle(int devnum, FILE* outputs[36], int cycle, float Rdead, float voltage){
	FILE* sb_output = outputs[devnum]; // set the output file stream to the current device we are working on
	// fprintf(sb_output, "Voltage (V), Current (A) \n");
	// fflush(sb_output);

	std::string temporary = "";
	char buffer[500000] = ""; // makes this larger if you increase sample points to hold a huge data dump after e.g. 5000 sample points

	//150427 removing this functionality as even at 250 kohms we should still read nA at 1mV
	// check whether we need to switch to a different bias voltage for this device and then do so, modifying voltageFlag as needed
	/*if (portVoltages[devnum] != voltageFlag){
		if (portVoltages[devnum] == 1) {
			GPIBWrite(pna, ":PAGE:MEAS:SAMP:CONS:SMU1 0.001");
			voltageFlag = 1;
		}
		else if (portVoltages[devnum] == 2){
			GPIBWrite(pna, ":PAGE:MEAS:SAMP:CONS:SMU1 0.01");
			voltageFlag = 2;
		}
		else if (portVoltages[devnum] == 3){
			GPIBWrite(pna, ":PAGE:MEAS:SAMP:CONS:SMU1 0.1");
			voltageFlag = 3;
		}
	}*/

	// POLLLING METHOD 1 - COULDN'T GET THIS WORKING
	//ibtmo(pna, 17); //set the GPIB read timeout to 1000 seconds
	////execute a single measurement
	//GPIBWrite(pna, ":PAGE:SCON:MEAS:SING; *OPC?;");// execute the measurement then wait until instrument returns an 'IDLE' status such that measurement is complete
	//

	// POLLLING METHOD 2 - COULDN'T GET THIS WORKING
	//ibtmo(pna, 17);
	//Sleep(1000);
	//GPIBWrite(pna, "*CLS;");
	//Sleep(1000);
	//GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");
	//Sleep(1000);
	//GPIBWrite(pna, "*OPC;");
	//int measureFlag = 1;
	//while (measureFlag){
	//	Sleep(1000);
	//	GPIBWrite(pna, "*ESR?");
	//	Sleep(1000);
	//	GPIBRead(pna, buffer);
	//	std::cout << "buffer is " << buffer << "\n";
	//	if (buffer[1] == '1'){
	//		measureFlag = 0;
	//	}
	//}

	// POLLLING METHOD 3 - SEEMS TO WORK....
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING");
	int measureFlag = 1;
	while (measureFlag){
		Sleep(200);
		GPIBWrite(pna, "PAGE:SCON:STAT?");
		GPIBRead(pna, buffer);
		//std::cout << buffer << "\n.";
		if (buffer[0] == 'I'){
			measureFlag = 0;
		}
	}

	// get the sampling output and write to file
	GetParsedSampleReading(buffer, temporary, sb_output);

	// now we assess whether the device is low, medium or high resistance, and modify the array that stores voltages to be used for each device
	// N.B. the intial value of portVoltages[i] is 1 and devices don't typically decrease in resistance, so not checking for that.
	std::string currstring(buffer);
	int pos = currstring.find_last_of(",");      // position of the last "," in the list of 1000 current readings
	std::string curr = currstring.substr(pos + 1);
	float current = strtof(buffer, NULL);
	std::cout << "Current is " << current << " A.\n";
	//if ((current < 0.000000001) && (current > 0.0000000001)){
	//	portVoltages[devnum] = 2;
	//}
	//else if (((current < 0.00000000001) | (current > 1 && cycle > 2))){ // note: current > 1 & cycle>3 catches the invalid data 9.91e307 that sometimes happens for really low current levels, but excludes the first few data pts which sometimes give this invalid reading
	//	portVoltages[devnum] = 3;
	//}

	int deadFlag = 0;
	if (current < voltage/Rdead){
		std::cout << "Device # " << devnum << " is dead - removing from group.\n";
		deadFlag = 1;
	}

	return deadFlag;
}

void SPA4156B::sweepSingle(int devnum, FILE* outputs[36], int gateFlag){
	FILE* sb_output = outputs[devnum]; // set the output file stream to the current device we are working on
	std::string temporary = "";
	char buffer[500000] = ""; // size of buffer is given at the beginning
	
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING"); // execute the measurement then wait until instrument returns an 'IDLE' status such that measurement is complete
	int measureFlag = 1;
	while (measureFlag){
		Sleep(1000);
		GPIBWrite(pna, "PAGE:SCON:STAT?");
		GPIBRead(pna, buffer);
		if (buffer[0] == 'I'){ 
			measureFlag = 0; 
		}
	}

	// write the headers to the output text files and then retrieve the data from SPA and write it into the text files
	if (gateFlag == 0){
		fprintf(sb_output, "Voltage (V), Current (A) \n");
		fflush(sb_output);
		GetParsedReading(buffer, temporary, sb_output);
	}
	else if(gateFlag == 1){
		fprintf(sb_output, "Source Voltage (V), Ids Current (A), Gate Voltage (V), Ig Current (A) \n");
		fflush(sb_output);
		GetGatedReading(buffer, temporary, sb_output);
	}
	else{
		fprintf(sb_output, "Source Voltage (V), Ids Current (A), Gate Voltage (V)\n");
		fflush(sb_output);
		GetGatedReadingNoIG(buffer, temporary, sb_output);
	}
}

float SPA4156B::sweepBreak(int devnum, FILE* outputs[36], float maxStart, float minStart, float targetR, float stepBias, float stepV, float temperature){

	FILE* sb_output = outputs[devnum]; // set the output file stream to the current device we are working on


	// header lines
	fprintf(sb_output, "Starting Sweep V (V) %.3f ; Initial Stopping Sweep V (V) %.3f ; Sweep Step (V) %.3f ; Step of Max V (V) %.3f ; Target Resistance (ohms) %.9f ; Temperature (K) %.6f \n ", minStart, maxStart, stepV, stepBias, targetR, temperature);


	std::string temporary = "";
	char buffer[800000] = ""; // size of buffer is given at the beginning

	std::cout << "=======================================================================\n";
	std::cout << "Press the 'F12' key to interrupt Sweep Breaking...\n";
	std::cout << "=======================================================================\n";

	float newR = 0;
	float maxBias = maxStart;
	float oldR = 0;
	int consecR = 0;


	// do an initial 100mV check and get an old Resistance baseline
	// do the check resistance 'sweep' which is really just one data point
	configSweep(0.1, 0.1, 0.001);
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");// execute the measurement then wait until instrument returns an 'IDLE' status such that measurement is complete
	int measureFlag = 1;
	while (measureFlag){
		Sleep(250);
		GPIBWrite(pna, "PAGE:SCON:STAT?");
		GPIBRead(pna, buffer);
		if (buffer[0] == 'I'){
			measureFlag = 0;
		}
	}
	float currentfloat;
	char buffer2[ARRAYSZ];
	GPIBWrite(pna, "DATA? 'I1'");
	GPIBRead(pna, buffer2);
	std::istringstream in(buffer2);
	in >> currentfloat;
	newR = 0.1 / currentfloat;
	std::cout << "Initial Resistance is " << newR << "\n";
	// note that now the while loop will not execute if newR from this initial check is already > targetR



	while (!GetAsyncKeyState(VK_F12) && (newR < targetR) && (newR >= 0) && (maxBias < 2))
	{
		// do the breaking sweep
		// configure SPA for standard sweep measurement
		configSweep(maxBias-0.01, maxBias, stepV);
		// make the integration time shorter compared to other sweeps we run
		GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM:LONG 1");
		GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM LONG");


		GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;"); // execute the measurement then wait until instrument returns an 'IDLE' status such that measurement is complete
		measureFlag = 1;
		while (measureFlag){
			Sleep(500);
			GPIBWrite(pna, "PAGE:SCON:STAT?");
			GPIBRead(pna, buffer);
			if (buffer[0] == 'I'){
				measureFlag = 0;
			}
		}

		fprintf(sb_output, "\n\n"); // write two new lines and declare the beginning of sweep data
		fflush(sb_output);
		GetParsedReading(buffer, temporary, sb_output); //get the sweep output and write to text file

		// do the check resistance 'sweep' which is really just one data point
		configSweep(0.1, 0.1, 0.001);
		GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");// execute the measurement then wait until instrument returns an 'IDLE' status such that measurement is complete
		measureFlag = 1;
		while (measureFlag){
			Sleep(250);
			GPIBWrite(pna, "PAGE:SCON:STAT?");
			GPIBRead(pna, buffer);
			if (buffer[0] == 'I'){
				measureFlag = 0;
			}
		}

		// get the new resistance 
		//float currentfloat;
		//char buffer2[ARRAYSZ];
		GPIBWrite(pna, "DATA? 'I1'");
		GPIBRead(pna, buffer2);
		std::istringstream in2(buffer2);
		in2 >> currentfloat;
		newR = 0.1 / currentfloat;
		std::cout << "New Resistance is " << newR << "\n";
		fprintf(sb_output, "Resistance = %f\n", newR); // write it into the output file
		fflush(sb_output);
		// and compare it to the old resistance to count how many consecutive times R fails to increase
		if (newR <= oldR){
			consecR = consecR + 1;
		}
		else
			consecR = 0;

		// decide whether to increase the max bias if there are just one consecutive runs with no R increase
		if (consecR >=1 ){
			maxBias = maxBias + stepBias;
			std::cout << "Stepping up the max bias to " << maxBias << "\n";
		}

		oldR = newR; // the new R becomes the old R for comparison with the next sweep
	}

	return maxBias;
}

void SPA4156B::yieldSingle(int devnum, FILE* outputs[36]){
	FILE* sb_output = outputs[0]; // set the output file stream to yield monitor file

	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning

	// the first measurements are always weird
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");
	Sleep(2000);
	//execute a single measurement (this will use the default bias of 1mV)
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");
	Sleep(2000);
	GetYieldReading(buffer, temporary, sb_output);
}

float SPA4156B::yieldGapSingle(){
	
	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning

	configSample(0.001); // reset to sampling mode 
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:CONS:SMU1 0.1"); 	// change to using a sampling bias of 100mV
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;"); // the first measurements are always weird, throw it out
	Sleep(2000); // wait a while (> 100 PLC) in case it is high R and integrates for a long time
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;"); //execute a single measurement
	Sleep(2000);

	sprintf(buffer, "");

	GPIBWrite(pna, ":DATA? 'V1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	float voltage;
	std::istringstream in(buffer);
	in >> voltage;

	GPIBWrite(pna, ":DATA? 'I1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	float current;
	std::istringstream in2(buffer);
	in2 >> current;

	float resistance = voltage / current;
	//std::cout << resistance << " ohms. \n";

	return resistance;
}

float SPA4156B::healSingle(float Vsample, FILE* output){

	// NOTE: this function expects SPA to already be in sampling mode w/ parameters defined by configSample()

	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
 
	sprintf(buffer, ":PAGE:MEAS:SAMP:CONS:SMU1 %f", Vsample);
	GPIBWrite(pna, buffer); 	// change to using the input sampling bias
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;"); // the first measurements are always weird, throw it out
	Sleep(700); // wait a while (11 PLC) in case it is high R and integrates for a long time
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;"); // the first measurements are always weird, throw it out
	Sleep(700); // wait a while (11 PLC) in case it is high R and integrates for a long time

	sprintf(buffer, "");

	GPIBWrite(pna, ":DATA? 'V1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	fprintf(output, "%s, ", buffer);
	fflush(output);
	float voltage;
	std::istringstream in(buffer);
	in >> voltage;

	GPIBWrite(pna, ":DATA? 'I1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	fprintf(output, "%s\n", buffer);
	fflush(output);
	float current;
	std::istringstream in2(buffer);
	in2 >> current;

	float resistance = voltage / current;
	//std::cout << resistance << " ohms. \n";

	fprintf(output, "%s\n", buffer);
	fflush(output);

	return resistance;
}

// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- READING AND WRITING FUNCTIONS -----------------------------------
// ---------------------------------------------------------------------------------------------------------


int SPA4156B::GPIBWrite(int ud, char* cmd){
	const int bitError = 15;
	//ibwrt writes to the device. Give it command given by the "cmd"
	//then take the returned int and go to the 15th bit of it
	//if it is 1, it means there was an error, so something did not go right
	//If error happend print out msg and the code of the error, return 1. otherwise return 0
	if ((ibwrt(ud, cmd, strlen(cmd)) >> bitError) & 1){
		printf("could not write GPIB msg on ud no %d - ", ud);
		std::cout << iberr << "\n";
		return 1;
	}
	return 0;
}

int SPA4156B::GPIBRead(int ud, char *message){
	//ibrd reads from the device.
	//then take the returned int and go to the 15th bit of it
	//if it is 1, it means there was an error, so something did not go right
	//If error happend print out msg and the code of the error
	const int bitError = 15;
	if ((ibrd(ud, message, ARRAYSZ) >> bitError) & 1){
		printf("Read GPIB msg on ud %d failed - ", ud);
		std::cout << iberr << "\n";
		return 1;
	}
	return 0;
}

int SPA4156B::GetReading(char* buffer, std::string &temporary, FILE* sb_output){

	// just dumps the unaltered V1 and I1 data into text file

	sprintf(buffer, "");

	GPIBWrite(pna, ":DATA? 'V1'");
	GPIBRead(pna, buffer);
	fprintf(sb_output, "%s\n", buffer);
	fflush(sb_output);

	//ask the device for the measured current
	GPIBWrite(pna, "DATA? 'I1'");
	GPIBRead(pna, buffer);
	fprintf(sb_output, "%s\n", buffer);
	fflush(sb_output);
	return 0;
}

int SPA4156B::GetParsedReading(char* buffer, std::string &temporary, FILE* sb_output){

	sprintf(buffer, "");

	//create the duplicate log.txt for plotting
	FILE* plotlog = fopen("log.txt", "w+");

	GPIBWrite(pna, ":DATA? 'V1';");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);

	std::string input = buffer;
	std::istringstream ss(input);
	
	GPIBWrite(pna, ":DATA? 'I1';");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);

	std::string input2 = buffer;
	std::istringstream ss2(input2);

	std::string token;
	std::string token2;

	// this loop with sequentially pull out (and write to text file) the individual values of voltage and current from the 
	// comma-separated list that was dumped into buffers above
	while (std::getline(ss, token, ',')) {
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		// N.B. spa will assert <data></n><^EOI> where EOI is it's default end of transmission character

		/*std::cout << "This is token contents: " << token << " and this is token.size " << token.size() << ".\n";
		std::cout << "This is token[token.size()] " << token[token.size()] << ".\n";
		std::cout << "This is token[token.size()-1] " << token[token.size()-1] << ".\n";
		std::cout << "This is token[token.size()-2] " << token[token.size() - 2] << ".\n";
*/
		if (!token.empty() && token[token.size() - 2] == '\n'){
			token.erase(token.size() - 2, 2);
		}
		sprintf(buffer, "%s, ", token.c_str());
		fprintf(sb_output, buffer);
		fprintf(plotlog, buffer);
		fflush(sb_output);
		fflush(plotlog);

		// Get the current data value and parse it
		std::getline(ss2, token2, ',');
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		if (!token2.empty() && token2[token2.size() - 2] == '\n'){
			token2.erase(token2.size() - 2, 2);
		}

		sprintf(buffer, "%s\n", token2.c_str());
		fprintf(sb_output, buffer);
		fprintf(plotlog, buffer);
		fflush(sb_output);
		fflush(plotlog);
	}

	fclose(plotlog);
	return 0;
}

int SPA4156B::GetParsedSampleReading(char* buffer, std::string &temporary, FILE* sb_output){

	sprintf(buffer, "");

	//create the duplicate log.txt for plotting
	FILE* plotlog = fopen("log.txt", "w+");

	GPIBWrite(pna, ":DATA? 'V1';");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);

	std::string input = buffer;
	std::istringstream ss(input);

	GPIBWrite(pna, ":DATA? 'I1';");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);

	std::string input2 = buffer;
	std::istringstream ss2(input2);

	std::string token;
	std::string token2;

	// 160319 - Modified this to correctly deal with \n and EOI at end of data dumps. 
	// this loop with sequentially pull out (and write to text file) the individual values of voltage and current from the 
	// comma-separated list that was dumped into buffers above
	while (std::getline(ss, token, ',')) {
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		// N.B. FOR SAMPLEING DATA spa will assert <data><\n><^EOI> where EOI is it's default end of transmission character
		/*std::cout << "This is token contents: " << token << " and this is token.size " << token.size() << ".\n";
		std::cout << "This is token[token.size()] " << token[token.size()] << ".\n";
		std::cout << "This is token[token.size()-1] " << token[token.size()-1] << ".\n";
		std::cout << "This is token[token.size()-2] " << token[token.size() - 2] << ".\n";*/

		if (!token.empty() && token[token.size() - 1] == '\n'){
			token.erase(token.size() - 1, 2);
		}

		sprintf(buffer, "%s, ", token.c_str());
		fprintf(sb_output, buffer);
		fprintf(plotlog, buffer);
		fflush(sb_output);
		fflush(plotlog);

		// Get the current data value and parse it
		std::getline(ss2, token2, ',');
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		if (!token2.empty() && token2[token2.size() - 1] == '\n'){
			token2.erase(token2.size() - 1, 2);
		}

		sprintf(buffer, "%s\n", token2.c_str());
		fprintf(sb_output, buffer);
		fprintf(plotlog, buffer);
		fflush(sb_output);
		fflush(plotlog);
	}

	fclose(plotlog);
	return 0;
}

int SPA4156B::GetGatedReading(char* buffer, std::string &temporary, FILE* sb_output){


	// ugly, ideally turn this into a loop through an array of pointers to the four input streams!

	sprintf(buffer, "");

	GPIBWrite(pna, ":DATA? 'V1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	std::string input = buffer;
	std::istringstream ss(input);

	GPIBWrite(pna, ":DATA? 'I1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	std::string input2 = buffer;
	std::istringstream ss2(input2);

	GPIBWrite(pna, ":DATA? 'V3'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	std::string input3 = buffer;
	std::istringstream ss3(input3);

	GPIBWrite(pna, ":DATA? 'I3'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	std::string input4 = buffer;
	std::istringstream ss4(input4);

	std::string token;
	std::string token2;
	std::string token3;
	std::string token4;

	// this loop with sequentially pull out (and write to text file) the individual values of voltage and current from the 
	// comma-separated list that was dumped into buffers above  
	while (std::getline(ss, token, ',')) {
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		if (!token.empty() && token[token.size() - 1] == '\n'){
			token.erase(token.size() - 1, 1);
		}
		fprintf(sb_output, "%s, ", token.c_str());
		fflush(sb_output);

		std::getline(ss2, token2, ',');
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		if (!token2.empty() && token2[token2.size() - 1] == '\n'){
			token2.erase(token2.size() - 1, 1);
		}
		fprintf(sb_output, "%s, ", token2.c_str());
		fflush(sb_output);

		std::getline(ss3, token3, ',');
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		if (!token3.empty() && token3[token3.size() - 1] == '\n'){
			token3.erase(token3.size() - 1, 1);
		}
		fprintf(sb_output, "%s, ", token3.c_str());
		fflush(sb_output);

		std::getline(ss4, token4, ',');
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		if (!token4.empty() && token4[token4.size() - 1] == '\n'){
			token4.erase(token4.size() - 1, 1);
		}
		fprintf(sb_output, "%s\n", token4.c_str());
		fflush(sb_output);

	}

	return 0;
}

int SPA4156B::GetGatedReadingNoIG(char* buffer, std::string &temporary, FILE* sb_output){

	// ugly, ideally turn this into a loop through an array of pointers to the four input streams!

	sprintf(buffer, "");

	GPIBWrite(pna, ":DATA? 'V1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	std::string input = buffer;
	std::istringstream ss(input);

	GPIBWrite(pna, ":DATA? 'I1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	std::string input2 = buffer;
	std::istringstream ss2(input2);

	GPIBWrite(pna, ":DATA? 'V3'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	std::string input3 = buffer;
	std::istringstream ss3(input3);

	std::string token;
	std::string token2;
	std::string token3;

	// this loop with sequentially pull out (and write to text file) the individual values of voltage and current from the 
	// comma-separated list that was dumped into buffers above  
	while (std::getline(ss, token, ',')) {
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		if (!token.empty() && token[token.size() - 1] == '\n'){
			token.erase(token.size() - 1, 1);
		}
		fprintf(sb_output, "%s, ", token.c_str());
		fflush(sb_output);

		std::getline(ss2, token2, ',');
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		if (!token2.empty() && token2[token2.size() - 1] == '\n'){
			token2.erase(token2.size() - 1, 1);
		}
		fprintf(sb_output, "%s, ", token2.c_str());
		fflush(sb_output);

		std::getline(ss3, token3, ',');
		// if it's the last parsed data point of the list then get rid of the end-of-line to avoid weirdness in the text file output
		if (!token3.empty() && token3[token3.size() - 1] == '\n'){
			token3.erase(token3.size() - 1, 1);
		}
		fprintf(sb_output, "%s\n ", token3.c_str());
		fflush(sb_output);

	}

	return 0;
}

int SPA4156B::GetYieldReading(char* buffer, std::string &temporary, FILE* sb_output){

	 // takes V1 and I1 and computes resistance, then dumps that to text file

	sprintf(buffer, "");

	GPIBWrite(pna, ":DATA? 'V1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	float voltage;
	std::istringstream in(buffer);
	in >> voltage;

	GPIBWrite(pna, ":DATA? 'I1'");
	ibwait(pna, CMPL);
	GPIBRead(pna, buffer);
	ibwait(pna, CMPL);
	float current;
	std::istringstream in2(buffer);
	in2 >> current;	

	float resistance = voltage / current;
	//std::cout << resistance << "\n";

	fprintf(sb_output, "%f\n", resistance);
	fflush(sb_output);
	std::cout << resistance << " ohms. \n";

	return 0;

/*
	sprintf(buffer, "");

	GPIBWrite(pna, ":DATA? 'V1'");
	Sleep(500);
	GPIBRead(pna, buffer);
	Sleep(500);

		fprintf(sb_output, "%s\n", buffer);
		fflush(sb_output);

	GPIBWrite(pna, ":DATA? 'I1'");
	Sleep(500);
	GPIBRead(pna, buffer);
	Sleep(500);
		fprintf(sb_output, "%s\n", buffer);
		fflush(sb_output);

	std::cout << buffer << "\n";

	return 0;*/


}


//int SPA4156B::SingleGetReading(char* buffer, std::string &temporary, FILE* sb_output){
//	sprintf(buffer, "");
//
//	GPIBWrite(pna, ":DATA? 'V1'");
//	GPIBRead(pna, buffer);
//	fprintf(sb_output, "%s\n", buffer);
//	fflush(sb_output);
//
//	//ask the device for the measured current
//	GPIBWrite(pna, ":DATA? 'I1'");
//	GPIBRead(pna, buffer);
//	fprintf(sb_output, "%s\n", buffer);
//	fflush(sb_output);
//	return 0;
//}


void SPA4156B::SendMail(std::string emailAddress){
	Spawn(0,emailAddress);
	system("Ruby  mail.rb");
}

void SPA4156B::ShowGraph(std::string emailAddress){
	Spawn(1, emailAddress);
	system("gnuscript.gp");
}

void SPA4156B::Spawn(int pick_program, std::string emailAddress){
	if (pick_program == 0){ //if we pick the ruby program, we just add the code to a string and output it in a .rb format
		std::string program = "";
		program += "require \'rubygems\' \nrequire \'net/smtp\' \nrequire \'tlsmail\'\n";
		program += "time1 = Time.new \n";
		program += "Net::SMTP.enable_tls(OpenSSL::SSL::VERIFY_NONE)\n";
		program += "FROM_EMAIL = \"cryptobatman123@gmail.com\" \nPASSWORD = \"ProjBatman\" \n";
		program += "TO_EMAIL = \"";
		program += emailAddress;
		program += "\" \n";
		program += "msgstr = <<END_OF_MESSAGE\n";
		program += "From: Your Name <#{FROM_EMAIL}> \nTo: my phone <#{TO_EMAIL}> \nSubject: finished SWEEP BREAKING <#{time1.inspect}> \n";
		program += "Date: Sat, 23 Jun 2001 16:26:43 +0900 \nMessage-Id: <unique.message.id.string@example.com> \n";
		program += "A device has finished sweep breaking. Come check me! \n";
		program += "END_OF_MESSAGE\n";
		program += "Net::SMTP.start(\'smtp.gmail.com\', 587, \'gmail.com\', \n";
		program += "FROM_EMAIL, PASSWORD, :plain) do |smtp| \n";
		program += "smtp.send_message msgstr, FROM_EMAIL, TO_EMAIL\n";
		program += "end";
		FILE* mailing = fopen("mail.rb", "w+");
		fprintf(mailing, program.c_str());
		fclose(mailing);
	}
	else if (pick_program == 1){ //if we pick the gnuscript, stuff is more complicated. Most of those cmds come from winapi or string.h
		//first of all, we need to get the path of the exec (all the log.txt files are in the same directory)
		HMODULE hModule = GetModuleHandleW(NULL);
		WCHAR path[MAX_PATH];
		GetModuleFileNameW(hModule, path, MAX_PATH);
		//then we want to get the same path in a char[] format instead of WCHAR
		char paths[250];
		char DefChar = ' ';
		WideCharToMultiByte(CP_ACP, 0, path, -1, paths, 250, &DefChar, NULL);
		//now we want to reformat the path so that we delete the name of the .exe
		std::string directory(paths);
		while (directory.at(directory.length() - 1) != '\\'){ //destroy name of the file
			directory.erase(directory.length() - 1, directory.length());
		}
		//now we want to add another "\" to every "\" that exists in the path since gnuplot needs to escape this special character
		int i = 0;
		while (i<directory.length()){
			if (directory.at(i) == '\\'){
				directory.insert(directory.begin() + sizeof(directory[0])*i, '\\');
				i++;
			}
			i++;
		}
		//now we can finally actually create the text of the script
		std::string script = "";
		script += "cd \"";
		script += directory.c_str();
		script += "\" \n";
		script += "set datafile separator \",\" \nset terminal windows \nset autoscale \nplot \"log.txt\" \npause -1 ";
		//and create the file in directory. My god, it was more fuss than I thought.
		FILE* scripts = fopen("gnuscript.gp", "w+");
		fprintf(scripts, script.c_str());
		fclose(scripts);
	}

}

