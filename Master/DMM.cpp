#include "DMM.h"
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
DMM::DMM(){
	configure();
}

// this is the destructor method, see comments in k2400.h
DMM::~DMM(){
	//dtor
}

int DMM::configure(){

	int ERROR1 = 0; // error information. if any problem happens, this variable will go to 1;
	std::cout << "Initializing Agilent34401A DMM...\n";

	this->pna = ibdev(0, PRIMARY_ADDR_OF_DMM, NO_SECONDARY_ADDR, TIMEOUT, EOTMODE, EOSMODE); //ibdev gives back ud! Unique iDentifier based on info given in

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
	//printf("initializing Agilent34401A DMM parameters\n");
	// sourcing voltage
	GPIBWrite(pna, "*RST");



	// sourcing voltage
	//GPIBWrite(pna, ":SOUR:FUNC VOLT");
	//// sensing mode DC current
	//GPIBWrite(pna, ":SENS:FUNC 'CURR:DC'");


	//       // voltage source range in auto
	//GPIBWrite(pna,":SOUR:VOLT:RANGE:AUTO ON");
	//       // current sensing range in auto
	//GPIBWrite(pna,":SENS:CURR:RANGE:AUTO ON");
	// Auto ranging seemed to cause exploding of devices.....???
	// Failing to auto range may be giving us really fucking weird measurements from keithley

	//nplc = 1;

	//// current protection (compliance) set at 1A
	//GPIBWrite(pna, ":SENS:CURR:PROT 1.0");
	//// fixed voltage mode
	//GPIBWrite(pna, ":SOUR:VOLT:MODE FIXED");

	//char delays[20];
	//sprintf(delays, ":SOUR:DEL %f", 0.02);
	//GPIBWrite(pna, delays);

	//// terminals connected on the front
	//GPIBWrite(pna, ":ROUT:TERM FRONT");
	//// 2w measurement
	//GPIBWrite(pna, ":SYST:RSEN OFF");
	//// setting output format to V I
	//GPIBWrite(pna, ":FORM:ELEM VOLT,CURR");
	//// checking possible error of GPIB and print stuff if there are an
	//// we also print the code of the error
	//// voltage source range in auto
	//GPIBWrite(pna, ":SOUR:VOLT:RANGE 21");
	//// current sensing range in auto
	//GPIBWrite(pna, ":SENS:CURR:RANGE 30E-3");


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
	else printf("Initialization of Agilent34401A DMM was successful.\n");
	return 0;

}





int DMM::GPIBWrite(int ud, char* cmd){
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

int DMM::GPIBRead(int ud, char *message){
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
