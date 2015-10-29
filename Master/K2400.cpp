#include "K2400.h"
#include <cmath>

// #include "graph.h"
#include <iostream>
#include <windows.h>
#include "ni488.h"
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <math.h> //for floor
// constructor, every instance of k2400 that is created will automatically run configure.
K2400::K2400(){
	configure();
}

// this is the destructor method, see comments in k2400.h
K2400::~K2400(){
	//dtor
}


int K2400::configure(){

	int ERROR1 = 0; // error information. if any problem happens, this variable will go to 1;
	std::cout << "Initializing Keithley 2400...\n";

	this->pna = ibdev(0, PRIMARY_ADDR_OF_KEITHLEY, NO_SECONDARY_ADDR, TIMEOUT, EOTMODE, EOSMODE); //ibdev gives back ud! Unique iDentifier based on info given in

	//clear GPIB communication interface
	ibclr(pna);
	//if there is a communication error, display error message and get the error flag up! We will catch it later, at the very end.
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

	//Now we will initialize the sourcemeters
	printf("initializing K2400 sourcemeter parameters\n");
	// sourcing voltage
	GPIBWrite(pna, ":SOUR:FUNC VOLT");
	// sensing mode DC current
	GPIBWrite(pna, ":SENS:FUNC 'CURR:DC'");


	//       // voltage source range in auto
	//GPIBWrite(pna,":SOUR:VOLT:RANGE:AUTO ON");
	//       // current sensing range in auto
	//GPIBWrite(pna,":SENS:CURR:RANGE:AUTO ON");
	// Auto ranging seemed to cause exploding of devices.....???
	// Failing to auto range may be giving us really fucking weird measurements from keithley


	// current protection (compliance) set at 1A
	GPIBWrite(pna, ":SENS:CURR:PROT 1.0");
	// fixed voltage mode
	GPIBWrite(pna, ":SOUR:VOLT:MODE FIXED");

	char delays[20];
	sprintf(delays, ":SOUR:DEL %f", 0.02);
	GPIBWrite(pna, delays);

	// terminals connected on the front
	GPIBWrite(pna, ":ROUT:TERM FRONT");
	// 2w measurement
	GPIBWrite(pna, ":SYST:RSEN OFF");
	// setting output format to V I
	GPIBWrite(pna, ":FORM:ELEM VOLT,CURR");
	// checking possible error of GPIB and print stuff if there are an
	// we also print the code of the error
	// voltage source range in auto
	GPIBWrite(pna, ":SOUR:VOLT:RANGE 21");
	// current sensing range in auto
	GPIBWrite(pna, ":SENS:CURR:RANGE 30E-3");


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
	else printf("Initialization of Keithley 2400 was successful.\n");
	return 0;
}

float K2400::setParamsEM(){

	int choice;
	std::cout << ">>>>>>>>>>> Use Default Parameters for EM? ('0' for No, '1' for Yes)\n";
	std::cin >> choice;

	if (choice){
		delay = 0;
		volt_ramp = 0.001;
		volt_stop = 4;
		volt_start = 0.001;
		resistance_tolerance = 0.025;
		resistance_tolerance_high = 0.06;
		target_resistance = 10000;
		target_resistance_tolerance = 0.001;
		volt_down = 0.06;
		ntrigger = 2;
		ntrigger_high = 6;
		nnegres = 2;
		nnegres_high = 6;
		nplc = 1;
		rampdown_dwell = 1;
		res_switch = 200;
	}
	else{
		// delay is always set as 0 so hardcode it for now as of 150724
		//std::cout << "Delay between ramping the voltage? (in seconds)\n";
		//std::cin >> delay;
		delay = 0;
		std::cout << "By how much are we ramping the voltage? (in volts)\n";
		std::cin >> volt_ramp;
		std::cout << "What is the stop voltage? \n";
		std::cin >> volt_stop;
		std::cout << "At what voltage do we want to start? \n";
		std::cin >> volt_start;
		std::cout << "What is the tolerance for resistance in low-R regime? Give a percentage in the float format (ex. 20% - 0.2)\n";
		std::cin >> resistance_tolerance;
		std::cout << "What is the tolerance for resistance in high-R regime? Give a percentage in the float format (ex. 20% - 0.2)\n";
		std::cin >> resistance_tolerance_high;
		std::cout << "What is the target resistance? \n";
		std::cin >> target_resistance;
		std::cout << "What is the tolerance for target resistance? Give a percentage in the float format (ex. 20% - 0.2)\n";
		std::cin >> target_resistance_tolerance;
		// volt_down is not being used as of 150724, hard code some reasonable value to pass
		//std::cout << "What is the voltage_down? (By how much do we go \" down \" after we reach the change in resistance?) \n";
		//std::cin >> volt_down;
		volt_down = 0.01;
		std::cout << "How many consecutive R<R_benchmark hits before triggering a ramp down in low-R regime? \n";
		std::cin >> ntrigger;
		std::cout << "How many consecutive R<R_benchmark hits before triggering a ramp down in high-R regime? \n";
		std::cin >> ntrigger_high;
		std::cout << "How many consecutive NDR hits before triggering a ramp down in low-R regime? \n";
		std::cin >> nnegres;
		std::cout << "How many consecutive NDR hits before triggering a ramp down in high-R regime? \n";
		std::cin >> nnegres_high;
		// nplc is always being set as 1 so hardcode it for now
		//std::cout << "What integer number of power line cycles to integrate over for taking current readings? (10 max) \n";
		//std::cin >> nplc;
		nplc = 1;
		// volt_down is not being used as of 150724, hard code some reasonable value to pass
		//std::cout << "How many msec to dwell on each point in the rampdown (rampdown rate will be 1mV/dwell) \n";
		//std::cin >> rampdown_dwell;
		rampdown_dwell = 1;
		std::cout << "At what resistance value will we switch to high-R regime? (in ohms) \n";
		std::cin >> res_switch;
	}



	std::cout << "Please check TempB on Lakeshore and input in Kelvin \n";
	std::cin >> temperature;

	char integration[30];
	sprintf(integration, ":SENS:CURR:NPLC %i", nplc);
	GPIBWrite(pna, integration);

	// voltage sensing range - 2V is the next lowest range but some devices do require 2+V
	GPIBWrite(pna, ":SOUR:VOLT:RANGE 21");
	// current sensing rangE
	GPIBWrite(pna, ":SENS:CURR:RANGE 30E-3");

	GPIBWrite(pna, ":SOUR:VOLT 0.0");                    // get the bias to 0

	if (GPIBWrite(pna, ":OUTP ON")){                   // turn the output ON - if fail, proceed to sending an info on screen
		printf("GPIB error while turning on sourcemeter\n");
	}
	// else printf("Keithley output on! Starting measurement! \n");
	Sleep(500);

	return temperature;
}

void K2400::initializeEM(){
	// voltage sensing range - 2V is the next lowest range but some devices do require 2+V
	GPIBWrite(pna, ":SOUR:VOLT:RANGE 21");
	// current sensing rangE
	GPIBWrite(pna, ":SENS:CURR:RANGE 30E-3");

	GPIBWrite(pna, ":SOUR:VOLT 0.0");                    // get the bias to 0

	if (GPIBWrite(pna, ":OUTP ON")){                   // turn the output ON - if fail, proceed to sending an info on screen
		printf("GPIB error while turning on sourcemeter\n");
	}
	// else printf("Keithley output on! Starting measurement! \n");
	Sleep(500);
}

void K2400::setParamsSweep(int* useEMV){
	int choice;
	std::cout << ">>>>>>>>>>> Use Default Parameters for Keithley Sweep? ('0' for No, '1' for Yes)\n";
	std::cin >> choice;

	if (choice){
		volt_ramp_KS = 0.00005;
		volt_stop_KS = 2;
		volt_start_KS = 0.5;
		target_resistance_KS = 280000;
		*useEMV = 1;
	}
	else{
		std::cout << "By how much are we ramping the voltage? (in volts)\n";
		std::cin >> volt_ramp_KS;
		std::cout << "What is the stop voltage (max allowed voltage)? \n";
		std::cin >> volt_stop_KS;
		std::cout << "At what voltage do we want to start? \n";
		std::cin >> volt_start_KS;
		std::cout << "What is the target resistance? \n";
		std::cin >> target_resistance_KS;
		std::cout << "Would you like to use the exit voltages for EM as the starting KS voltages? ('0' for NO, '1' for YES) \n";
		std::cin >> *useEMV;
	}
}

void K2400::initializeSweep(){	
	
	char integration[30];
	sprintf(integration, ":SENS:CURR:NPLC %i", nplc);
	GPIBWrite(pna, integration);

	GPIBWrite(pna, ":SOUR:VOLT:RANGE 1");  // voltage source range to 2V so we can step at 50 uV
	GPIBWrite(pna, ":SENS:CURR:RANG 100E-6");
	GPIBWrite(pna, ":SOUR:VOLT 0.0");    // get the bias to 0                
	if (GPIBWrite(pna, ":OUTP ON")){                   // turn the output ON - if fail, proceed to sending an info on screen
		printf("GPIB error while turning on sourcemeter\n");
	}
	Sleep(500);
}

void K2400::setParamsDwell(){
	int choice;
	std::cout << ">>>>>>>>>>> Use Default Parameters for Keithley Dwelling? ('0' for No, '1' for Yes)\n";
	std::cin >> choice;

	if (choice){
		delay = 0;
		volt_ramp = 0.001;
		volt_stop = 4;
		volt_start = 0.001;
		//resistance_tolerance = 0.025;
		//resistance_tolerance_high = 0.06;
		target_resistance = 200000;
		//target_resistance_tolerance = 0.001;
		//volt_down = 0.06;
		//ntrigger = 2;
		//ntrigger_high = 6;
		//nnegres = 2;
		//nnegres_high = 6;
		//nplc = 1;
		//rampdown_dwell = 1;
		//res_switch = 200;
	}
	else{
		// delay is always set as 0 so hardcode it for now as of 150724
		//std::cout << "Delay between ramping the voltage? (in seconds)\n";
		//std::cin >> delay;
		delay = 0;
		std::cout << "By how much are we ramping the voltage? (in volts)\n";
		std::cin >> volt_ramp;
		std::cout << "What is the stop voltage? \n";
		std::cin >> volt_stop;
		std::cout << "At what voltage do we want to start? \n";
		std::cin >> volt_start;
		/*std::cout << "What is the tolerance for resistance in low-R regime? Give a percentage in the float format (ex. 20% - 0.2)\n";
		std::cin >> resistance_tolerance;
		std::cout << "What is the tolerance for resistance in high-R regime? Give a percentage in the float format (ex. 20% - 0.2)\n";
		std::cin >> resistance_tolerance_high;*/
		std::cout << "What is the target resistance? \n";
		std::cin >> target_resistance;
		//std::cout << "What is the tolerance for target resistance? Give a percentage in the float format (ex. 20% - 0.2)\n";
		//std::cin >> target_resistance_tolerance;
		//// volt_down is not being used as of 150724, hard code some reasonable value to pass
		////std::cout << "What is the voltage_down? (By how much do we go \" down \" after we reach the change in resistance?) \n";
		////std::cin >> volt_down;
		//volt_down = 0.01;
		//std::cout << "How many consecutive R<R_benchmark hits before triggering a ramp down in low-R regime? \n";
		//std::cin >> ntrigger;
		//std::cout << "How many consecutive R<R_benchmark hits before triggering a ramp down in high-R regime? \n";
		//std::cin >> ntrigger_high;
		//std::cout << "How many consecutive NDR hits before triggering a ramp down in low-R regime? \n";
		//std::cin >> nnegres;
		//std::cout << "How many consecutive NDR hits before triggering a ramp down in high-R regime? \n";
		//std::cin >> nnegres_high;
		//// nplc is always being set as 1 so hardcode it for now
		////std::cout << "What integer number of power line cycles to integrate over for taking current readings? (10 max) \n";
		////std::cin >> nplc;
		//nplc = 1;
		//// volt_down is not being used as of 150724, hard code some reasonable value to pass
		////std::cout << "How many msec to dwell on each point in the rampdown (rampdown rate will be 1mV/dwell) \n";
		////std::cin >> rampdown_dwell;
		//rampdown_dwell = 1;
		//std::cout << "At what resistance value will we switch to high-R regime? (in ohms) \n";
		//std::cin >> res_switch;
	}



	std::cout << "Please check TempB on Lakeshore and input in Kelvin \n";
	std::cin >> temperature;

	char integration[30];
	sprintf(integration, ":SENS:CURR:NPLC %i", nplc);
	GPIBWrite(pna, integration);

	// current sensing range
	GPIBWrite(pna, ":SENS:CURR:RANGE 30E-3");

	GPIBWrite(pna, ":SOUR:VOLT 0.0");                    // get the bias to 0

	if (GPIBWrite(pna, ":OUTP ON")){                   // turn the output ON - if fail, proceed to sending an info on screen
		printf("GPIB error while turning on sourcemeter\n");
	}
	// else printf("Keithley output on! Starting measurement! \n");
	Sleep(500);
}

float K2400::sweepSingle(int devnum, FILE* outputs[36], float Vstart){
	// want to keep the temp data in the file with other params
	FILE* log_keithley = fopen("log.txt", "w+");
	FILE* reading_log_keithley = fopen("log.txt", "r");

	FILE* output = outputs[devnum];
	fprintf(output, "DELAY (SEC) %.3f ; RAMP (V) %.4f ; MAX VOLT (V) %.2f ; TARGET RESISTANCE (Ohms) %.9f; TEMPERATURE (K) %.4f \n ", delay, volt_ramp_KS, volt_stop_KS, target_resistance_KS, temperature);
	fprintf(output, "voltage,current,resistance \n");
	fflush(output);

	float exitV = 0;
	
	// if the user chose to use EM exit voltages then Vstart will have been reassigned to some number that isn't -1
	if (Vstart != -1){
		volt_start_KS = Vstart;
	}

	std::cout << "=======================================================================\n";
	std::cout << "Press the 'F12' key to interrupt Keithley Sweep...\n";
	std::cout << "=======================================================================\n";

	exitV = DoSweep(log_keithley, reading_log_keithley, output, delay); //Here the real measurement is done. See comments within the function

	GPIBWrite(pna, ":SOUR:VOLT 0.0"); // set output back to 0 V in preparation for closing channel to next device to be EM'd

	//close the streams
	fclose(log_keithley);
	fclose(reading_log_keithley);
	// fclose(output); leave it open for now, since might monitor self-breaking. The closeFiles function in menu.h will take care of it
	return exitV;
}

float K2400::dwellSingle(int devnum, FILE* outputs[36], float dwellV){
	// want to keep the temp data in the file with other params
	FILE* log_keithley = fopen("log.txt", "w+");
	FILE* reading_log_keithley = fopen("log.txt", "r");

	FILE* output = outputs[devnum];
	fprintf(output, "DELAY (SEC) %.3f ; RAMP (V) %.4f ; MAX VOLT (V) %.2f ; TARGET RESISTANCE (Ohms) %.9f; TEMPERATURE (K) %.4f \n ", delay, volt_ramp, volt_stop, target_resistance, temperature);
	fprintf(output, "voltage,current,resistance \n");
	fflush(output);

	float exitV = 0;

	std::cout << "=======================================================================\n";
	std::cout << "Press the 'F12' key to interrupt Keithley Sweep...\n";
	std::cout << "=======================================================================\n";

	exitV = DoDwell(log_keithley, reading_log_keithley, output, delay, dwellV); //Here the real measurement is done. See comments within the function

	GPIBWrite(pna, ":SOUR:VOLT 0.0"); // set output back to 0 V in preparation for closing channel to next device to be EM'd

	//close the streams
	fclose(log_keithley);
	fclose(reading_log_keithley);
	// fclose(output); leave it open for now, since might monitor self-breaking. The closeFiles function in menu.h will take care of it
	return exitV;
}

float K2400::emSingle(int devnum, FILE* outputs[36]){
	// want to keep the temp data in the file with other params
	FILE* log_keithley = fopen("log.txt", "w+");
	FILE* reading_log_keithley = fopen("log.txt", "r");

	FILE* output = outputs[devnum];
	fprintf(output, "DELAY (SEC) %.3f ; RAMP (V) %.4f ; MAX VOLT (V) %.2f ; RAMP RESISTANCE TOLERANCE (low R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (high R) (float %%) %.3f ; TARGET RESISTANCE (Ohms) %.9f ; TARGET RESISTANCE TOLERANCE (float %%) %.3f ; RAMP DOWN (V) %.3f ; NUM CONSEC % HITS (low R) %i ; NUM CONSEC % HITS (high R) %i ; NUM CONSEC NDR (low R) %i ; NUM CONSEC NDR (high R) %i ; HIGH/LOW SWITCH RESISTANCE (ohms) %.9f ; INTEGRATION TIME (PLC) %i ; RAMPDOWN DWELL (msec/mV) %.4i ; TEMPERATURE (K) %.4f \n ", delay, volt_ramp, volt_stop, resistance_tolerance, resistance_tolerance_high, target_resistance, target_resistance_tolerance, volt_down, ntrigger, ntrigger_high, nnegres, nnegres_high, res_switch, nplc, rampdown_dwell, temperature);
	fprintf(output, "voltage,current,resistance \n");
	fflush(output);

	float exitV = 0;

	std::cout << "=======================================================================\n";
	std::cout << "Press the 'F12' key to interrupt active EM...\n";
	std::cout << "=======================================================================\n";

	exitV = DoMeasurement(log_keithley, reading_log_keithley, output, delay); //Here the real measurement is done. See comments within the function

	GPIBWrite(pna, ":SOUR:VOLT 0.0"); // set output back to 0 V in preparation for closing channel to next device to be EM'd

	//close the streams
	fclose(log_keithley);
	fclose(reading_log_keithley);
	// fclose(output); leave it open for now, since might monitor self-breaking. The closeFiles function in menu.h will take care of it

	//send mail!
	// SendMail();
	//show graph
	// ShowGraph();

	return exitV;
}

int K2400::emSingleProbeStation(){
	// want to keep the temp data in the file with other params
	FILE* log_keithley = fopen("log.txt", "w+");
	FILE* reading_log_keithley = fopen("log.txt", "r");

	std::string file_out;
	std::cout << "What is the stem to use for output file name i.e. chip and device ID?\n";
	std::cin >> file_out;

	char filebuffer[1024] = "";
	sprintf(filebuffer, "%s_EM.txt", file_out.c_str());
	FILE* output = fopen(filebuffer, "w+");

	fprintf(output, "DELAY (SEC) %.3f ; RAMP (V) %.4f ; MAX VOLT (V) %.2f ; RAMP RESISTANCE TOLERANCE (float %%) %.3f ; TARGET RESISTANCE (Ohms) %.9f ; TARGET TOLERANCE (low R) (float %%) %.3f ; TARGET TOLERANCE (high R) (float %%) %.3f ;RAMP DOWN (V) %.3f ; NUM CONSEC % HITS (low R) %i ; NUM CONSEC % HITS (high R) %i ; NUM CONSEC NDR (low R) %i ; NUM CONSEC NDR (high R) %i ; HIGH/LOW SWITCH RESISTANCE (ohms) %.9f ; INTEGRATION TIME (PLC) %i ; RAMPDOWN DWELL (msec/mV) %.4i ; TEMPERATURE (K) %.4f \n ", delay, volt_ramp, volt_stop, resistance_tolerance, resistance_tolerance_high, target_resistance, target_resistance_tolerance, volt_down, ntrigger, ntrigger_high, nnegres, nnegres_high, res_switch, nplc, rampdown_dwell, temperature);
	fprintf(output, "voltage,current,resistance \n");
	fflush(output);

	DoMeasurement(log_keithley, reading_log_keithley, output, delay); //Here the real measurement is done. See comments within the function

	GPIBWrite(pna, ":SOUR:VOLT 0.0"); // set output back to 0 V in preparation for closing channel to next device to be EM'd

	//close the streams
	fclose(log_keithley);
	fclose(reading_log_keithley);

	return 0;
}

float K2400::DoMeasurement(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay){
	//initialize local variables
	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	int targ_res_consecutive = 0;
	int neg_res_consecutive = 0;
	int bench_res_consecutive = 0;
	int res_consecutive = 0;
	resistance_timely = 0;
	resistance_benchmark = 0;
	float diffres = 0;
	float voltage_write = volt_start - volt_ramp; //we omit the point V=0 by increasing the voltage before measurement!
	float voltage_read = 0;
	float current_read = 0;

	char delays[40] = "";
	sprintf(delays, ":SOUR:DEL %f", delay);
	int counter = 0; //number of measurements since the most recent ramp_down
	//execute the code in for loop until we get the required resistance +/- resistance_tol*resistance
	for (; GotTargetResistance(resistance_timely, targ_res_consecutive, volt_stop, voltage_read);){

		voltage_write += volt_ramp;    //setting new voltage on Keithley
		sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);    //create a new string of a command
		GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
		GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files

		//compute the resistance on basis of measurement from Keithley. Also writes it to output and log
		// 140422 (sonya) temporarily try just V/I instead of differential resistance computation - see GetResistance function
		resistance_timely = GetResistance(&diffres, voltage_read, current_read, reading_log_keithley, output, counter);

		//this function keeps tracks of the count of consecutive weird measurements and takes care of setting resistance_benchmark
		KeepTrackIndicators(res_consecutive, targ_res_consecutive, neg_res_consecutive, bench_res_consecutive, resistance_benchmark,
			resistance_timely, diffres, target_resistance, target_resistance_tolerance, counter);

		counter++;  //increase the counter to keep track of whether we are at the beginning of ramp cycle or not

		//check if we need to ramp down the voltage. If so, do it and zero the counter. Check if we
		//decrease the voltage below 0. if so, break the measurement. display message.
		if (CheckRampDown(resistance_timely,res_consecutive, bench_res_consecutive, neg_res_consecutive)){

			// 140430 (SDS) instantaneously ramp down to 50% of present voltage
			voltage_write = 0.8*voltage_write;
			sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
			GPIBWrite(pna, buffer);
			if (voltage_write <= 0){
				printf("ERROR! We just tried to lower the voltage below 0! Stopping the measurement! \n");
				break;
			}

			// // 140502 (sds) ramp back at user input rate and ramp back by amount 10% of current voltage
			//volt_down = 0.3*voltage_write;
			//int ramp_round = floor(volt_down * 1000); 	 // round the ramp down voltage to nearest integer in mV
			// sweep the voltage down by a total amount of volt_down, but at 1 mV steps with short delay, and without taking any readings.
			//for (int k = 0; k < ramp_round; k++){
			// Sleep(rampdown_dwell); //wait for dwell time miliseconds then proceed with this thread. since we have only one thread it means that we just wait for some time.
			// voltage_write = voltage_write - 0.001;
			// sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
			// GPIBWrite(pna, buffer);
			// if (voltage_write <= 0){
			//	 printf("ERROR! We just tried to lower the voltage below 0! Stopping the measurement! \n");
			//	 break;
			// }
			//	 }
			// GPIBWrite(pna, delays); 	 //go back to using the user-set delay

			counter = 0; 	 // after any ramp down reset counter to 0 so it will take new benchmark resistance on next 6 iterations of loop.
			bench_res_consecutive = 0; // we have to get this to 0. otherwise we might run into trouble with many ramp downs one after another
			neg_res_consecutive = 0; // we have to get this to 0. otherwise we might run into trouble with many ramp downs one after another
			res_consecutive = 0;

		}
	}
	return voltage_write;
}

float K2400::DoSweep(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay){
	//initialize local variables
	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	int targ_res_consecutive = 0;
	resistance_timely = 0;
	float target_resistance_scaled = 0;
	resistance_benchmark = 0;
	float diffres = 0;
	float voltage_write = volt_start_KS - volt_ramp_KS; //we omit the point V=0 by increasing the voltage before measurement!
	float voltage_read = 0;
	float current_read = 0;
	char delays[40] = "";
	sprintf(delays, ":SOUR:DEL %f", delay);
	int counter = 0; //hardcoding counter to 0 and never incrementing it means DON'T do differential resistance, do R = V/I

	//execute the code in for loop until we get the required resistance
	while (!targ_res_consecutive && voltage_write < volt_stop_KS && !GetAsyncKeyState(VK_F12)){
		voltage_write += volt_ramp_KS;    //setting new voltage on Keithley
		sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);    //create a new string of a command
		GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
		GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files

		//compute the resistance on basis of measurement from Keithley. Also writes it to output and log
		// 140422 (sonya) temporarily try just V/I instead of differential resistance computation - see GetResistance function
		resistance_timely = GetResistance(&diffres, voltage_read, current_read, reading_log_keithley, output, counter);
		std::cout << resistance_timely << " Ohms @ " << voltage_write << " V \n";

		if (voltage_write < 0.2){
			target_resistance_scaled = target_resistance_KS;
		}
		else if (voltage_write > 1.0){
			target_resistance_scaled = 150000;
		}
		else{
			target_resistance_scaled = 400000 - pow((voltage_write - 0.2), 2) * 312000;
		}
		// update the indicator for whether we have a string of consecutive target resistance hits. 
		if ((resistance_timely > target_resistance_scaled*(1 - target_resistance_tolerance)) | (resistance_timely<0)){
			targ_res_consecutive = targ_res_consecutive + 1;
		}
		else targ_res_consecutive = 0;
	}
	return voltage_write;
}

float K2400::DoDwell(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay, float dwellV){
	//initialize local variables
	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	int targ_res_consecutive = 0;
	resistance_timely = 0;
	resistance_benchmark = 0;
	float diffres = 0;
	float voltage_write = volt_start - volt_ramp; //we omit the point V=0 by increasing the voltage before measurement!
	float voltage_read = 0;
	float current_read = 0;

	char delays[40] = "";
	sprintf(delays, ":SOUR:DEL %f", delay);
	int counter = 0; //number of measurements since the most recent ramp_down
	//execute the code in for loop until we get the required resistance +/- resistance_tol*resistance
	while (!targ_res_consecutive && voltage_write < volt_stop && !GetAsyncKeyState(VK_F12)){
		if (voltage_write < dwellV){
			voltage_write += volt_ramp;
			sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);    //create a new string of a command
			GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
		}
		//setting new voltage on Keithley
		GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files

		//compute the resistance on basis of measurement from Keithley. Also writes it to output and log
		// 140422 (sonya) temporarily try just V/I instead of differential resistance computation - see GetResistance function
		resistance_timely = GetResistance(&diffres, voltage_read, current_read, reading_log_keithley, output, counter);
		std::cout << resistance_timely << " ohms \n";
		// update the indicator for whether we have a string of consecutive target resistance hits. 
		if ((resistance_timely > target_resistance*(1 - target_resistance_tolerance)) | (resistance_timely<0)){
			targ_res_consecutive = targ_res_consecutive + 1;
		}
		else targ_res_consecutive = 0;
	}
	return voltage_write;
}

float K2400::GetResistance(float *diffres, float &voltage_read, float &current_read, FILE* reading_log_keithley, FILE* output, int counter){

	//initialize/define the local variables
	char voltage_new[14]; //here we load the string of numbers for voltage before we convert it into a number
	char current_new[14]; //here we load the string of numbers for current before we convert it into a number
	char trash[2]; //comma will be stored here after loading
	float voltage_change = 0;
	float current_change = 0;
	float voltage_new_num = 0;
	float current_new_num = 0;
	float resistance = 0;

	//at first we get the values from the log file as strings
	for (int i = 0; i<2; i++){
		if (i % 2 == 0){
			fgets(voltage_new, 14, reading_log_keithley);
			std::istringstream in(voltage_new);
			in >> voltage_new_num;
		}
		else {
			fgets(current_new, 14, reading_log_keithley);
			std::istringstream in(current_new);
			in >> current_new_num;
		}
		fgets(trash, 2, reading_log_keithley);
	}
	// if the measurement is not first in a row, not at the bottom of "hill" do dU/dI
	if (counter != 0){
		//compute the dV and dI
		voltage_change = voltage_new_num - voltage_read;
		current_change = current_new_num - current_read;
		//then compute the resistance as dU/dI and require that its positive
		resistance = voltage_change / current_change;
	}
	//if this IS the first measurement in a row ("bottom of the hill") do R=U/I
	else resistance = voltage_new_num / current_new_num;

	// after all the differential resistance calculation machinery is through, use its value for resistance to return to diffres
	*diffres = resistance;

	//and assign new values to the voltage and intensity variables
	voltage_read = voltage_new_num;
	current_read = current_new_num;

	//140422 (sonya) temporarily do V/I rather than differential resistance.
	resistance = voltage_new_num / current_new_num;

	//print the resultant resistance to output file
	fprintf(output, "%f\n", resistance);
	fflush(output);

	//the function returns the value of resistance;
	return resistance;
}

void K2400::GetReading(char* buffer, std::string &temporary, FILE* output, FILE* log_keithley){
	sprintf(buffer, "");
	//ask the device for a readings

	// wait, why is this write necessary before executing read?
	GPIBWrite(pna, ":READ?");
	// read the voltage and intensity value given
	GPIBRead(pna, buffer);
	//save to log_file and flush the stream
	fprintf(log_keithley, "%s", buffer);
	fflush(log_keithley);
	//save to the output file, changing the format of what we write, and flush the stream
	temporary.erase();
	temporary = buffer;
	temporary.erase(temporary.length() - 1, 1);
	temporary += ",";
	fprintf(output, "%s", temporary.c_str());
	fflush(output);
}

void K2400::KeepTrackIndicators(int &res_consecutive, int &targ_res_consecutive, int &neg_res_consecutive, int &bench_res_consecutive, float &resistance_benchmark,
	float &resistance_timely, float &diffres, float target_resistance, float target_resistance_tolerance, int &counter){

	//if it is the first measurement "down the hill", use it to set benchmark. Reject first n data points up to lowcount, then use next
	// m data points up to highcount to generate an average resistance as benchmark.

	int highcount = 13;
	int lowcount = 5;

	if (counter < highcount){
		if (counter < lowcount){
			resistance_benchmark = resistance_timely;
		}
		else{
			resistance_benchmark = (resistance_benchmark*(counter - lowcount + 1) + resistance_timely) / (counter - lowcount + 2);
		}
	}

	if (counter == highcount){
		char bench[100] = "";
		sprintf(bench, "avg resistance is %f", resistance_benchmark);
		std::cout << bench << std::endl;
	}

	//if (counter > highcount && (resistance_timely >= (resistance_benchmark*(1 + resistance_tolerance)))){
	// bench_res_consecutive = bench_res_consecutive + 1;
	//}
	//else bench_res_consecutive = 0;

	//// update the indicator for whether we have a string of consecutive negative resistance hits.
	//if (resistance_timely < 0){
	// neg_res_consecutive = neg_res_consecutive + 1;
	//}
	//else neg_res_consecutive = 0;


	// pick which resistance tolerance we are using depending on if we are in high or low resistance regime
	float restol;
	if (resistance_timely < res_switch){
		restol = resistance_tolerance;
	}
	else { restol = resistance_tolerance_high; }

	// increment for whether v/i metric gives us res > res benchmark
	if ((counter > highcount && (resistance_timely >= (resistance_benchmark*(1 + restol))))){
		res_consecutive = res_consecutive + 1;
		std::cout << "Hit. Res_consecutive is " << res_consecutive << "\n";
	}
	else res_consecutive = 0;

	// increment for whether dv/di metric gives us NDR - this indicator increments at any point in the ramp, even if we are still in the initial part of the ramp (counter < highcount)
	if (diffres < 0){
		neg_res_consecutive = neg_res_consecutive + 1;
	}
	else neg_res_consecutive = 0;

	// update the indicator for whether we have a string of consecutive target resistance hits. We are looking only for stuff above
	if ((resistance_timely > target_resistance*(1 - target_resistance_tolerance)) | (resistance_timely<0)){
		targ_res_consecutive = targ_res_consecutive + 1;
	}
	else targ_res_consecutive = 0;


}

bool K2400::CheckRampDown(float resistance_timely, int &res_consecutive, int &bench_res_consecutive, int &neg_res_consecutive){

	// choose which number of consecutive increased resistance or consecutive negative resistance to use depending on whether we are in high or low R regime
	float trigger;
	float neg;
	if (resistance_timely < res_switch){
		trigger = ntrigger;
		neg = nnegres;
	}
	else { 
		trigger = ntrigger_high;
		neg = nnegres_high;}

	// check the consecutive increased resistances and consecutive negative differential resistances conditions
	if ((res_consecutive >= trigger) | (neg_res_consecutive >= neg)){
		std::cout << "ramp down triggered \n";
		return true;
	}
	return false;
}

bool K2400::GotTargetResistance(float &r_time, int &targ_res_consecutive, float &volt_stop, float &volt){
	// this returns a T/F that dictates whether the loop in doMeasurement keeps going or ends (ending measurement).
	//the condition is: Backspace is NOT pressed, r_time is NOT in the range <tar_g_r(1-targ_r_tol),targ_r(1+targ+r+tol)>

	if (!GetAsyncKeyState(VK_F12) &&
		targ_res_consecutive < 1 && volt < volt_stop){
		return true;
	}
	else return false;
}

void K2400::display_parameters(){ // just display all the parameters
	char delays[20];
	sprintf(delays, ":SOUR:DEL %f", delay);

	std::cout << " ========----PARAMS----========== \n";
	std::cout << "ID of the board: " << 0 << "\n";
	std::cout << "The maximum size of SCPI command string (Hardcoded): " << ERRMSGSIZE << "\n";
	std::cout << "Board Index of GPIB: " << PRIMARY_ADDR_OF_KEITHLEY << "\n";
	std::cout << "The END message status (Enabled 1, Disabled 0): " << EOTMODE << "\n";
	std::cout << "The EOS MODE status (Enabled 1, Disabled 0): " << EOSMODE << "\n";
	std::cout << "Delay between ramping the voltage (in seconds):" << delay << " " << delays << "\n";
	std::cout << "Between Delay's we're ramping the voltage by (in volts)" << volt_ramp << "\n";
	std::cout << "The stop voltage is: " << volt_stop << "\n";
	std::cout << " ========---------------========== " << "\n";
}

void K2400::holdGateVoltage(){
	// sourcing voltage
	GPIBWrite(pna, ":SOUR:FUNC VOLT");
	// sensing mode DC current
	GPIBWrite(pna, ":SENS:FUNC 'CURR:DC'");
	//       // voltage source range in auto
	//GPIBWrite(pna,":SOUR:VOLT:RANGE:AUTO ON");
	//       // current sensing range in auto
	GPIBWrite(pna,":SENS:CURR:RANGE:AUTO ON");
	// Auto ranging seemed to cause exploding of devices.....???

	GPIBWrite(pna, ":SOUR:VOLT:RANGE 21");

	// current protection (compliance) set at 1A

	GPIBWrite(pna, ":SENS:CURR:PROT 1.0");
	// fixed voltage mode
	GPIBWrite(pna, ":SOUR:VOLT:MODE FIXED");
	// terminals connected on the front
	GPIBWrite(pna, ":ROUT:TERM FRONT");
	// 2w measurement
	GPIBWrite(pna, ":SYST:RSEN OFF");
	// setting output format to V I
	GPIBWrite(pna, ":FORM:ELEM VOLT,CURR");
	GPIBWrite(pna, ":SENS:CURR:NPLC 1");
	GPIBWrite(pna, ":SOUR:DEL 0.01");

	float voltage;
	std::cout << "What voltage would you like to output on Keithley? (in volts) \n";
	std::cin >> voltage;
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	sprintf(buffer, ":SOUR:VOLT %f\n", voltage);    //create a new string of a command for sourcing the voltage
	GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
	GPIBWrite(pna, ":OUTP ON");

	// hold the voltage until the user indicates to stop
	std::cout << "=======================================================================\n";
	std::cout << "Press 'F12' to stop sourcing the gate voltage... \n";
	std::cout << "=======================================================================\n";
	while (!GetAsyncKeyState(VK_F12)){
		Sleep(500);
		GPIBWrite(pna, ":READ?");
		GPIBRead(pna, buffer);
	}

	// go back to 0 voltage on gateline
	GPIBWrite(pna, ":SOUR:VOLT 0");
	GPIBWrite(pna, ":OUTP OFF");
}

int K2400::GPIBWrite(int ud, char* cmd){
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

int K2400::GPIBRead(int ud, char *message){
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

void K2400::SendMail(std::string emailAddress){
	Spawn(0, emailAddress);
	system("Ruby  mail.rb");
}
//
//void K2400::ShowGraph(){
//	Spawn(1);
//	system("gnuscript.gp");
//}
//
void K2400::Spawn(int pick_program, std::string emailAddress){
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
		program += "From: Your Name <#{FROM_EMAIL}> \nTo: my phone <#{TO_EMAIL}> \nSubject: finished last device at <#{time1.inspect}> \n";
		program += "Date: Sat, 23 Jun 2001 16:26:43 +0900 \nMessage-Id: <unique.message.id.string@example.com> \n";
		program += "All devices on the chip have been broken and monitored. Come check me! \n";
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


