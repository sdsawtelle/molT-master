#include "SPA4145B.h"
#include <iostream>
#include <windows.h>
#include "ni488.h"
#include <cstdlib>

// constructor, every instance of SPA4145B that is created will automatically run configure.
SPA4145B::SPA4145B(){
	configure();
}


// this is the destructor method, see comments in SPA4145B.h
SPA4145B::~SPA4145B(){
	//dtor
}


// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- SETUP AND CONFIGURING FUNCTIONS -----------------------------------
// ---------------------------------------------------------------------------------------------------------


int SPA4145B::configure(){

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
	else printf("Initialization of SPA4145 was successful.\n");
	return 0;

}


void SPA4145B::setParams(int mode){

	switch (mode){
		// mode = 1 for sampling (uses SMU1 and SMU2)
	case 1:{
			   std::cout << "What is the wait time between sampling? (in seconds) \n";
			   std::cin >> wait_time;
			   std::cout << "We will use the following voltage values depending on resistance: \n";
			   std::cout << "1mV, 10mV, 100mV \n";
			   configSample();
			   break; }

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
			   break; }

		// mode 3 = yield monitoring (uses SMU1 and SMU2)
	case 3:{
			   std::cout << "We will use a voltage value of 1mV for sampling yield. \n";
			   configSample();
			   break; }

		// mode 4 = gated IV's i.e. 3-terminal sweeps (uses all SMU1,2 and 3)
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
			   std::cout << "What is the starting value for biasing the gate? (in V) \n";
			   std::cin >> Vstart_gate;
			   float Vstep_gate;
			   std::cout << "What is the voltage step size for biasing the gate? (in V) \n";
			   std::cin >> Vstep_gate;
			   int Nsteps_gate;
			   std::cout << "How many steps do you want the gate bias to increment through (i.e. number of IV sweeps)? (in V) \n";
			   std::cin >> Nsteps_gate;

			   configGatedSweeps(Vstart, Vstop, Vstep, Vstart_gate, Vstep_gate, Nsteps_gate);
			   break; }

	default:
		std::cout << "Error in setting SPA parameters \n";
		break;
	}

}

void SPA4145B::configSample(){

	// initialize portVoltages for the monitoring functions
	for (int i = 0; i < 36; i++){
		portVoltages[i] = 1;
	}

	// Most of the commands that control and set the 4155B / 56B will also update the instrument screen,but it takes time.You can enable or disable this time consuming instrument screen update as follows :
	// GPIBWrite(pna, ":DISP OFF");

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
	GPIBWrite(pna, ":PAGE:CHAN:SMU1:FUNC CONS");
	GPIBWrite(pna, ":PAGE:CHAN:SMU2:FUNC CONS");

	// This command sets the MEASUREMENT MODE
	GPIBWrite(pna, ":PAGE:CHAN:MODE SAMP");

	//This command sets the value of 'LONG' integration, then selects LONG as the the INTEGRATION TIME mode.
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM:LONG 2");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM LONG");

	//This command selects the ranging MODE of SMU<n>
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU1:RANG:MODE AUTO");

	// This command selects the sb_output sequence mode for sampling measurement. You use this command only if the measurement mode is sampling. "SIM" means all source unit starts sb_output at same timing. 
	GPIBWrite(pna, ":PAGE:MEAS:OSEQ:MODE SIM");

	//This command changes the present display page to MEASURE : SAMPLING SETUP page
	GPIBWrite(pna, ":PAGE:MEAS:SAMP");

	//This command sets the MODE for sampling measurement. The sampling mode determines the sampling interval.
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:MODE LIN");

	// This command sets the constant SOURCE value of SMU<n> for the sampling measurement. The mode of the specified SMU must be V or I.
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:CONS:SMU1 0.001");
	voltageFlag = 1;

	// This command sets the INITIAL INTERVAL for sampling measurement(in seconds)
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:IINT 0.002");

	// This command sets the HOLD TIME of sampling measurement.This is the wait time between turning on the 'sb_outputs' and taking the first sampling point measurement. Min is 30ms
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:HTIM 0.005");

	// This command sets the TOTAL SAMPLING TIME for sampling measurement (in seconds). Auto (disables total sampling time stop event, and enables the number of samples stop event.)
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:PER:AUTO ON");

	// This command sets the NUMBER OF SAMPLES for sampling measurement.
	GPIBWrite(pna, ":PAGE:MEAS:SAMP:POIN 1");

	//GPIBWrite(pna, "*WAI");
	Sleep(1000);


}

void SPA4145B::constconfigSample(){
	// DE changes to channel setup page
	//These commands set the INAME (current) and VNAME (voltage) and mode/function of SMU<n>
	GPIBWrite(pna, "DE, CH 1, 'V1', 'I1', 1, 3;"); // set smu1 to source mode = voltage, function = CONSTANT
	GPIBWrite(pna, "CH 2, 'V2', 'I2', 3, 3;"); // set smu2 to source mode = COM, function = constant
	GPIBWrite(pna, "CH 3;"); // remove smu3 and smu4
	GPIBWrite(pna, "CH 4;");

	// SS changes to source setup page
	//This command sets the sweeping parameters and compliance for VAR1 (smu1 in this case)
	GPIBWrite(pna, "SS, VC 1, 0.001, 0.001;"); // set smu1 to source 1 mV with a compliance of 1 mA
	// no need to set smu2 b/c it is COM

	// SM changes to output style page
	//change the output style to 'list' and define what variables show up in the list.
	GPIBWrite(pna, "SM, DM 2;");
	GPIBWrite(pna, "LI 'I1';");
	GPIBWrite(pna, "WT 0.01;");
	GPIBWrite(pna, "IN 0.3;");
	GPIBWrite(pna, "NR 20;");

	// Change to the list display page
	GPIBWrite(pna, "MD;");

}

void SPA4145B::configSweep(float Vstart, float Vstop, float Vstep){
	// Most of the commands that control and set the 4155B / 56B will also update the instrument screen,but it takes time.You can enable or disable this time consuming instrument screen update as follows :
	// GPIBWrite(pna, ":DISP OFF");

	//This command deletes the settings of all units (SMU,VSU,VMU,PGU,GNDU).
	// GPIBWrite(pna, ":PAGE:CHAN:ALL:DIS");

	// increase the timeout delay
	// ibtmo(pna, 13);

	// DE changes to channel setup page
	//These commands set the INAME (current) and VNAME (voltage) and mode/function of SMU<n>
	GPIBWrite(pna, "DE, CH 1, 'V1', 'I1', 1, 1;"); // set smu1 to source mode = voltage, function = variable
	GPIBWrite(pna, "CH 2, 'V2', 'I2', 3, 3;"); // set smu2 to source mode = COM, function = constant
	GPIBWrite(pna, "CH 3;"); // remove smu3 and smu4
	GPIBWrite(pna, "CH 4;");

	// SS changes to source setup page
	//This command sets the sweeping parameters and compliance for VAR1 (smu1 in this case)
	char starting[60];
	sprintf(starting, "SS, VR 1, %f, %f, %f, 0.001;", Vstart, Vstop, Vstep);
	GPIBWrite(pna, starting);
	// no need to set smu2 b/c it is COM

	// SM changes to output style page
	//change the output style to 'list' and define what variables show up in the list.
	GPIBWrite(pna, "SM, DM 2;");
	GPIBWrite(pna, "LI 'I1';");

	// Change to the list display page
	GPIBWrite(pna, "MD;");

}

void SPA4145B::configGatedSweeps(float Vstart, float Vstop, float Vstep, float Vstart_gate, float Vstep_gate, int Nsteps_gate){

	// For subordinate sweep measurement, you set up a secondary sweep source(VAR2)
	// in addition to a primary sweep source(VAR1).After primary sweep is completed,
	// the output of secondary sweep source is incremented or decremented by the
	// specified step value, then the primary sweep source is swept again

	// Most of the commands that control and set the 4155B / 56B will also update the instrument screen,but it takes time.You can enable or disable this time consuming instrument screen update as follows :
	// GPIBWrite(pna, ":DISP OFF");

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
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM:LONG 4");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:ITIM LONG");

	//This command selects the ranging MODE of SMU<n>
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU1:RANG:MODE AUTO");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU2:RANG:MODE AUTO");
	GPIBWrite(pna, ":PAGE:MEAS:MSET:SMU3:RANG:MODE AUTO");

	//This command sets the Ids sweeps to a single sweep with linear steps
	GPIBWrite(pna, ":PAGE:MEAS:VAR1:MODE SINGLE");
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
	GPIBWrite(pna, ":PAGE:MEAS:VAR2:MODE SINGLE");
	GPIBWrite(pna, ":PAGE:MEAS:VAR2:SPAC LIN");

	// set the sweep parameters for the Gate bias incrementing
	char starting_gate[60];
	sprintf(starting_gate, ":PAGE:MEAS:VAR2:START %f", Vstart_gate);
	std::cout << starting_gate;
	GPIBWrite(pna, starting_gate);

	char stepping_gate[60];
	sprintf(stepping_gate, ":PAGE:MEAS:VAR2:STEP %f", Vstep_gate);
	std::cout << stepping_gate;
	GPIBWrite(pna, stepping_gate);

	char steps[60];
	sprintf(steps, ":PAGE:MEAS:VAR2:POINTS %i", Nsteps_gate);
	std::cout << steps;
	GPIBWrite(pna, steps);

	std::cout << "done setting up spa \n";
}



// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- EXECUTING MEASUREMENT FUNCTIONS -----------------------------------
// ---------------------------------------------------------------------------------------------------------



int SPA4145B::sampleSingle(int devnum, FILE* outputs[36], int cycle){
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
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");
	GPIBWrite(pna, "*WAI");

	// changed this to from SingleGateReading since seem identical
	GetReading(buffer, temporary, sb_output);
	// std::cout << "buffer is " << buffer << "\n";

	// now we assess whether the device is low, medium or high resistance, and modify the array that stores voltages to be used for each device
	// N.B. the intial value of portVoltages[i] is 1 and devices don't typically decrease in resistance, so not checking for that.
	float current = strtof(buffer, NULL);
	std::cout << "current for device # " << devnum << " is " << current << "\n";

	int deadFlag = 0;
	if (current < 0.000000005){
		std::cout << "Device # " << devnum << " is dead - removing from group.\n";
		deadFlag = 1;
	}

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


void SPA4145B::constsampleSingle(FILE* output, int cycle){
	FILE* sb_output = output; // set the output file stream to the current device we are working on
	std::string temporary = "";
	char buffer[10000] = ""; // makes this larger if you increase sample points to hold a huge data dump after e.g. 5000 sample points

	//execute a single measurement
	GPIBWrite(pna, "ME 1;");
	Sleep(7000);
	// changed from SingleGetReading since didn't seem to be any different
	
	//ask the device for the measured current
	GPIBWrite(pna, "DO 'I1';");
	Sleep(100);
	GPIBRead(pna, buffer);
	fprintf(sb_output, "%s\n", buffer);
	fflush(sb_output);
	
	std::cout << buffer << "\n";
	// GetReading(buffer, temporary, sb_output);

	// now we assess whether the device is low, medium or high resistance, and modify the array that stores voltages to be used for each device
	// N.B. the intial value of portVoltages[i] is 1 and devices don't typically decrease in resistance, so not checking for that.
	std::string currstring(buffer);
	int pos = currstring.find_last_of(",");      // position of the last "," in the list of 1000 current readings
	std::string curr = currstring.substr(pos + 2);
	std::cout << curr << "\n";
	float current = strtof(buffer, NULL);
	std::cout << "current is " << current << "\n";
	//if ((current < 0.000000001) && (current > 0.0000000001)){
	//	portVoltages[devnum] = 2;
	//}
	//else if (((current < 0.00000000001) | (current > 1 && cycle > 2))){ // note: current > 1 & cycle>3 catches the invalid data 9.91e307 that sometimes happens for really low current levels, but excludes the first few data pts which sometimes give this invalid reading
	//	portVoltages[devnum] = 3;
	//}
}



void SPA4145B::sweepSingle(int devnum, FILE* outputs[36], int gateFlag){
	FILE* sb_output = outputs[devnum]; // set the output file stream to the current device we are working on
	fprintf(sb_output, "\n SPA Sweep \n"); // write a new line and declare the beginning of sweep data
	std::string temporary = "";
	char buffer[500000] = ""; // size of buffer is given at the beginning
	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");
	GPIBWrite(pna, "*WAI");
	if (gateFlag == 0){
		GetReading(buffer, temporary, sb_output);
	}
	else{
		GetGatedReading(buffer, temporary, sb_output);
	}
}


void SPA4145B::sweepBreak(FILE* output, float maxStart, float minStart, float targetR, float stepBias, float stepV){

	FILE* sb_output = output; // set the output file stream to the current device we are working on
	fprintf(sb_output, "\n\n"); // write two new lines and declare the beginning of sweep data
	fflush(sb_output);
	std::string temporary = "";
	char buffer[800000] = ""; // size of buffer is given at the beginning

	std::cout << "Press 'F12' to stop the sweeping... \n";

	float newR = 0;
	float maxBias = maxStart;
	float oldR = 0;
	int consecR = 0;

	while (!GetAsyncKeyState(VK_F12) && (newR < targetR) && (maxBias < 1.5))
	{
		// do the breaking sweep
		// configure SPA for standard sweep measurement
		configSweep(minStart, maxBias, 0.002);

		GPIBWrite(pna, "ME 1;"); // execute single measurement
		//GPIBWrite(pna, "*WAI;"); // wait for the above measurement to finish executing before spa returns control
		// GetReading(buffer, temporary, sb_output);
		Sleep(20000);
		// do the check resistance 'sweep' which is really just one data point
		configSweep(0.1, 0.1, 0.001);
		GPIBWrite(pna, "ME 1;"); // execute single measurement
		Sleep(2000);
		// GPIBWrite(pna, "*WAI;"); // wait for the above measurement to finish executing before spa returns control

		// get the new resistance 
		float currentfloat;
		char buffer2[ARRAYSZ];
		GPIBWrite(pna, "DO 'I1';");
		Sleep(20000);
		GPIBRead(pna, buffer2);
		memmove(buffer2, buffer2 + 1, (ARRAYSZ - 2));
		std::istringstream in(buffer2);
		in >> currentfloat;
		newR = 0.1 / currentfloat;
		std::cout << "Measured current is " << currentfloat << "\n";
		std::cout << "New Resistance is " << newR << "\n";
		fprintf(sb_output, "%f\n", newR); // write it into the output file
		fflush(sb_output);
		// and compare it to the old resistance to count how many consecutive times R fails to increase
		if (newR <= oldR){
			consecR = consecR + 1;
		}
		else
			consecR = 0;


		// decide whether to increase the max bias if there are two consecutive runs with no R increase
		if (consecR >= 2){
			maxBias = maxBias + stepBias;
			std::cout << "Stepping up the max bias to " << maxBias << "\n";
		}

		oldR = newR; // the new R becomes the old R for comparison with the next sweep
	}
}


void SPA4145B::yieldSingle(int devnum, FILE* outputs[36]){
	FILE* sb_output = outputs[0]; // set the output file stream to yield monitor file

	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning

	//execute a single measurement (this will use the default bias of 1mV)

	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");
	GPIBWrite(pna, "*WAI");

	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");
	GPIBWrite(pna, "*WAI");

	// for some reason first measurement is always crap and returns invalid data...?
	//if (devnum == 0){ 

	//	GPIBWrite(pna, ":PAGE:SCON:MEAS:SING;");
	//	//GPIBWrite(pna, "*WAI");
	//	Sleep(1000);
	//}

	GetReading(buffer, temporary, sb_output);
}



// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- READING AND WRITING FUNCTIONS -----------------------------------
// ---------------------------------------------------------------------------------------------------------


int SPA4145B::GPIBWrite(int ud, char* cmd){
	const int bitError = 15;
	//ibwrt writes to the device. Give it command given by the "cmd"
	//then take the returned int and go to the 15th bit of it
	//if it is 1, it means there was an error, so something did not go right
	//If error happend print out msg and the code of the error, return 1. otherwise return 0
	if ((ibwrt(ud, cmd, strlen(cmd)) >> bitError) & 1){
		printf("could not write GPIB msg on ud no %d\n", ud);
		std::cout << iberr;
		return 1;
	}
	return 0;
}

int SPA4145B::GPIBRead(int ud, char *message){
	//ibrd reads from the device.
	//then take the returned int and go to the 15th bit of it
	//if it is 1, it means there was an error, so something did not go right
	//If error happend print out msg and the code of the error
	const int bitError = 15;
	if ((ibrd(ud, message, ARRAYSZ) >> bitError) & 1){
		printf("Read GPIB msg on ud %d failed\n", ud);
		std::cout << iberr;
		return 1;
	}
	return 0;
}


int SPA4145B::GetReading(char* buffer, std::string &temporary, FILE* sb_output){

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



int SPA4145B::GetParsedReading(char* buffer, std::string &temporary, FILE* sb_output){

	sprintf(buffer, "");

	GPIBWrite(pna, ":DATA? 'V1'");
	GPIBRead(pna, buffer);


	std::string input = buffer;
	std::istringstream ss(input);
	std::string token;

	while (std::getline(ss, token, ',')) {
		std::cout << token << '\n';
	}

	return 0;
}



int SPA4145B::GetGatedReading(char* buffer, std::string &temporary, FILE* sb_output){

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

	GPIBWrite(pna, ":DATA? 'V3'");
	GPIBRead(pna, buffer);
	fprintf(sb_output, "%s\n", buffer);
	fflush(sb_output);

	//ask the device for the measured current
	GPIBWrite(pna, "DATA? 'I3'");
	GPIBRead(pna, buffer);
	fprintf(sb_output, "%s\n", buffer);
	fflush(sb_output);

	return 0;
}


int SPA4145B::GetYieldReading(char* buffer, std::string &temporary, FILE* sb_output){

	// takes V1 and I1 and computes resistance, then dumps that to text file

	sprintf(buffer, "");

	GPIBWrite(pna, ":DATA? 'V1'");
	GPIBRead(pna, buffer);
	float voltage = atof(buffer);
	std::cout << voltage << "\n";

	GPIBWrite(pna, "DATA? 'I1'");
	GPIBRead(pna, buffer);
	float current = atof(buffer);
	std::cout << current << "\n";

	float resistance = voltage / current;
	std::cout << resistance << "\n";

	fprintf(sb_output, "%f ohms\n", resistance);
	fflush(sb_output);
	return 0;
}

//int SPA4145B::SingleGetReading(char* buffer, std::string &temporary, FILE* sb_output){
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