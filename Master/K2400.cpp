#include "K2400.h"
#include <cmath>
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
	//printf("initializing K2400 sourcemeter parameters\n");
	// sourcing voltage
	GPIBWrite(pna, "*RST");
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

	nplc = 1;

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



// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- KEITHLEY RANGE HANDLING -----------------------------------
// ---------------------------------------------------------------------------------------------------------

float K2400::determine_voltage_range(float src_volt){
	float range;
	if (abs(src_volt) < 0.2){
		range = 0.2;
	}
	else if (abs(src_volt) < 2.0){
		range = 2.0;
	}
	else{
		range = 21.0;
	}
	return range;
}

int K2400::set_voltage_range(float range_volt){
	std::cout << "Changing Voltage Range from " << present_volt_range << " to " << range_volt << ".\n";
	char setrange[30];
	sprintf(setrange, ":SOUR:VOLT:RANG %f", range_volt);
	GPIBWrite(pna, setrange);
	present_volt_range = range_volt;  // Keep track of what range we are in
	return 0;
}

float K2400::determine_current_range(float meas_curr){
	float range;
	if (abs(meas_curr) < 0.0001){
		range = 0.0001;
	}
	else if (abs(meas_curr) < 0.001){
		range = 0.001;
	}
	else if (abs(meas_curr) < 0.01){
		range = 0.01;
	}
	else{
		range = 0.1;
	}
	return range;
}

int K2400::set_current_range(float range_curr){
	if (range_curr != present_curr_range){
		std::cout << "Changing Current Range from " << present_curr_range << " to " << range_curr << ".\n";
		char setrange[30];
		sprintf(setrange, ":SENS:CURR:RANG %f", range_curr);
		GPIBWrite(pna, setrange);
		present_curr_range = range_curr;  // Keep track of what range we are in
	}
	return 0;
}

int K2400::switch_voltage_range(Switchbox switchbox, int devnum, float range_volt){
	switchbox.openChan(devnum, "keithley", 1);
	Sleep(300);
	set_voltage_range(range_volt);
	Sleep(300);
	switchbox.closeChan(devnum, "keithley");
	Sleep(30);
	return 1;
}

int K2400::switch_current_range(Switchbox switchbox, int devnum, float range_curr){
	switchbox.openChan(devnum, "keithley", 1);
	Sleep(300);
	set_current_range(range_curr);
	Sleep(300);
	switchbox.closeChan(devnum, "keithley");
	Sleep(30);
	return 1;
}



// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- VOLTAGE PULSES -----------------------------------
// ---------------------------------------------------------------------------------------------------------

float K2400::setParamsPulse(){
	// auto-zero and auto-range off b/c they increase pulse time
	GPIBWrite(pna, ":SYS:AZER:STAT OFF");
	GPIBWrite(pna, ":SOUR:VOLT:RANGE 21");
	GPIBWrite(pna, ":SENS:FUNC:CONC OFF");
	GPIBWrite(pna, ":SOUR:VOLT:RANGE:AUTO OFF");
	GPIBWrite(pna, ":SENS:CURR:RANGE:AUTO OFF");
	GPIBWrite(pna, ":SENS:FUNC:OFF ALL");
	// custom voltage sweep mode and set the number of points in the sweep 
	GPIBWrite(pna, ":SOUR:VOLT:MODE LIST");
	GPIBWrite(pna, ":TRIG:COUN 2");
	// adjust trigger & source delay to achieve minimum pulse time
	GPIBWrite(pna, ":TRIG:DEL 0.0");
	GPIBWrite(pna, ":SOUR:DEL 0.0");
	GPIBWrite(pna, ":OUTP ON");
	return 0;
}

float K2400::pulseSingle(float voltage){
	// set voltage values for the pulse
	char volts[30];
	sprintf(volts, ":SOUR:LIST:VOLT %f,0", voltage);
	GPIBWrite(pna, volts);
	// trigger a single sweep
	GPIBWrite(pna, ":INIT");
	return 0;
}



// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- VOLTAGE SWEEPS -----------------------------------
// ---------------------------------------------------------------------------------------------------------

void K2400::setParamsSweep(int* useEMV){
	int choice;
	std::cout << ">>>>>>>>>>> Use Default Parameters for Keithley Sweep? ('0' for No, '1' for Yes)\n";
	std::cin >> choice;

	if (choice){
		volt_ramp_KS = 0.0001;
		volt_stop_KS = 2;
		volt_start_KS = 0.7;
		target_resistance_KS = 325000;
		*useEMV = 1;
	}
	else{
		std::cout << "By how much are we ramping the voltage? (in volts)\n";
		std::cin >> volt_ramp_KS;
		std::cout << "What is the stop voltage (max allowed voltage)? \n";
		std::cin >> volt_stop_KS;
		std::cout << "At what voltage do we want to start? \n";
		std::cin >> volt_start_KS;
		std::cout << "What is the target resistance at 100 mV? \n";
		std::cin >> target_resistance_KS;
		std::cout << "Would you like to use the exit voltages for EM as the starting KS voltages? ('0' for NO, '1' for YES) \n";
		std::cin >> *useEMV;
	}
}

void K2400::initializeSweep(){	
	
	char integration[30];
	sprintf(integration, ":SENS:CURR:NPLC %i", nplc);
	GPIBWrite(pna, integration);

	GPIBWrite(pna, ":SOUR:VOLT:RANG 1");  // voltage source range to 2V so we can step at 50 uV
	GPIBWrite(pna, ":SENS:CURR:RANG 1E-3");  
	if (GPIBWrite(pna, ":OUTP ON")){                   // turn the output ON - if fail, proceed to sending an info on screen
		printf("GPIB error while turning on sourcemeter\n");
	}
	GPIBWrite(pna, ":SOUR:VOLT 0.0");    // get the bias to 0  
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
	if (Vstart != -1.0){
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

float K2400::DoSweep(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay){
	//initialize local variables
	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	int targ_res_consecutive = 0;
	resistance_timely = 0;
	float target_resistance_scaled = 0;

	// parameters for the resistance target model
	float R_max = 150000;
	float beta = 1;

	float storevoltage = 0;
	float resistance_lowbias = 0;

	float diffres = 0;
	float old_resistance = 0;
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
		resistance_timely = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter);
		std::cout << resistance_timely << " Ohms @ " << voltage_write << " V - ";

		if (voltage_write < 0.1){
			target_resistance_scaled = target_resistance_KS;
		}
		else if (voltage_write >= 0.1 && voltage_write <= 0.2){
			target_resistance_scaled = target_resistance_KS - (voltage_write - 0.1) * 2500000 * beta;
		}
		else if (voltage_write > 0.2 && voltage_write <= 0.7){
			float Rlowend = target_resistance_KS - (0.1) * 2500000 * beta;
			float Rhighend = target_resistance_KS - (target_resistance_KS - 100000)*beta;
			target_resistance_scaled = Rlowend - (voltage_write - 0.2)*(Rlowend - Rhighend) / 0.5;
			//target_resistance_scaled = 400000 - pow((voltage_write - 0.2), 2) * alpha;
		}
		else{

			target_resistance_scaled = target_resistance_KS - (target_resistance_KS - 100000)* beta;
			//target_resistance_scaled = R_max + (880000-alpha)/8;
		}

		std::cout << " beta is " << beta << " and R_target is " << target_resistance_scaled << " ohms\n";

		// update the indicator for whether we have a string of consecutive target resistance hits. 
		if ((resistance_timely > target_resistance_scaled) | (resistance_timely < 0)){

			// sample at 100mV to check actual resistance, do it 3 times to see how reliable keithley is
			storevoltage = voltage_write;

			GPIBWrite(pna, ":SOUR:VOLT 0.1\n");     //and send it to the source (Keithley)

			GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files
			resistance_lowbias = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter);
			GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files
			resistance_lowbias = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter);
			GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files
			resistance_lowbias = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter);

			// based on 200mV sample decide whether it is finished, or whether to move up to more aggressive algorithm
			if ((resistance_lowbias > target_resistance_KS) || (beta == 0)){
				targ_res_consecutive = targ_res_consecutive + 1;
				voltage_write = storevoltage;
			}
			else{
				beta -= 0.02;
				std::cout << "\n\n Switching to more aggressive algorithm with alpha = " << beta << "\n\n\n";
				voltage_write = storevoltage;
			}

		}
		else targ_res_consecutive = 0;
	}
	return voltage_write;
}



// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- DWELLING AT VOLTAGE -----------------------------------
// ---------------------------------------------------------------------------------------------------------

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

float K2400::DoDwell(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay, float dwellV){
	//initialize local variables
	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	int targ_res_consecutive = 0;
	resistance_timely = 0;
	resistance_benchmark = 0;
	float diffres = 0;
	float old_resistance = 0;
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
		resistance_timely = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter);
		std::cout << resistance_timely << " ohms \n";
		// update the indicator for whether we have a string of consecutive target resistance hits. 
		if ((resistance_timely > target_resistance*(1 - target_resistance_tolerance)) | (resistance_timely<0)){
			targ_res_consecutive = targ_res_consecutive + 1;
		}
		else targ_res_consecutive = 0;
	}
	return voltage_write;
}



// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- HEALING GAPS -----------------------------------
// ---------------------------------------------------------------------------------------------------------

float K2400::healSingle(int devnum, FILE* outputs[36]){
	// want to keep the temp data in the file with other params
	FILE* log_keithley = fopen("log.txt", "w+");
	FILE* reading_log_keithley = fopen("log.txt", "r");

	FILE* output = outputs[devnum];
	fprintf(output, "DELAY (SEC) %.3f ; RAMP (V) %.4f ; MAX VOLT (V) %.2f ; RAMP RESISTANCE TOLERANCE (low R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (high R) (float %%) %.3f ; TARGET RESISTANCE (Ohms) %.9f ; TARGET RESISTANCE TOLERANCE (float %%) %.3f ; RAMP DOWN (V) %.3f ; NUM CONSEC % HITS (low R) %i ; NUM CONSEC % HITS (high R) %i ; NUM CONSEC NDR (low R) %i ; NUM CONSEC NDR (high R) %i ; HIGH/LOW SWITCH RESISTANCE (ohms) %.9f ; INTEGRATION TIME (PLC) %i ; RAMPDOWN DWELL (msec/mV) %.4i ; TEMPERATURE (K) %.4f \n ", delay, volt_ramp, volt_stop, resistance_tolerance, resistance_tolerance_high, target_resistance, target_resistance_tolerance, volt_down, ntrigger, ntrigger_high, nnegres, nnegres_high, res_switch, nplc, rampdown_dwell, temperature);
	fprintf(output, "voltage,current,resistance \n");
	fflush(output);

	float exitV = 0;

	// current sensing range in auto
	GPIBWrite(pna, ":SENS:CURR:RANGE:AUTO ON");

	std::cout << "=======================================================================\n";
	std::cout << "Press the 'F12' key to interrupt HEALING...\n";
	std::cout << "=======================================================================\n";

	exitV = DoHeal(log_keithley, reading_log_keithley, output, delay); //Here the real measurement is done. See comments within the function

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

float K2400::DoHeal(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay){
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
	float old_resistance = 0;
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
		resistance_timely = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter);

		//this function keeps tracks of the count of consecutive weird measurements and takes care of setting resistance_benchmark
		KeepTrackHealIndicators(res_consecutive, targ_res_consecutive, neg_res_consecutive, bench_res_consecutive, resistance_benchmark,
			resistance_timely, diffres, target_resistance, target_resistance_tolerance, counter);

		counter++;  //increase the counter to keep track of whether we are at the beginning of ramp cycle or not

		//check if we need to ramp down the voltage. If so, do it and zero the counter. Check if we
		//decrease the voltage below 0. if so, break the measurement. display message.
		if (CheckRampDown(resistance_timely, 0, res_consecutive, bench_res_consecutive, neg_res_consecutive)){

			// 140430 (SDS) instantaneously ramp down to 50% of present voltage
			voltage_write = voltage_write;
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



// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- ELECTROMIGRATION -----------------------------------
// ---------------------------------------------------------------------------------------------------------

float K2400::setParamsEM(int emType){

	if (emType == 0 | emType == 1 | emType == 3){
		int choice;
		std::cout << ">>>>>>>>>>> Use Default Parameters for EM? ('0' for No, '1' for Yes)\n";
		std::cin >> choice;

		if (choice){
			delay = 0;
			volt_ramp = 0.001;
			volt_stop = 2;
			volt_start = 0.21;
			target_resistance = 200;
			//std::cout << "What is the target resistance? \n";
			//std::cin >> target_resistance;
			target_resistance_tolerance = 0.001;
			nplc = 1;
			rampdown_dwell = 1;
			//res_switch = 350;
			//resistance_tolerance = 0.02;
			//nnegres = 2;
			//ntrigger = 2;

			float resistance_tolerance_defaults[4] = { 0.02, 0.018, 0.016, 0.014 };
			float res_switch_defaults[4] = { 50, 100, 150, 200 };
			float ntrigger_defaults[4] = { 3, 3, 3, 3 };
			float nnegres_defaults[4] = { 4, 4, 4, 4 };

			//float resistance_tolerance_defaults[4] = {1000000, 0.02, 0.06, 10000000.00 };
			//float res_switch_defaults[4] = { 10000000000, 350, 400, 1000000 };
			//float ntrigger_defaults[4] = { 100000000, 3, 6, 100000 };
			//float nnegres_defaults[4] = { 100000000, 3, 6, 10000000 };


			for (int j = 0; j < 4; j++){
				res_switch[j] = res_switch_defaults[j];
				resistance_tolerance[j] = resistance_tolerance_defaults[j];
				ntrigger[j] = ntrigger_defaults[j];
				nnegres[j] = nnegres_defaults[j];
			}

			std::cout << "What is the average series R for the algo to run off of? (ohms)? \n";
			std::cin >> avg_rs;

			//////////////////////////////////////////
			// TEMPORARY! LET US OVERRIDE THE DEFAULTS
			//std::cout << "At what voltage do we want to start? \n";
			//std::cin >> volt_start;
			//std::cout << "What is the % resistance change? Give a percentage in the float format (ex. 20% - 0.2)\n";
			//std::cin >> resistance_tolerance[0];
			//////std::cout << "What is the stop voltage? \n";
			//////std::cin >> volt_stop;
			//std::cout << "What is the number of consecutive % resistance changes?\n";
			//std::cin >> ntrigger[0];
			//std::cout << "What is number of consecutive NDRs?\n";
			//std::cin >> nnegres[0];


			//std::cout << "SECOND REGIME: What is the % resistance change? Give a percentage in the float format (ex. 20% - 0.2)\n";
			//std::cin >> resistance_tolerance[1];
			////std::cout << "What is the stop voltage? \n";
			////std::cin >> volt_stop;
			//std::cout << "SECOND REGIME: What is the number of consecutive % resistance changes?\n";
			//std::cin >> ntrigger[1];
			//std::cout << "SECOND REGIME: What is number of consecutive NDRs?\n";
			//std::cin >> nnegres[1];

			//std::cout << "THIRD REGIME: What is the % resistance change? Give a percentage in the float format (ex. 20% - 0.2)\n";
			//std::cin >> resistance_tolerance[2];
			////std::cout << "What is the stop voltage? \n";
			////std::cin >> volt_stop;
			//std::cout << "THIRD REGIME: What is the number of consecutive % resistance changes?\n";
			//std::cin >> ntrigger[2];
			//std::cout << "THIRD REGIME: What is number of consecutive NDRs?\n";
			//std::cin >> nnegres[2];


			///////////////////////////////////////////////////
		}
		else{
			// delay is always set as 0 so hardcode it for now as of 150724
			//std::cout << "Delay between ramping the voltage? (in seconds)\n";
			//std::cin >> delay;
			delay = 0;
			//std::cout << "By how much are we ramping the voltage? (in volts)\n";
			//std::cin >> volt_ramp;
			volt_ramp = 0.001;

			//std::cout << "What is the stop voltage? \n";
			//std::cin >> volt_stop;
			volt_stop = 6;

			std::cout << "What is the average series R for the algo to run off of? (ohms)? \n";
			std::cin >> avg_rs;
			std::cout << "At what voltage do we want to start? \n";
			std::cin >> volt_start;
			std::cout << "What is the target resistance? \n";
			std::cin >> target_resistance;

			//std::cout << "What is the tolerance for target resistance? Give a percentage in the float format (ex. 20% - 0.2)\n";
			//std::cin >> target_resistance_tolerance;
			target_resistance_tolerance = 0.001;

			nplc = 1;
			// volt_down is not being used as of 150724, hard code some reasonable value to pass
			//std::cout << "How many msec to dwell on each point in the rampdown (rampdown rate will be 1mV/dwell) \n";
			//std::cin >> rampdown_dwell;
			rampdown_dwell = 1;

			std::string RegimeNames[4] = { "LOW", "LOWER MIDDLE", "UPPER MIDDLE", "HIGH" };
			for (int j = 0; j < 4; j++){
				std::cout << "\n\n*****Set parameters for the " << RegimeNames[j] << " Regime*****\n";
				if (j < 3){
					std::cout << "At what R_jct will we leave this regime (in ohms)?\n";
					std::cin >> res_switch[j];
				}
				std::cout << "What is the % resistance change? Give a percentage in the float format (ex. 20% - 0.2)\n";
				std::cin >> resistance_tolerance[j];
				std::cout << "What is the number of consecutive % resistance changes?\n";
				std::cin >> ntrigger[j];
				std::cout << "What is number of consecutive NDRs?\n";
				std::cin >> nnegres[j];
				std::cout << "\n";
			}

			// Set the voltage range
			float volt_range;
			std::cout << "What voltage range to use (2 or 21)?\n";
			std::cin >> volt_range;
			char setrange[30];
			sprintf(setrange, ":SOUR:VOLT:RANGE %i", volt_range);
			GPIBWrite(pna, setrange);

		}



		// adding this in in the hopes that it fixes the bizarre problem of the EM algorithm writing four columns of data...
		configure();

		std::cout << "Please check TempB on Lakeshore and input in Kelvin \n";
		std::cin >> temperature;

		volt_down = 0.2;

		char integration[30];
		sprintf(integration, ":SENS:CURR:NPLC %i", nplc);
		GPIBWrite(pna, integration);

		GPIBWrite(pna, ":SOUR:VOLT 0.0");                    // get the bias to 0

		if (GPIBWrite(pna, ":OUTP ON")){                   // turn the output ON - if fail, proceed to sending an info on screen
			printf("GPIB error while turning on sourcemeter\n");
		}
		// else printf("Keithley output on! Starting measurement! \n");
		Sleep(500);
	}
	if (emType == 2){
		std::cout << "By how much are we ramping the voltage? (in volts)\n";
		std::cin >> volt_ramp;
		std::cout << "What is the stop voltage? \n";
		std::cin >> volt_stop;
		std::cout << "At what voltage do we want to start? \n";
		std::cin >> volt_start;
		std::cout << "What is the target resistance? \n";
		std::cin >> target_resistance;
		std::cout << "(NOTE: exit condition is target R _or_ 2000 data points at the dwell voltage.)\n";
		std::cout << "What is the tolerance for target resistance? Give a percentage in the float format (ex. 20% - 0.2)\n";
		std::cin >> target_resistance_tolerance;

		// decide what will be the triggers for indicating EM onset and indicating rapid EM needing a ramp back
		std::cout << "What is the percentage resistance difference that will prevent voltage incrementing (float format)?\n";
		std::cin >> rampup_thresh;
		std::cout << "What is the percentage resistance difference that will trigger dwelling voltage (float format)?\n";
		std::cin >> rampback_thresh;

		std::cout << "Please check TempB on Lakeshore and input in Kelvin \n";
		std::cin >> temperature;

		// Set the voltage range
		float volt_range;
		std::cout << "What voltage range to use (2 or 21)?\n";
		std::cin >> volt_range;
		char setrange[30];
		sprintf(setrange, ":SOUR:VOLT:RANGE %i", volt_range);
		GPIBWrite(pna, setrange);

	}

	if (emType == 4){
		int choice;
		std::cout << ">>>>>>>>>>> Use Default Parameters for EM? ('0' for No, '1' for Yes)\n";
		std::cin >> choice;

		if (choice){
			delay = 0;
			volt_ramp = 0.001;
			volt_stop = 3;
			volt_start = 0.05;
			target_resistance = 400;
			target_resistance_tolerance = 0.001;
			nplc = 1;
			rampdown_dwell = 1;

			float resistance_tolerance_defaults[4] = { 0.02, 0.02, 0.02, 4 };
			float res_switch_defaults[4] = { 50, 100, 150, 200 };
			float ntrigger_defaults[4] = { 3, 3, 3, 3 };
			float nnegres_defaults[4] = { 3, 3, 3, 4 };
			for (int j = 0; j < 4; j++){
				res_switch[j] = res_switch_defaults[j];
				resistance_tolerance[j] = resistance_tolerance_defaults[j];
				ntrigger[j] = ntrigger_defaults[j];
				nnegres[j] = nnegres_defaults[j];
			}


			//std::cout << "What is the delta(R) to trigger ramp back counter (ohms)? \n";
			//std::cin >> delta;
			std::cout << "What is the number of consecutive delta(R) to trigger ramp back? \n";
			std::cin >> delta_counts;
			std::cout << "What is the number of consecutive NDR to trigger ramp back? \n";
			std::cin >> ndr_counts;



			////////////////////////////////////////////
			//// TEMPORARY! LET US OVERRIDE THE DEFAULTS
			////std::cout << "At what voltage do we want to start? \n";
			////std::cin >> volt_start;
			//std::cout << "What is the % resistance change? Give a percentage in the float format (ex. 20% - 0.2)\n";
			//std::cin >> resistance_tolerance[0];
			//////std::cout << "What is the stop voltage? \n";
			//////std::cin >> volt_stop;
			//std::cout << "What is the number of consecutive % resistance changes?\n";
			//std::cin >> ntrigger[0];
			//std::cout << "What is number of consecutive NDRs?\n";
			//std::cin >> nnegres[0];
			/////////////////////////////////////////////////////
		}
		else{
			delay = 0;
			nplc = 1;
			volt_down = 0.2;
			target_resistance_tolerance = 0.001;
			volt_stop = 6;
			rampdown_dwell = 1;

			//std::cout << "By how much are we ramping the voltage? (in volts)\n";
			//std::cin >> volt_ramp;
			volt_ramp = 0.001;

			//std::cout << "What is the delta(R) to trigger ramp back counter (ohms)? \n";
			//std::cin >> delta;

			std::cout << "At what voltage do we want to start? \n";
			std::cin >> volt_start;
			std::cout << "What is the target resistance? \n";
			std::cin >> target_resistance;
			std::cout << "What is the average series R for the algo to run off of? (ohms)? \n";
			std::cin >> avg_rs;


			//std::string RegimeNames[4] = { "LOW", "LOWER MIDDLE", "UPPER MIDDLE", "HIGH" };
			//for (int j = 0; j < 4; j++){
			//	std::cout << "\n\n*****Set parameters for the " << RegimeNames[j] << " Regime*****\n";
			//	if (j < 3){
			//		std::cout << "At what R_jct will we leave this regime (in ohms)?\n";
			//		std::cin >> res_switch[j];
			//	}
			//	std::cout << "What is the delta(R_jct) to ramp back on?\n";
			//	std::cin >> resistance_tolerance[j];
			//	std::cout << "What is the number of consecutive delta(R_jct's) to ramp back on?\n";
			//	std::cin >> ntrigger[j];
			//	std::cout << "What is number of consecutive NDRs to ramp back on?\n";
			//	std::cin >> nnegres[j];
			//	std::cout << "\n";
			//}

		}


		// adding this in in the hopes that it fixes the bizarre problem of the EM algorithm writing four columns of data...
		configure();

		std::cout << "Please check TempB on Lakeshore and input in Kelvin \n";
		std::cin >> temperature;

		char delays[40] = "";
		sprintf(delays, ":SOUR:DEL %f", delay);
		GPIBWrite(pna, delays);

		char integration[30];
		sprintf(integration, ":SENS:CURR:NPLC %i", nplc);
		GPIBWrite(pna, integration);

		GPIBWrite(pna, ":SOUR:VOLT 0.0");                    // get the bias to 0

		if (GPIBWrite(pna, ":OUTP ON")){                   // turn the output ON - if fail, proceed to sending an info on screen
			printf("GPIB error while turning on sourcemeter\n");
		}



		// Based on volt_start, initialize present_voltage_range and actually set the corresponding range on keithley
		present_volt_range = determine_voltage_range(volt_start);
		char setrange[30];
		sprintf(setrange, ":SOUR:VOLT:RANG %f", present_volt_range);
		GPIBWrite(pna, setrange);
		std::cout << "Set the initial voltage range to " << present_volt_range << ".\n";
		// initialize present_current_range and actually set the corresponding range on keithley
		present_curr_range = 0.001;
		sprintf(setrange, ":SENS:CURR:RANG %f", present_curr_range);
		GPIBWrite(pna, setrange);



		// else printf("Keithley output on! Starting measurement! \n");
		Sleep(500);
	}

	// Based on volt_start, initialize present_voltage_range and actually set the corresponding range on keithley
	present_volt_range = determine_voltage_range(volt_start);
	char setrange[30];
	sprintf(setrange, ":SOUR:VOLT:RANG %f", present_volt_range);
	GPIBWrite(pna, setrange);
	std::cout << "Set the initial voltage range to " << present_volt_range << ".\n";
	// initialize present_current_range and actually set the corresponding range on keithley
	present_curr_range = 0.0001;
	sprintf(setrange, ":SENS:CURR:RANG %f", present_curr_range);
	GPIBWrite(pna, setrange);

	return temperature;
}

void K2400::initializeEM(int emType){
	//if (emType == 0){ //normal EM
	//	GPIBWrite(pna, ":SOUR:VOLT:RANGE 21"); // voltage sensing range - 2V is the next lowest range but some devices do require 2+V
	//	GPIBWrite(pna, ":SENS:CURR:RANGE 7E-3");
	//}
	//if (emType == 1){ //stabilize EM

	//	GPIBWrite(pna, ":SOUR:VOLT:RANGE 2");
	//	GPIBWrite(pna, ":SENS:CURR:RANGE 7e-3");
	//}
	//if (emType == 2){ //curvature EM
	//	GPIBWrite(pna, ":SOUR:VOLT:RANGE 2");
	//	GPIBWrite(pna, ":SENS:CURR:RANGE 7E-3");
	//}
	//if (emType == 3){ //alternating polarity EM
	//	GPIBWrite(pna, ":SOUR:VOLT:RANGE 21"); // voltage sensing range - 2V is the next lowest range but some devices do require 2+V
	//	GPIBWrite(pna, ":SENS:CURR:RANGE 7E-3");


	GPIBWrite(pna, ":SOUR:VOLT 0.0");                    // get the bias to 0

	if (GPIBWrite(pna, ":OUTP ON")){                   // turn the output ON - if fail, proceed to sending an info on screen
		printf("GPIB error while turning on sourcemeter\n");
	}
	// else printf("Keithley output on! Starting measurement! \n");
	Sleep(500);
}



float K2400::emSingle(int devnum, FILE* outputs[36], int emType, float Vstab, Switchbox switchbox){
	// want to keep the temp data in the file with other params
	FILE* log_keithley = fopen("log.txt", "w+");
	FILE* reading_log_keithley = fopen("log.txt", "r");
	FILE* output = outputs[devnum];

	float exitV = 0;

	std::cout << "=======================================================================\n";
	std::cout << "Press the 'F12' key to interrupt active EM...\n";
	std::cout << "=======================================================================\n";

	if (emType == 0){ // normal EM
		//write header
		fprintf(output, "DELAY (SEC) %.3f ; RAMP (V) %.4f ; MAX VOLT (V) %.2f ; RAMP RESISTANCE TOLERANCE (low R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (low-med R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (med-high R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (high R) (float %%) %.3f ; TARGET RESISTANCE (Ohms) %.9f ; TARGET RESISTANCE TOLERANCE (float %%) %.3f ; RAMP DOWN (V) %.3f ; NUM CONSEC % HITS (low R) %i ; NUM CONSEC % HITS (low-med R) %i ;  NUM CONSEC % HITS (med-high R) %i ; NUM CONSEC % HITS (high R) %i ; NUM CONSEC NDR (low R) %i ; NUM CONSEC % HITS (low-med R) %i ; NUM CONSEC % HITS (med-high R) %i ; NUM CONSEC NDR (high R) %i ; LOW/LOW-MED SWITCH RESISTANCE (ohms) %.9f ; LOW-MED/MED-HIGH SWITCH RESISTANCE (ohms) %.9f ; MED-HIGH/HIGH SWITCH RESISTANCE (ohms) %.9f ;  INTEGRATION TIME (PLC) %i ; RAMPDOWN DWELL (msec/mV) %.4i ; TEMPERATURE (K) %.4f \n ", delay, volt_ramp, volt_stop, resistance_tolerance[0], resistance_tolerance[1], resistance_tolerance[2], resistance_tolerance[3], target_resistance, target_resistance_tolerance, volt_down, ntrigger[0], ntrigger[1], ntrigger[2], ntrigger[3], nnegres[0], nnegres[1], nnegres[2], nnegres[3], res_switch[0], res_switch[1], res_switch[2], nplc, rampdown_dwell, temperature);
		fprintf(output, "voltage,current,resistance \n");
		fflush(output);
		//do measurement
		exitV = DoMeasurement(log_keithley, reading_log_keithley, output, delay, switchbox, devnum); //Here the real measurement is done. See comments within the function
	}
	if (emType == 1){ // EM followed by stabilization monitoring
		//write header
		fprintf(output, "DELAY (SEC) %.3f ; RAMP (V) %.4f ; MAX VOLT (V) %.2f ; RAMP RESISTANCE TOLERANCE (low R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (low-med R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (med-high R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (high R) (float %%) %.3f ; TARGET RESISTANCE (Ohms) %.9f ; TARGET RESISTANCE TOLERANCE (float %%) %.3f ; RAMP DOWN (V) %.3f ; NUM CONSEC % HITS (low R) %i ; NUM CONSEC % HITS (low-med R) %i ;  NUM CONSEC % HITS (med-high R) %i ; NUM CONSEC % HITS (high R) %i ; NUM CONSEC NDR (low R) %i ; NUM CONSEC % HITS (low-med R) %i ; NUM CONSEC % HITS (med-high R) %i ; NUM CONSEC NDR (high R) %i ; LOW/LOW-MED SWITCH RESISTANCE (ohms) %.9f ; LOW-MED/MED-HIGH SWITCH RESISTANCE (ohms) %.9f ; MED-HIGH/HIGH SWITCH RESISTANCE (ohms) %.9f ;  INTEGRATION TIME (PLC) %i ; RAMPDOWN DWELL (msec/mV) %.4i ; TEMPERATURE (K) %.4f \n ", delay, volt_ramp, volt_stop, resistance_tolerance[0], resistance_tolerance[1], resistance_tolerance[2], resistance_tolerance[3], target_resistance, target_resistance_tolerance, volt_down, ntrigger[0], ntrigger[1], ntrigger[2], ntrigger[3], nnegres[0], nnegres[1], nnegres[2], nnegres[3], res_switch[0], res_switch[1], res_switch[2], nplc, rampdown_dwell, temperature);
		fprintf(output, "voltage,current,resistance \n");
		fflush(output);
		float stabstep;
		std::cout << "By how much should we increment the stabilizing voltage (in Volts)?\n (SAY '0' FOR PURE STABILIZATION MEASUREMENTS)\n";
		std::cin >> stabstep;
		//do measurement
		exitV = DoStabilizeMeasurement(log_keithley, reading_log_keithley, output, delay, Vstab, stabstep); //Here the real measurement is done. See comments within the function
	}
	if (emType == 2){ // EM designed to pull out dR/dt exactly at EM onset
		//write header
		fprintf(output, "TARGET RESISTANCE (Ohms) %.9f ; VOLTAGE STEP (V) %.3f ; %% CHANGE THAT PREVENTS STEP (float %%) %.3f ; %% CHANGE THAT TRIGGERS DWELL (float %%) %.3f ; TEMPERATURE (K) %.4f\n", target_resistance, volt_ramp, rampup_thresh, rampback_thresh, temperature);
		fprintf(output, "time(ms),voltage(V),current(A),resistance(Ohms) \n");
		fflush(output);
		//do measurement
		exitV = DoCurvatureMeasurement(log_keithley, reading_log_keithley, output, delay); //Here the real measurement is done. See comments within the function
	}

	if (emType == 3){ //EM that alternates polarity on each ramp cycle
		fprintf(output, "DELAY (SEC) %.3f ; RAMP (V) %.4f ; MAX VOLT (V) %.2f ; RAMP RESISTANCE TOLERANCE (low R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (low-med R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (med-high R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (high R) (float %%) %.3f ; TARGET RESISTANCE (Ohms) %.9f ; TARGET RESISTANCE TOLERANCE (float %%) %.3f ; RAMP DOWN (V) %.3f ; NUM CONSEC % HITS (low R) %i ; NUM CONSEC % HITS (low-med R) %i ;  NUM CONSEC % HITS (med-high R) %i ; NUM CONSEC % HITS (high R) %i ; NUM CONSEC NDR (low R) %i ; NUM CONSEC % HITS (low-med R) %i ; NUM CONSEC % HITS (med-high R) %i ; NUM CONSEC NDR (high R) %i ; LOW/LOW-MED SWITCH RESISTANCE (ohms) %.9f ; LOW-MED/MED-HIGH SWITCH RESISTANCE (ohms) %.9f ; MED-HIGH/HIGH SWITCH RESISTANCE (ohms) %.9f ;  INTEGRATION TIME (PLC) %i ; RAMPDOWN DWELL (msec/mV) %.4i ; TEMPERATURE (K) %.4f \n ", delay, volt_ramp, volt_stop, resistance_tolerance[0], resistance_tolerance[1], resistance_tolerance[2], resistance_tolerance[3], target_resistance, target_resistance_tolerance, volt_down, ntrigger[0], ntrigger[1], ntrigger[2], ntrigger[3], nnegres[0], nnegres[1], nnegres[2], nnegres[3], res_switch[0], res_switch[1], res_switch[2], nplc, rampdown_dwell, temperature);
		fprintf(output, "voltage,current,resistance \n");
		fflush(output);
		//do measurement
		exitV = DoAlternateMeasurement(log_keithley, reading_log_keithley, output, delay); //Here the real measurement is done. See comments within the function
	}


	GPIBWrite(pna, ":SOUR:VOLT 0.0"); // set output back to 0 V in preparation for closing channel to next device to be EM'd
	//GPIBWrite(pna, ":OUTP OFF");
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

float K2400::RjemSingle(int devnum, FILE* outputs[36], Switchbox switchbox){

	// create log file streams 
	FILE* log_keithley = fopen("log.txt", "w+");
	FILE* reading_log_keithley = fopen("log.txt", "r");
	
	// pull correct output file for this devnum and write header
	FILE* output = outputs[devnum];
	fprintf(output, "DELAY (SEC) %.3f ; RAMP (V) %.4f ; MAX VOLT (V) %.2f ; RAMP RESISTANCE TOLERANCE (low R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (low-med R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (med-high R) (float %%) %.3f ; RAMP RESISTANCE TOLERANCE (high R) (float %%) %.3f ; TARGET RESISTANCE (Ohms) %.9f ; TARGET RESISTANCE TOLERANCE (float %%) %.3f ; RAMP DOWN (V) %.3f ; NUM CONSEC % HITS (low R) %i ; NUM CONSEC % HITS (low-med R) %i ;  NUM CONSEC % HITS (med-high R) %i ; NUM CONSEC % HITS (high R) %i ; NUM CONSEC NDR (low R) %i ; NUM CONSEC % HITS (low-med R) %i ; NUM CONSEC % HITS (med-high R) %i ; NUM CONSEC NDR (high R) %i ; LOW/LOW-MED SWITCH RESISTANCE (ohms) %.9f ; LOW-MED/MED-HIGH SWITCH RESISTANCE (ohms) %.9f ; MED-HIGH/HIGH SWITCH RESISTANCE (ohms) %.9f ;  INTEGRATION TIME (PLC) %i ; RAMPDOWN DWELL (msec/mV) %.4i ; TEMPERATURE (K) %.4f \n ", delay, volt_ramp, volt_stop, resistance_tolerance[0], resistance_tolerance[1], resistance_tolerance[2], resistance_tolerance[3], target_resistance, target_resistance_tolerance, volt_down, ntrigger[0], ntrigger[1], ntrigger[2], ntrigger[3], nnegres[0], nnegres[1], nnegres[2], nnegres[3], res_switch[0], res_switch[1], res_switch[2], nplc, rampdown_dwell, temperature);
	fprintf(output, "voltage,current,resistance \n");
	fflush(output);

	//do measurement then return to 0 voltage
	float exitV = 0;
	exitV = DoRjMeasurement(log_keithley, reading_log_keithley, output, delay, devnum, switchbox); //actual measurement
	GPIBWrite(pna, ":SOUR:VOLT 0.0");

	//close the streams
	fclose(log_keithley);
	fclose(reading_log_keithley);
	fclose(output);

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

	//DoMeasurement(log_keithley, reading_log_keithley, output, delay); //Here the real measurement is done. See comments within the function

	GPIBWrite(pna, ":SOUR:VOLT 0.0"); // set output back to 0 V in preparation for closing channel to next device to be EM'd

	//close the streams
	fclose(log_keithley);
	fclose(reading_log_keithley);

	return 0;
}



float K2400::DoRjMeasurement(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay, int devnum, Switchbox switchbox){
	
	//initialize local variables
	std::string temporary = "";
	char buffer[ARRAYSZ] = "";
	float voltage_read = 0;  // V that keithley reads on a step
	float current_read = 0;  // I that keithley reads on a step
	float  initial_res = 0;  // R at the very beginning (will be set on 1st data point)
	float benchmark_resistance = 0; // target R which triggers an R change count (recomputed at beginning of each ramp)
	float old_resistance = 0; // R on previous V step
	float delta_res = 0; // difference between prior and present R
	float diffres = 0;  // dV/dI between prior and present steps
	int counter_consec = 0; // number consecutive R change events
	int counter_ndr = 0; // number consecutive Neg. Differential R events
	float rangeV; // keithley's V range
	float rangeI; // keithley's I range
	int counter = 0; //number measurements since most recent rampdown
	resistance_timely = 0; // R measured on most recent V step. why is this an attribute of keithley?

	// hold initial V for a bit to equilibrate the device
	float voltage_write = volt_start - volt_ramp; //since we increase the voltage before measurement!
	sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
	Sleep(1000);

	std::cout << "=======================================================================\n";
	std::cout << "Press the 'F12' key to interrupt active EM...\n";
	std::cout << "=======================================================================\n";

	//execute loop until we get required resistance increase or F12 interrupt
	while ((resistance_timely <= target_resistance + initial_res) && !GetAsyncKeyState(VK_F12)){

		// Set the next voltage point and get a reading
		voltage_write += volt_ramp;
		sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
		GPIBWrite(pna, buffer);
		GetReading(buffer, temporary, output, log_keithley);  //read keithley, write to output and log files

		// Calculate present R, and differential R and delta(R) from the previous to present step
		resistance_timely = CalcRes(&diffres, &delta_res, &old_resistance, voltage_read, current_read, reading_log_keithley, output);

		// If this is the first step then set initial R (used for loop exit condition)
		if (counter == 0){
			initial_res = resistance_timely;
		}

		// Update the counters for triggering ramp back
		TrackIndicators(counter_consec, counter_ndr, counter, diffres, delta_res, benchmark_resistance);  // Based on new resistance measurement, update counters for NDR and consecutive delta(R)

		// Based on counters, trigger rampback OR consider range change.
		if (counter_consec >= delta_counts || counter_ndr >= ndr_counts){
			voltage_write = 0.7*voltage_write;
			counter_consec = 0;
			counter_ndr = 0;
			counter = 0;
		}
		else{
			// Based on the present reading, determine desired V, I ranges
			rangeV = determine_voltage_range(voltage_read);
			rangeI = determine_current_range(current_read);

			//If either of the desired ranges are not the present ranges then we need to act:
			if ((rangeV != present_volt_range) || (rangeI != present_curr_range)){

				// First, if we might be in the middle of EM then instead of range changing just do a ramp back
				if ((counter_consec > 1 || counter_ndr > 1) && counter != 1){
					voltage_write = 0.6*voltage_write;
					counter_consec = 0;
					counter_ndr = 0;
					counter = 0;
				}
				// Else if we aren't in the middle of EM then execute the change carefully
				else {
					voltage_write = voltage_write - 20 * volt_ramp;
					sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);

					if (rangeV != present_volt_range){
						switch_voltage_range(switchbox, devnum, rangeV);
					}
					if (rangeI != present_curr_range){
						switch_current_range(switchbox, devnum, rangeI);
					}

					for (int k = 0; k < 20; k++){
						voltage_write += volt_ramp;
						sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
						GPIBWrite(pna, buffer);
						Sleep(15);
					}

				}
			}

		}

		counter++;
	}

	return voltage_write;
}

void K2400::TrackIndicators(int &counter_consec, int &counter_ndr, int &counter, float &diffres, float &delta_res, float &resistance_benchmark){

	//Running Average R calculation and display, for user monitoring purposes only.
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
	else if (counter == highcount){
		char bench[100] = "";
		sprintf(bench, "Avg. Res. is %f", resistance_benchmark);
		std::cout << bench << std::endl;
	}


	// increment if v/i metric gives us delta_res above target
	if (delta_res > delta){
		counter_consec = counter_consec + 1;
		std::cout << "Delta(R) Counter is " << counter_consec << "\n";
	}
	else counter_consec = 0;

	// increment if dv/di metric gives us NDR
	if ((counter > 0) && (diffres < 0)){
		counter_ndr = counter_ndr + 1;
		std::cout << "NDR Counter is " << counter_ndr << "\n";
	}
	else counter_ndr = 0;

}

float K2400::CalcRes(float *diffres, float *delta_res, float *old_resistance, float &voltage_read, float &current_read, FILE* reading_log_keithley, FILE* output){

	//initialize/define the local variables
	char voltage_new[14]; //here we load the string of numbers for voltage before we convert it into a number
	char current_new[14]; //here we load the string of numbers for current before we convert it into a number
	char trash[2]; //comma will be stored here after loading
	float voltage_change = 0;
	float current_change = 0;
	float voltage_new_num = 0;
	float current_new_num = 0;
	float resistance = 0;

	//get V,I from the log file as strings and compute resistance
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
	resistance = voltage_new_num / current_new_num;

	//compute the dV and dI for NDR metric
	voltage_change = voltage_new_num - voltage_read;
	current_change = current_new_num - current_read;
	*diffres = voltage_change / current_change;

	// calc change in res and then update old_resistance for next point
	*delta_res = resistance - *old_resistance;
	*old_resistance = resistance;

	// assign new values to the voltage and intensity variables
	voltage_read = voltage_new_num;
	current_read = current_new_num;

	//print the resistance and ranges to output file
	fprintf(output, "%f, ", resistance);
	fflush(output);
	fprintf(output, "%f, %f\n", present_volt_range, present_curr_range);

	//the function returns the value of resistance;
	return resistance;
}



float K2400::DoMeasurement(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay, Switchbox switchbox, int devnum){
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
	float old_resistance = 0;
	float voltage_write = volt_start - volt_ramp; //we omit the point V=0 by increasing the voltage before measurement!
	float voltage_read = 0;
	float current_read = 0;

	char delays[40] = "";
	sprintf(delays, ":SOUR:DEL %f", delay);
	int counter = 0; //number of measurements since the most recent ramp_down
	//execute the code in for loop until we get the required resistance +/- resistance_tol*resistance
	int switchcounter = 0;
	float resistance_initial = 100000000;

	float rangeV;
	float rangeI;


	int first_ramp = 1;

	// hold the initial voltage for a bit to equilibrate the device
	sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
	Sleep(5000);
	resistance_timely = 0;


	for (; GotTargetResistance(resistance_timely, targ_res_consecutive, volt_stop, voltage_read);){

		// This machinery is to get the conductance steps breaking for Mark (160220)
		//if (resistance_timely > 1500){
		//	voltage_write += 0.00005;
		//}
		//else{
		//	voltage_write += volt_ramp;    //setting new voltage on Keithley
		//}

		voltage_write += volt_ramp;    //setting new voltage on Keithley
		sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);    //create a new string of a command
		GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
		GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files

		//compute the resistance on basis of measurement from Keithley. Also writes it to output and log
		// 140422 (sonya) temporarily try just V/I instead of differential resistance computation - see GetResistance function
		resistance_timely = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter);

		// set the initial resistance if this is the 20th data point taken (give some time to settle)
		if (switchcounter == 20 || switchcounter == 0){
			resistance_initial = resistance_timely;
			std::cout << "Initial Resistance is set to " << resistance_timely << " ohms.\n";
			std::cout << "Exit condition (Initial + Target) will be " << (resistance_initial + target_resistance) << " ohms.\n";
			std::cout << "**************************************************\n";
		}

		//this function keeps tracks of the count of consecutive weird measurements and takes care of setting resistance_benchmark
		KeepTrackIndicators(res_consecutive, targ_res_consecutive, neg_res_consecutive, bench_res_consecutive, resistance_benchmark,
			resistance_timely, resistance_initial, diffres, target_resistance, target_resistance_tolerance, counter);

		counter++;  //increase the counter to keep track of whether we are at the beginning of ramp cycle or not
		switchcounter++; //this is just used to store a true initial resistance for the wire against which to check for whether to switch to highR or lowR regime

		//check if we need to ramp down the voltage. If so, do it and zero the counter. Check if we
		//decrease the voltage below 0. if so, break the measurement. display message.
		if (CheckRampDown(resistance_timely, resistance_initial, res_consecutive, bench_res_consecutive, neg_res_consecutive)){

			// 140430 (SDS) instantaneously ramp down to 50% of present voltage
			voltage_write = 0.7*voltage_write;
			sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
			GPIBWrite(pna, buffer);
			if (voltage_write <= 0){
				printf("ERROR! We just tried to lower the voltage below 0! Stopping the measurement! \n");
				break;
			}

			first_ramp = 0;  // This is what lets us know whether to permit range changing - do not allow unless on the initial ramp up.

			counter = 0; 	 // after any ramp down reset counter to 0 so it will take new benchmark resistance on next 6 iterations of loop.
			bench_res_consecutive = 0; // we have to get this to 0. otherwise we might run into trouble with many ramp downs one after another
			neg_res_consecutive = 0; // we have to get this to 0. otherwise we might run into trouble with many ramp downs one after another
			res_consecutive = 0;
		}
		// Based on the present reading, determine what V, I ranges we should be using
		rangeV = determine_voltage_range(voltage_read);
		rangeI = determine_current_range(current_read);

		//If either of the desired ranges are not the present ranges then we might act:
		if ((rangeV != present_volt_range) || (rangeI != present_curr_range)){
			// If we are on the initial ramp up then do a safe range change
			if (first_ramp == 1){

				float volt_temp = voltage_write;
				float volt_step = volt_temp / 10;
				for (int k = 0; k < 10; k++){
					volt_temp -= volt_step;
					sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
					GPIBWrite(pna, buffer);
					Sleep(3);
				}


				if (rangeV != present_volt_range){
					switch_voltage_range(switchbox, devnum, rangeV);
				}
				if (rangeI != present_curr_range){
					switch_current_range(switchbox, devnum, rangeI);
				}

				for (int k = 0; k < 10; k++){
					volt_temp += volt_step;
					sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
					GPIBWrite(pna, buffer);
					Sleep(3);
				}


				//voltage_write = 0.7*voltage_write;
				//counter = 0; 	 // after any ramp down reset counter to 0 so it will take new benchmark resistance on next 6 iterations of loop.
				//bench_res_consecutive = 0; // we have to get this to 0. otherwise we might run into trouble with many ramp downs one after another
				//neg_res_consecutive = 0; // we have to get this to 0. otherwise we might run into trouble with many ramp downs one after another
				//res_consecutive = 0;
				//std::cout << "Ramp Back Due to Range Change.\n";
			}
			// Else if we aren't in the middle of EM then execute the change carefully
			else if (counter == 0 && (rangeI != present_curr_range)){
				float volt_temp = voltage_write;
				float volt_step = volt_temp / 10;
				for (int k = 0; k < 10; k++){
					volt_temp -= volt_step;
					sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
					GPIBWrite(pna, buffer);
					Sleep(3);
				}

				if (rangeI != present_curr_range){
					switch_current_range(switchbox, devnum, rangeI);
				}

				for (int k = 0; k < 10; k++){
					volt_temp += volt_step;
					sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
					GPIBWrite(pna, buffer);
					Sleep(3);
				}
			}
		}

	}
	return voltage_write;
}

float K2400::DoAlternateMeasurement(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay){
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
	float old_resistance = 0;
	float voltage_write = volt_start - volt_ramp; //we omit the point V=0 by increasing the voltage before measurement!
	float voltage_read = 0;
	float current_read = 0;

	char delays[40] = "";
	sprintf(delays, ":SOUR:DEL %f", delay);
	int counter = 0; //number of measurements since the most recent ramp_down
	//execute the code in for loop until we get the required resistance +/- resistance_tol*resistance
	int switchcounter = 0;
	float resistance_initial = 100000000;

	// hold the initial voltage for a bit to equilibrate the device
	sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
	Sleep(5000);
	resistance_timely = 0;

	for (; GotTargetResistance(resistance_timely, targ_res_consecutive, volt_stop, voltage_read);){

		// This machinery is to get the conductance steps breaking for Mark (160220)
		//if (resistance_timely > 1500){
		//	voltage_write += 0.00005;
		//}
		//else{
		//	voltage_write += volt_ramp;    //setting new voltage on Keithley
		//}

		voltage_write += volt_ramp;    //setting new voltage on Keithley
		sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);    //create a new string of a command
		GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
		GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files

		//compute the resistance on basis of measurement from Keithley. Also writes it to output and log
		// 140422 (sonya) temporarily try just V/I instead of differential resistance computation - see GetResistance function
		resistance_timely = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter);

		// set the initial resistance if this is the 20th data point taken (give some time to settle)
		if (switchcounter == 20){
			resistance_initial = resistance_timely;
			std::cout << "Initial Resistance is set to " << resistance_benchmark << " ohms.\n";
			std::cout << "Exit condition (Initial + Target) will be " << (resistance_initial + target_resistance) << " ohms.\n";
		}

		//this function keeps tracks of the count of consecutive weird measurements and takes care of setting resistance_benchmark
		KeepTrackIndicators(res_consecutive, targ_res_consecutive, neg_res_consecutive, bench_res_consecutive, resistance_benchmark,
			resistance_timely, resistance_initial, diffres, target_resistance, target_resistance_tolerance, counter);

		counter++;  //increase the counter to keep track of whether we are at the beginning of ramp cycle or not
		switchcounter++; //this is just used to store a true initial resistance for the wire against which to check for whether to switch to highR or lowR regime

		//check if we need to ramp down the voltage. If so, do it and zero the counter. Check if we
		//decrease the voltage below 0. if so, break the measurement. display message.
		if (CheckRampDown(resistance_timely, resistance_initial, res_consecutive, bench_res_consecutive, neg_res_consecutive)){

			// 140430 (SDS) instantaneously ramp down to 50% of present voltage
			voltage_write = -0.8*voltage_write;
			volt_ramp = -volt_ramp;
			sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
			GPIBWrite(pna, buffer);

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

	volt_ramp = abs(volt_ramp);  // For EMing multiple devices, want to always get this back to positive for the initial ramp
	return voltage_write;
}

float K2400::DoCurvatureMeasurement(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay){
	//initialize local variables
	std::string temporary = "";
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning

	//counter_0 is number of measurements since the most recent ramp_down, counter_1 is number of measurements since 
	//most recent voltage increment, counter_2 is number of consecutive R > 1.005*Rbench hits and counter_3 is number of 
	//consecutive R > 1.04*Rbench hits
	int counter_0 = 0, counter_1 = 0, counter_2 = 0, counter_3 = 0;
	int targ_res_consecutive = 0;

	resistance_timely = 0;
	resistance_benchmark = 0;

	float diffres = 0;
	float old_resistance = 0;
	float voltage_write = volt_start; //we omit the point V=0 by increasing the voltage before measurement!
	float voltage_read = 0;
	float current_read = 0;

	char delays[40] = "";
	sprintf(delays, ":SOUR:DEL %f", delay);

	//execute the code in for loop until we get the required resistance +/- resistance_tol*resistance
	int switchcounter = 0;
	float resistance_initial = 100000000;
	int bestString = 0;

	// hold the initial voltage for a bit to equilibrate the device
	sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
	Sleep(5000);
	resistance_timely = 0;

	//set up the machinery for timestamping each measurement
	//set up time keeping apparatus
	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::milliseconds milliseconds;
	Clock::time_point t0;
	Clock::time_point t1;
	milliseconds Elapsed;
	t0 = Clock::now();

	// initialize the variable that holds resistance check value for dwelling confirmation
	float rescheck = 100;

	for (; GotTargetResistance(resistance_timely, targ_res_consecutive, volt_stop, voltage_read);){

		// write a timestamp for that measurement
		//print the resultant resistance to output file
		t1 = Clock::now();
		Elapsed = std::chrono::duration_cast<milliseconds>(t1 - t0);
		fprintf(output, "%i, ", Elapsed.count());
		fflush(output);

		sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);    //create a new string of a command
		GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
		GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files

		//compute the resistance on basis of measurement from Keithley. Also writes it to output and log
		// 140422 (sonya) temporarily try just V/I instead of differential resistance computation - see GetResistance function
		resistance_timely = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter_0);


		// set the initial resistance if this is the 20th data point taken (give some time to settle)
		if (switchcounter == 20){
			resistance_initial = resistance_benchmark;
			std::cout << "Initial Resistance is set to " << resistance_initial << " ohms.\n";
			std::cout << "Exit condition (Initial + Target) will be " << (resistance_initial + target_resistance) << " ohms.\n";
		}

		// update indicators for counting number of points since previous ramp back, and number of points dwelt on this voltage
		counter_0++;
		counter_1++;
		switchcounter++; //this is just used to store a true initial resistance for the wire against which to check for whether to switch to highR or lowR regime
		
		// update indicators for whether the present resistance is indicating EM onset or rapid EM
		KeepTrackCurvatureIndicators(counter_0, counter_1, counter_2, counter_3, bestString, rampup_thresh, rampback_thresh, targ_res_consecutive, resistance_benchmark, resistance_timely, old_resistance, resistance_initial, diffres, target_resistance, target_resistance_tolerance);

		////check if we need to ramp up, ramp down or keep fixed the voltage, and zero the counters as needed. Check if we
		////decrease the voltage below 0. if so, break the measurement. display message.
		////if (counter_1 < 5){
		////	voltage_write = voltage_write;
		////}
		////else{
		////	if (bestString < 4 && counter_1 == 5){
		////		voltage_write += volt_ramp;
		////		counter_1 = 0;
		////		counter_2 = 0;
		////		bestString = 0;
		////		std::cout << "Resistance is " << resistance_timely << " ohms. Step up to " << voltage_write << " V.\n";
		////	}
		////	else{
		////		//voltage_write = voltage_write*0.8;
		////		// 160303 - modifying to dwell after a string of R > Rtol is found
		////		voltage_write = voltage_write;
		/////*		counter_0 = 0;
		////		counter_1 = 0;
		////		counter_2 = 0;
		////		bestString = 0;*/
		////		std::cout << "Resistance is " << resistance_timely << " ohms.\n";
		////		// std::cout << "**RAMP BACK TRIGGERED**\n";
		////	}
		////}
	
		// old algo: 5 points at each voltage and look for consecutively increasing

		//// More complicated dwell routine - dwells on four consecutive increasing R's, but then checks again after another 15 measurements at that voltage to see if it was a faulty dwell.
		if (counter_1 < 5){
			voltage_write = voltage_write; // always take at least five measurements at a given voltage
			std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
		}
		else if (counter_1 == 5){		// once five measurements are taken, check whether to stay at this voltage
			if (bestString < 3){		// if there were less than four consecutive increasing R measurements then ramp up V, otherwise dwell at V for another measurement
				voltage_write += volt_ramp;
				counter_1 = 0;
				counter_2 = 0;
				bestString = 0;
				std::cout << "Resistance is " << resistance_timely << " ohms. Step up to " << voltage_write << " V.\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
			}
			else {
				voltage_write = voltage_write;
				counter_2 = 0;
				bestString = 0;  // reset bestString to prepare for checking whether this a false dwell
				std::cout << "DWELL TRIGGERED: Resistance is " << resistance_timely << " ohms.\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
				rescheck = resistance_timely + 0.05;
			}
		}
		else if (counter_1 > 5 && counter_1 < 10){		// always take at least 5 more measurements at the V where a dwell is initiated
			voltage_write = voltage_write;
			std::cout << "DWELL CHECK 1: Resistance is " << resistance_timely << " ohms.\n";
			std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
		}
		else if (counter_1 == 10){		// after 5 extra measurements check whether it was a false dwell or if we really do have EM
			if (bestString < 3){
				voltage_write += volt_ramp;
				counter_1 = 0;
				counter_2 = 0;
				bestString = 0;
				std::cout << "FALSE DWELL!\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
			}
			else {
				voltage_write = voltage_write;
				std::cout << "DWELL CHECK #1 PASSED: Resistance is " << resistance_timely << " ohms.\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
				bestString = 0;
				counter_2 = 0;
			}
			/*if (resistance_timely < rescheck){
				voltage_write += volt_ramp;
				counter_1 = 0;
				counter_2 = 0;
				bestString = 0;
				std::cout << "FALSE DWELL!\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
			}
			else {
				voltage_write = voltage_write;
				std::cout << "DWELL CONFIRMED: Resistance is " << resistance_timely << " ohms.\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
			}*/
		}
		else if (counter_1 > 10 && counter_1 < 15){		// always take at least 5 more measurements at the V where a dwell is initiated
			voltage_write = voltage_write;
			std::cout << "DWELL CHECK 2: Resistance is " << resistance_timely << " ohms.\n";
			std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
		}
		else if (counter_1 == 15){		// after 5 extra measurements check whether it was a false dwell or if we really do have EM
			if (bestString < 3){
				voltage_write += volt_ramp;
				counter_1 = 0;
				counter_2 = 0;
				bestString = 0;
				std::cout << "FALSE DWELL!\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
			}
			else {
				voltage_write = voltage_write;
				std::cout << "DWELL CHECK #2 PASSED: Resistance is " << resistance_timely << " ohms.\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
				rescheck = resistance_timely + 0.3;
			}
		}
		else if (counter_1 > 15 && counter_1 < 110){		// always take at least 100 more measurements at the V where a dwell is initiated
			voltage_write = voltage_write;
			std::cout << "DWELL CHECK 3: Resistance is " << resistance_timely << " ohms.\n";
			std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
		}
		else if (counter_1 == 110){		// after 25 extra measurements check whether it was a false dwell or if we really do have EM
			if (resistance_timely <= rescheck){
				voltage_write += volt_ramp;
				counter_1 = 0;
				counter_2 = 0;
				bestString = 0;
				std::cout << "FALSE DWELL!\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
			}
			else {
				voltage_write = voltage_write;
				std::cout << "DWELL CHECK #3 PASSED: Resistance is " << resistance_timely << " ohms.\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
				rescheck = resistance_timely + 0.3;
			}
		}
		else if (counter_1 > 110 && counter_1 < 220){		// always take at least 100 more measurements at the V where a dwell is initiated
			voltage_write = voltage_write;
			std::cout << "DWELL CHECK #4: Resistance is " << resistance_timely << " ohms.\n";
			std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
		}
		else if (counter_1 == 220){		// after 100 extra measurements check whether it was a false dwell or if we really do have EM
			if (resistance_timely <= rescheck){
				voltage_write += volt_ramp;
				counter_1 = 0;
				counter_2 = 0;
				bestString = 0;
				std::cout << "FALSE DWELL!\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
			}
			else {
				voltage_write = voltage_write;
				std::cout << "DWELL CHECK #4 PASSED: Resistance is " << resistance_timely << " ohms.\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
				rescheck = resistance_timely + 0.3;
			}
		}
		else if (counter_1 > 220 && counter_1 < 320){		// always take at least 100 more measurements at the V where a dwell is initiated
			voltage_write = voltage_write;
			std::cout << "DWELL CHECK #5: Resistance is " << resistance_timely << " ohms.\n";
			std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
		}
		else if (counter_1 == 320){		// after 100 extra measurements check whether it was a false dwell or if we really do have EM
			if (resistance_timely <= rescheck){
				voltage_write += volt_ramp;
				counter_1 = 0;
				counter_2 = 0;
				bestString = 0;
				std::cout << "FALSE DWELL!\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
			}
			else {
				voltage_write = voltage_write;
				std::cout << "DWELL CHECK #5 PASSED: Resistance is " << resistance_timely << " ohms.\n";
				std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
			}
		}
		else {
			voltage_write = voltage_write;
			std::cout << "DWELLING: Resistance is " << resistance_timely << " ohms.\n";
			std::cout << "counter_1 = " << counter_1 << " , bestString = " << bestString << ".\n";
		}


		// another old algo: 25 points at each, then 100 point dwell check, then 500 point dwell check

		//// 25 points at each voltage, looking for certain absolute R increase, several increasingly long dwell checks
		//if (counter_1 < 25){
		//	voltage_write = voltage_write; // always take at least 25 measurements at a given voltage
		//	if (counter_1 == 3){
		//		rescheck = resistance_timely + 0.2;
		//	}
		//}
		//else if (counter_1 == 25){		// once five measurements are taken, check whether to stay at this voltage
		//	if (resistance_timely < rescheck){		// if R hasn't increased enough ramp up V, otherwise dwell at V for another measurement
		//		voltage_write += volt_ramp;
		//		// reset counter for # of measurements and initialize to the new resistance check value
		//		counter_1 = 0;
		//		rescheck = resistance_timely + 0.2;
		//		std::cout << "Resistance is " << resistance_timely << " ohms. Step up to " << voltage_write << " V.\n";
		//	}
		//	else {
		//		voltage_write = voltage_write;
		//		std::cout << "DWELL TRIGGERED: Resistance is " << resistance_timely << " ohms.\n";
		//		rescheck = resistance_timely + 0.4;
		//	}
		//}
		//else if (counter_1 > 25 && counter_1 < 100){		// always take at least 75 more measurements at the V where a dwell is initiated
		//	voltage_write = voltage_write;
		//	std::cout << "DWELL CHECK 1: Resistance is " << resistance_timely << " ohms.\n";
		//}
		//else if (counter_1 == 100){		// after 75 extra measurements check whether it was a false dwell or if we really do have EM
		//	if (resistance_timely < rescheck){
		//		voltage_write += 0.001;
		//		// reset counter for # of measurements and initialize to the new resistance check value
		//		counter_1 = 0;
		//		rescheck = resistance_timely + 0.2;
		//		std::cout << "DWELL CHECK #1 FAILED!\n";
		//	}
		//	else {
		//		voltage_write = voltage_write;
		//		std::cout << "DWELL CHECK #1 PASSED: Resistance is " << resistance_timely << " ohms.\n";
		//		rescheck = resistance_timely + 2;
		//	}
		//}
		//else if (counter_1 > 100 && counter_1 < 500){		// always take at least 400 more measurements at the V where a dwell is initiated
		//	voltage_write = voltage_write;
		//	std::cout << "DWELL CHECK 2: Resistance is " << resistance_timely << " ohms.\n";
		//}
		//else if (counter_1 == 500){		// after 25 extra measurements check whether it was a false dwell or if we really do have EM
		//	if (resistance_timely <= rescheck){
		//		voltage_write += 0.001;
		//		// reset counter for # of measurements and initialize to the new resistance check value
		//		counter_1 = 0;
		//		rescheck = resistance_timely + 0.2;
		//		std::cout << "DWELL CHECK #2 FAILED!\n";
		//	}
		//	else {
		//		voltage_write = voltage_write;
		//		std::cout << "DWELL CHECK #2 PASSED: Resistance is " << resistance_timely << " ohms.\n";
		//	}
		//}
		//else {
		//	voltage_write = voltage_write;
		//	std::cout << "DWELLING: Resistance is " << resistance_timely << " ohms.\n";
		//}


		// 100 points at each voltage, dwell check is Rinc > some value, never dwell forever just keep checking dwell check every 100 points

		//if (counter_1 < 100){
		//	voltage_write = voltage_write; // always take at least 25 measurements at a given voltage
		//	if (counter_1 == 5){
		//		rescheck = resistance_timely + 0.5;
		//	}
		//}
		//else if (counter_1 == 100){		// once five measurements are taken, check whether to stay at this voltage
		//	if (resistance_timely < rescheck){		// if R hasn't increased enough ramp up V, otherwise dwell at V for another measurement
		//		voltage_write += volt_ramp;
		//		// reset counter for # of measurements and initialize to the new resistance check value
		//		counter_1 = 0;
		//		std::cout << "RAMP UP: Resistance is " << resistance_timely << " ohms. Step up to " << voltage_write << " V.\n";
		//	}
		//	else {
		//		voltage_write = voltage_write;
		//		counter_1 = 0;
		//		std::cout << "DWELL: Resistance is " << resistance_timely << " ohms.\n";
		//	}
		//}
		//
		//else {
		//	voltage_write = voltage_write;
		//	std::cout << "DWELLING: Resistance is " << resistance_timely << " ohms.\n";
		//}



















		sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
		GPIBWrite(pna, buffer);
		if (voltage_write <= 0){
			printf("ERROR! We just tried to lower the voltage below 0! Stopping the measurement! \n");
			break;
		}


		// Turn the counter_1 exit condition into a target exit R condition, just to save some time with coding....
		if (counter_1 > 2000){
			resistance_timely = 1000000;
		}

	}

	return voltage_write;
}

float K2400::DoStabilizeMeasurement(FILE* log_keithley, FILE* reading_log_keithley, FILE* output, float delay, float Vstab, float stabstep){
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
	float old_resistance = 0;
	float voltage_write = volt_start - volt_ramp; //we omit the point V=0 by increasing the voltage before measurement!
	float voltage_read = 0;
	float current_read = 0;

	char delays[40] = "";
	sprintf(delays, ":SOUR:DEL %f", delay);
	int counter = 0; //number of measurements since the most recent ramp_down
	//execute the code in for loop until we get the required resistance +/- resistance_tol*resistance
	int switchcounter = 0;
	float resistance_initial = 100000000;

	//// hold the initial voltage for a bit to equilibrate the device
	//sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);
	//Sleep(5000);

	std::cout << "Press F10 to halt current stabilization.\n";
	voltage_write = Vstab;

	while (resistance_timely < 15000 && !GetAsyncKeyState(VK_F10)){
		voltage_write = voltage_write + stabstep;    //setting new voltage on Keithley
		sprintf(buffer, ":SOUR:VOLT %f\n", voltage_write);    //create a new string of a command
		GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
		GetReading(buffer, temporary, output, log_keithley); //get what keithley displays and write it to output and log files

		//compute the resistance on basis of measurement from Keithley. Also writes it to output and log
		// 140422 (sonya) temporarily try just V/I instead of differential resistance computation - see GetResistance function
		resistance_timely = GetResistance(&diffres, &old_resistance, voltage_read, current_read, reading_log_keithley, output, counter);
		std::cout << resistance_timely << " ohms.\n";
	}


	return voltage_write;
}




float K2400::GetResistance(float *diffres, float *old_resistance, float &voltage_read, float &current_read, FILE* reading_log_keithley, FILE* output, int counter){

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

	//store a value that holds the resistance of the prior data point
	*old_resistance = voltage_read / current_read;

	//and assign new values to the voltage and intensity variables
	voltage_read = voltage_new_num;
	current_read = current_new_num;

	//140422 (sonya) temporarily do V/I rather than differential resistance.
	resistance = voltage_new_num / current_new_num;

	//print the resultant resistance to output file
	fprintf(output, "%f, ", resistance);
	fflush(output);
	fprintf(output, "%f, %f\n", present_volt_range, present_curr_range);


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
	float &resistance_timely, float resistance_initial, float &diffres, float target_resistance, float target_resistance_tolerance, int &counter){

	// pick which resistance tolerance we are using depending on what regime we are in. The value starts out as that of the highest-R regime and then we loop through the regimes - restol gets overwritten on each successive loop if the endpoint of that regime is above Rjct. 
	float restol = resistance_tolerance[3];
	float compare;
	float Rjct = max(0, (resistance_timely - resistance_initial));
	for (int k = 3; k >= 0; --k){
		compare = res_switch[k] - Rjct;
		if (compare > 0){
			restol = resistance_tolerance[k];
		}
	}

	// ***********************************************
	// This is all modified for the series-r-normalized algorithm:
	// ***********************************************

	float res_norm = Rjct + avg_rs;
	//if it is the first measurement "down the hill", use it to set benchmark. Reject first n data points up to lowcount, then use next
	// m data points up to highcount to generate an average resistance as benchmark.

	int highcount = 13;
	int lowcount = 5;

	if (counter < highcount){
		if (counter < lowcount){
			resistance_benchmark = res_norm;
		}
		else{
			resistance_benchmark = (resistance_benchmark*(counter - lowcount + 1) + res_norm) / (counter - lowcount + 2);
		}
	}

	if (counter == highcount){
		char bench[100] = "";
		sprintf(bench, "Norm'lzd Benchmark = %f (Non-Norm'lzd R = %f)\n", resistance_benchmark, resistance_timely);
		std::cout << bench << std::endl;
	}


	// increment for whether v/i metric gives us res > res benchmark
	if ((counter > highcount && (res_norm >= (resistance_benchmark*(1 + restol))))){
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
	if ((resistance_timely >(resistance_initial + target_resistance)*(1 - target_resistance_tolerance)) | (resistance_timely<0)){
		targ_res_consecutive = targ_res_consecutive + 1;
	}
	else targ_res_consecutive = 0;





	// ***********************************************
	// This is the original algo:
	// ***********************************************


	////if it is the first measurement "down the hill", use it to set benchmark. Reject first n data points up to lowcount, then use next
	//// m data points up to highcount to generate an average resistance as benchmark.

	//int highcount = 13;
	//int lowcount = 5;

	//if (counter < highcount){
	//	if (counter < lowcount){
	//		resistance_benchmark = resistance_timely;
	//	}
	//	else{
	//		resistance_benchmark = (resistance_benchmark*(counter - lowcount + 1) + resistance_timely) / (counter - lowcount + 2);
	//	}
	//}

	//if (counter == highcount){
	//	char bench[100] = "";
	//	sprintf(bench, "avg resistance is %f", resistance_benchmark);
	//	std::cout << bench << std::endl;
	//}

	////if (counter > highcount && (resistance_timely >= (resistance_benchmark*(1 + resistance_tolerance)))){
	//// bench_res_consecutive = bench_res_consecutive + 1;
	////}
	////else bench_res_consecutive = 0;

	////// update the indicator for whether we have a string of consecutive negative resistance hits.
	////if (resistance_timely < 0){
	//// neg_res_consecutive = neg_res_consecutive + 1;
	////}
	////else neg_res_consecutive = 0;

	//// increment for whether v/i metric gives us res > res benchmark
	//if ((counter > highcount && (resistance_timely >= (resistance_benchmark*(1 + restol))))){
	//	res_consecutive = res_consecutive + 1;
	//	std::cout << "Hit. Res_consecutive is " << res_consecutive << "\n";
	//}
	//else res_consecutive = 0;

	//// increment for whether dv/di metric gives us NDR - this indicator increments at any point in the ramp, even if we are still in the initial part of the ramp (counter < highcount)
	//if (diffres < 0){
	//	neg_res_consecutive = neg_res_consecutive + 1;
	//}
	//else neg_res_consecutive = 0;

	//// update the indicator for whether we have a string of consecutive target resistance hits. We are looking only for stuff above
	//if ((resistance_timely > (resistance_initial+target_resistance)*(1 - target_resistance_tolerance)) | (resistance_timely<0)){
	//	targ_res_consecutive = targ_res_consecutive + 1;
	//}
	//else targ_res_consecutive = 0;


}

void K2400::KeepTrackCurvatureIndicators(int &counter_0, int &counter_1, int &counter_2, int &counter_3, int &bestString, float rampup_thresh, float rampback_thresh, int &targ_res_consecutive, float &resistance_benchmark,
	float &resistance_timely, float old_resistance, float resistance_initial, float &diffres, float target_resistance, float target_resistance_tolerance){

	//if it is the first measurement "down the hill", use it to set benchmark. Reject first n data points up to lowcount, then use next
	// m data points up to highcount to generate an average resistance as benchmark.
	int highcount = 15;
	int lowcount = 5;

	if (counter_0 < highcount){
		if (counter_0 < lowcount){
			resistance_benchmark = resistance_timely;
		}
		else{
			resistance_benchmark = (resistance_benchmark*(counter_0 - lowcount + 1) + resistance_timely) / (counter_0 - lowcount + 2);
		}
	}

	if (counter_0 == highcount){
		char bench[100] = "";
		sprintf(bench, "avg resistance is %f", resistance_benchmark);
		std::cout << "BENCHMARK COMPUTED - " << resistance_benchmark << " ohms. \n";
	}

	// bestString tracks number of consecutive measurements where R has increased compared to previous measurement
	// increment bestString for whether this data point has a higher resistance than the previous one
	if ((counter_0 > highcount) && (resistance_timely >= old_resistance)){
		counter_2 = counter_2 + 1;
		bestString = max(bestString, counter_2);
		//std::cout << "EM ONSET DETECTED - consecutive points is " << counter_2 << ".\n";
	}
	else {
		bestString = max(bestString, counter_2);
		counter_2 = 0;
	}

	//// 160303 Change counter_2 from looking for consecutive increases in R to looking for non-consecutive count of R > (1+tol)*Rbench
	//// increment for whether this data point has a resistance greater than the benchmark by some tolerance
	//if ((counter_0 > highcount) && (resistance_timely >= (1+rampback_thresh)*resistance_benchmark)){
	//	counter_2 = counter_2 + 1;
	//	bestString = counter_2;
	//	//std::cout << "EM ONSET DETECTED - consecutive points is " << counter_2 << ".\n";
	//}
	//else {
	//	/*bestString = max(bestString, counter_2);*/
	//	//counter_2 = 0;
	//}



	//// increment for whether v/i metric gives us R such that we want to NOT increment the voltage
	//if ((counter_0 > highcount && (resistance_timely >= (resistance_benchmark*(1 + rampup_thresh))))){
	//	counter_2 = counter_2 + 1;
	//	std::cout << "EM ONSET DETECTED - consecutive points is " << counter_2 << ".\n";
	//}
	//else counter_2 = 0;

	//// increment for whether v/i metric gives us R such that we want to ramp back the voltage
	//if ((counter_0 > highcount && (resistance_timely >= (resistance_benchmark*(1 + rampback_thresh))))){
	//	counter_3 = counter_3 + 1;
	//	std::cout << "RAPID EM DETECTED - consecutive points is " << counter_3 << ".\n";
	//}
	//else counter_3 = 0;

	// update the indicator for whether we have a string of consecutive target resistance hits. We are looking only for stuff above
	if ((resistance_timely >(resistance_initial + target_resistance)*(1 - target_resistance_tolerance)) | (resistance_timely<0)){
		targ_res_consecutive = targ_res_consecutive + 1;
	}
	else targ_res_consecutive = 0;


}

void K2400::KeepTrackHealIndicators(int &res_consecutive, int &targ_res_consecutive, int &neg_res_consecutive, int &bench_res_consecutive, float &resistance_benchmark,
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


	// pick which resistance tolerance we are using depending on what regime we are in. The value starts out as that of the highest-R regime and then we loop through the regimes - restol gets overwritten on each successive loop if the endpoint of that regime is above Rjct. 
	float restol = resistance_tolerance[3];
	float compare;
	float Rjct = max(0, resistance_timely);
	for (int k = 3; k >= 0; --k){
		compare = res_switch[k] - Rjct;
		if (compare > 0){
			restol = resistance_tolerance[k];
		}
	}

	// increment for whether v/i metric gives us res > res benchmark
	if ((counter > highcount && (resistance_timely <= (resistance_benchmark*(1 - restol))))){
		res_consecutive = res_consecutive + 1;
		std::cout << "Hit. Res_consecutive is " << res_consecutive << "\n";
	}
	else res_consecutive = 0;

	//// increment for whether dv/di metric gives us NDR - this indicator increments at any point in the ramp, even if we are still in the initial part of the ramp (counter < highcount)
	//if (diffres > 0){
	//	neg_res_consecutive = neg_res_consecutive + 1;
	//}
	//else neg_res_consecutive = 0;

	// update the indicator for whether we have a string of consecutive target resistance hits. We are looking only for stuff above
	if (resistance_timely < target_resistance*(1 - target_resistance_tolerance)){
		targ_res_consecutive = targ_res_consecutive + 1;
	}
	else targ_res_consecutive = 0;


}

bool K2400::CheckRampDown(float resistance_timely, float resistance_initial, int &res_consecutive, int &bench_res_consecutive, int &neg_res_consecutive){

	// pick which consecutive numbers we are using depending on what regime we are in. The value starts out as that of the highest-R regime and then we loop through the regimes - value gets overwritten on each successive loop if the endpoint of that regime is above Rjct. 
	float ntrig = ntrigger[3];
	float nndrs = nnegres[3];
	float compare;
	float Rjct = max(0, (resistance_timely - resistance_initial));
	for (int k = 3; k >= 0; --k){
		compare = res_switch[k] - Rjct;
		if (compare > 0){
			ntrig = ntrigger[k];
			nndrs = nnegres[k];
		}
	}


	// check the consecutive increased resistances and consecutive negative differential resistances conditions
	if ((res_consecutive >= ntrig) | (neg_res_consecutive >= nndrs)){
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




// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- GATING AND DEP -----------------------------------
// ---------------------------------------------------------------------------------------------------------

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

void K2400::setGateVoltage(float voltage){
	// sourcing voltage
	GPIBWrite(pna, ":SOUR:FUNC VOLT");
	// sensing mode DC current
	GPIBWrite(pna, ":SENS:FUNC 'CURR:DC'");
	//       // voltage source range in auto
	//GPIBWrite(pna,":SOUR:VOLT:RANGE:AUTO ON");
	//       // current sensing range in auto
	GPIBWrite(pna, ":SENS:CURR:RANGE:AUTO ON");
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

	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	sprintf(buffer, ":SOUR:VOLT %f\n", voltage);    //create a new string of a command for sourcing the voltage
	GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
	GPIBWrite(pna, ":OUTP ON");
}

void K2400::holdDEPVoltage(float voltage, float holdTime){
	// sourcing voltage
	GPIBWrite(pna, ":SOUR:FUNC VOLT");
	// sensing mode DC current
	GPIBWrite(pna, ":SENS:FUNC 'CURR:DC'");
	//       // voltage source range in auto
	//GPIBWrite(pna,":SOUR:VOLT:RANGE:AUTO ON");
	//       // current sensing range in auto
	GPIBWrite(pna, ":SENS:CURR:RANGE:AUTO ON");
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

	
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	sprintf(buffer, ":SOUR:VOLT %f\n", voltage);    //create a new string of a command for sourcing the voltage
	GPIBWrite(pna, buffer);     //and send it to the source (Keithley)
	GPIBWrite(pna, ":OUTP ON");

	typedef std::chrono::high_resolution_clock Clock;
	Clock::time_point t0;
	Clock::time_point t1;
	typedef std::chrono::minutes minutes;
	minutes Elapsed;

	t0 = Clock::now();

	// hold the voltage for the allotted time or until the user presses F12
	std::cout << "=======================================================================\n";
	std::cout << "Press 'F12' to stop sourcing the DEP voltage... \n";
	std::cout << "=======================================================================\n";
	t1 = Clock::now();
	Elapsed = std::chrono::duration_cast<minutes>(t1 - t0);
	while (Elapsed.count() < holdTime && !GetAsyncKeyState(VK_F12)){
		Sleep(100);
		GPIBWrite(pna, ":READ?");
		GPIBRead(pna, buffer);
		t1 = Clock::now();
		Elapsed = std::chrono::duration_cast<minutes>(t1 - t0);
	}

	// go back to 0 voltage on gateline
	GPIBWrite(pna, ":SOUR:VOLT 0");
	//GPIBWrite(pna, ":OUTP OFF");
}



// ---------------------------------------------------------------------------------------------------------
// --------------------------------------- READING, WRITING, SUBPROCESSES -----------------------------------
// ---------------------------------------------------------------------------------------------------------

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


