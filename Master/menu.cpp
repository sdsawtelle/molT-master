#include "menu.h"
#include "K2400.h"
#include "SPA4156B.h"
#include "SPA4145B.h"
#include "Switchbox.h"

#include <fstream>
#include <iostream>
#include <windows.h>
#include "ni488.h"
#include <cstdlib>
#include "engine.h"

menu::menu()
{
	//ctor
}

menu::~menu()
{
	//dtor
}

void menu::menu_start(K2400 keithley, SPA4156B SPA, Switchbox switchbox){

	// get the port specs for all the devices on chip
	switchbox.getAllPorts(); // reads in all the pad specifications, reformats them for switchbox, and creates output file streams

	emailAddress = "sdsawtelle@gmail.com";
	std::cout << "What is the ID for this chip?\n";
	std::cin >> chip_ID;
	std::cout << "What is the Feed-thru ID for this Group?\n";
	std::cin >> ft_ID;
	switchbox.ft_ID = ft_ID;

	int choice = 0;
	while (1){
		std::cout << "\n ===========-------  MENU  -------============= \n\n";
		std::cout << "   (1)  SHORT ALL devices (S and D pads) to Shorting Cap (04)\n";
		std::cout << "   (2)  OPEN ALL possible routes (0101:1028) \n\n" ;
		std::cout << "   (3)  Perform NANOWIRE SAMPLING YIELD measurements on a group of devices\n";

		//std::cout << "   (4)  Execute IV SWEEP OF THE GATELINE for the first listed device \n\n";	
		//std::cout << "   (5)  Perform GATE LEAKAGE SAMPLING YIELD measurements on a group of devices\n";
		//std::cout << "   (6)  Perform GATE LEAKAGE SWEEPS (IG) measurements on a group of devices\n";

		std::cout << "   (7)  Take IV SWEEPS of a group of devices\n";
		//std::cout << "   (8)  Take GATED IV SWEEPS (OPTIONALLY WITH LEAKAGE) of a group of devices \n";

		//std::cout << "   (9)  Connect a device for IETS (OPTIONALLY GATED)\n";
		//std::cout << "   (10)  Hold DEP VOLTAGE across all devices\n\n";

		std::cout << "   (11)  Execute EM ON A GROUP of devices \n";
		std::cout << "	 (12)  Execute CONSTANT VOLTAGE EM (Rate vs. Power) on a group of devices. \n\n";
		//std::cout << "   (13)  Execute EM AND IVs on a group of devices\n\n";

		//std::cout << "   (14)  Execute EM AND SELF-BREAK MONITORING on a group of devices\n";
		//std::cout << "   (15)  Execute SELF-BREAK MONITORING of a group of devices\n\n";
		//std::cout << "   (16)  Execute EM AND SELF-BREAK MONITORING (OPTIONALLY GATED) and switch devices after full breaking \n\n";

		std::cout << "   (17)  EXIT \n\n";
		std::cout << " ============================ Please enter a command:  ";
		std::cin >> choice;

		int ymflag = 0; // is this a yield monitoring measurement? affects how switchbox.getPorts creates output files streams
		FILE* outputs[36]; // this array will hold all the file pointers to the output files being written

		switch (choice){
		case 1:
					//SHORT ALL output ports(01 to 16) to Shorting Cap(04)
					switchbox.closeAll();
					break;
		case 2:
					//OPEN ALL channels from Shorting Cap (04) 
					switchbox.openAll();
					break;
		case 3:{
				   // Perform NANOWIRE YIELD measurements on a group of devices
				   ymflag = 1; // single output file
				   switchbox.getPorts(ymflag, outputs);
				   SPA.setParams(3);
				   executeYield(SPA, switchbox, outputs);
				   closeFiles(ymflag, switchbox.ndev, outputs, switchbox);
				   break; }
		case 4:{
				   // Take SWEEP OF GATELINE of first device\n";
				   int outputfileflag = 1; //we just want one file, not one for each device
				   switchbox.getPorts(outputfileflag, outputs);
				   SPA.setParams(2);
				   executeGatelineSweep(SPA, switchbox, outputs);
				   closeFiles(outputfileflag, switchbox.ndev, outputs, switchbox);
				   break;}
		case 5:{
				   // Perform GATE LEAKAGE SAMPLING YIELD measurements on a group of devices
				   ymflag = 1; // single output file
				   switchbox.getPorts(ymflag, outputs);  //now ymflag=1 will tell switchbox to just open one output file
				   SPA.setParams(3);
				   executeLeakageYield(SPA, switchbox, outputs);
				   closeFiles(ymflag, switchbox.ndev, outputs, switchbox);
				   break; }
		case 6:{
					// Take GATE LEAKAGE SWEEPS (IG) on a group of devices\n";
					switchbox.getPorts(ymflag, outputs);
					SPA.setParams(2);
					executeLeakageSweep(SPA, switchbox, outputs);
					closeFiles(ymflag, switchbox.ndev, outputs, switchbox);
					break; }
		case 7:{
					// Take IV SWEEPS on a group of devices\n";
					switchbox.getPorts(ymflag, outputs);
					SPA.setParams(2);
					executeIV(SPA, switchbox, outputs);
					closeFiles(ymflag, switchbox.ndev, outputs, switchbox);
					break; }
		case 8:{
					// Take GATED IV SWEEPS (OPTIONALLY WITH LEAKAGE) of a group of devices \n\n";
					switchbox.getPorts(ymflag, outputs);
					int leakageFlag = 1;
					std::cout << "Enter 1 for leakage data and 2 for NO leakage data.\n";
					std::cin >> leakageFlag;
					SPA.setParams(3 + leakageFlag);
					executeGatedIV(leakageFlag, SPA, switchbox, outputs);
					closeFiles(ymflag, switchbox.ndev, outputs, switchbox);
					break; }
		case 9:{
					// Connect a device for IETS (OPTIONALLY GATED). \n";
					ymflag = 2;
					switchbox.getPorts(ymflag, outputs);
					bool gateFlag = false;
					std::cout << "Would you like to hold a gate voltage during IETS? (1 for yes, 0 for no)\n";
					std::cin >> gateFlag;
					executeIETS(keithley, switchbox, outputs, gateFlag);
					closeFiles(ymflag, switchbox.ndev, outputs, switchbox);
					break; }
		case 10:{
					// DEP on a group of devices
					switchbox.getPorts(ymflag, outputs);
					keithleyDEP(keithley, switchbox, outputs);
					break; }
		case 11:{	
				   // Execute EM ON A GROUP of devices\n";
				   int emType = 0; // normal EM
				   keithley.setParamsEM_fromfile(emType); // sets the EM parameters
				   switchbox.getPorts(ymflag, outputs); // reads in all the pad specs and mark devices to be tested
				   keithley.setParamsEM_fromfile(emType); // sets the EM parameters
				   keithley.initializeEM(emType); //set ranges, integration time, delay and turn OUTPUT ON at 0 V
				   executeEM(emType, keithley, SPA, switchbox, outputs); // EM on each marked device 
				   closeFiles(ymflag, switchbox.ndev, outputs, switchbox); // close all the output files
				   //test();
				   break;}
		case 12:{
					// Execute CONSTANT-VOLTAGE EM (RATE VS. POWER) ON A GROUP of devices\n";
					int emType = 1; // constant-voltage EM (rate vs. power)
					switchbox.getPorts(ymflag, outputs); // reads in all the pad specs and mark devices to be tested
					keithley.setParamsEM_fromfile(emType); // sets the EM parameters
					keithley.initializeEM(emType); //set ranges, integration time, delay and turn OUTPUT ON at 0 V
					executeEM(emType, keithley, SPA, switchbox, outputs); // EM on each marked device 
					closeFiles(ymflag, switchbox.ndev, outputs, switchbox); // close all the output files
					break; }
		case 13:{
					// Execute EM AND IVs on a group of devices
					int emType = 0;
					executeEMandIV(emType, SPA, switchbox, keithley);
					break; }
		case 14:{
				   //Execute EM AND SELF-BREAK MONITORING on a group of devices\n";
				   int emType = 0; // normal EM
				   switchbox.getPorts(ymflag, outputs); // reads in all the pad specs and mark devices to be tested
				   keithley.setParamsEM_fromfile(emType); // sets the EM parameters
				   keithley.initializeEM(emType); //set ranges, integration time, delay and turn OUTPUT ON at 0 V
				   executeEM(emType, keithley, SPA, switchbox, outputs); // EM on each marked device 
				   executeMonitor(SPA, switchbox, outputs); //executes self-breaking monitoring until the user pressed 'Alt' key
				   closeFiles(ymflag, switchbox.ndev, outputs, switchbox);
				   break; }
		case 15:{
				   // Execute SELF-BREAK MONITORING of a group of devices\n\n";
				   switchbox.getPorts(ymflag, outputs);
				   SPA.setParams(1); // sets the sampling parameters for monitoring the devices (some user-input)
				   executeMonitor(SPA, switchbox, outputs); //executes self-breaking monitoring until the user pressed 'Alt' key
				   closeFiles(ymflag, switchbox.ndev, outputs, switchbox);
				   break; }
		case 16:{
					//Execute EM AND SELF-BREAK MONITORING (OPTIONALLY GATED) and switch devices after full breaking. \n\n";
					int emType = 0;
					executeEMandMonitor(emType, keithley, SPA, switchbox, outputs); // performs sequential EM and then self-break monitoring on each device in turn - it switches to the next device once the DUT has self-broken to a very high resistance 
					closeFiles(ymflag, switchbox.ndev, outputs, switchbox);
					//send email to let us know that all devices have been used and a new chip is needed!
					SendMail(emailAddress);
					break; }
		case 17:
			goto EXIT;
			break;

		default:
			break;
		}
	}
EXIT:;
}






void menu::executeYield(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]){

	// Get the time and prep for printing it
	std::chrono::time_point<std::chrono::system_clock> time_now = std::chrono::system_clock::now();
	std::time_t time_now_t = std::chrono::system_clock::to_time_t(time_now);
	std::tm now_tm = *std::localtime(&time_now_t);
	char buf[512];
	std::strftime(buf, 512, "%Y%m%d_%H_%M_%S, ", &now_tm);
	std::cout << buf << std::endl;

	// get some comments demarcating this particular round of sampling and write it to the YM output file
	std::string comments;
	std::cout << "What are user comments for this yield check?\n";
	std::getline(std::cin, comments);
	fprintf(outputs[0], "UserComments---%s \n\n", comments.c_str());
	fprintf(outputs[0], "Device Port Specs, Resistance at 1 mV (Ohms)\n", comments.c_str());
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			// write device identifier to the output text file, to be followed by the measurement recording
			FILE* sb_output = outputs[0]; // set the output file stream to yield monitor file
			fprintf(sb_output, buf);
			fprintf(sb_output, "%s_%s,", ft_ID.c_str(), switchbox.portSpecs[3][devnum].c_str());
			fflush(sb_output);
			std::cout << "Device ID# " << switchbox.portSpecs[3][devnum].c_str() << " R @ 1mV = ";
			switchbox.closeChan(devnum, "SPA"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
			Sleep(100);
			SPA.yieldSingle(devnum, outputs); // similar to sampleSingle but always writes to output[0]
			Sleep(100);
			switchbox.openChan(devnum, "SPA", 1); // get back to the state of all output ports shorted to shorting cap ground
			Sleep(100);
		}
	}
}

float menu::executeGapYield(int devnum, SPA4156B SPA, Switchbox switchbox){
	//this is a function that samples a device at 100mV after some initial electromigration
	switchbox.closeChan(devnum, "SPA"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
	Sleep(50);
	float resistance = SPA.yieldGapSingle(); //Samples at 100mV, returns resistance
	Sleep(50);
	switchbox.openChan(devnum, "SPA", 1); // get back to the state of all output ports shorted to shorting cap ground
	std::cout << "Actual Resistance of Device " << switchbox.portSpecs[3][devnum].c_str() << " at 100 mV is " << resistance << " Ohms.\n";
	return resistance;
}

void menu::executeGatelineSweep(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]){
	switchbox.openAll();
	std::cout << "...Assuming all devices are on a single gateline...\n";
	int devnum = 0;
	int gateFlag = 0; //
	switchbox.closeChan(devnum, "Gateline"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
	Sleep(250);
	//SPA.yieldSingle(devnum, outputs); // similar to sampleSingle but always writes to output[0]
	SPA.sweepSingle(devnum, outputs, gateFlag); // writes IV data to output file. 
	Sleep(250);
	switchbox.openChan(devnum, "Gateline", 0); // do NOT go back to all output ports shorted to keithley ground
	Sleep(250);
	switchbox.closeAll();
}

void menu::executeLeakageYield(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]){
	// get some comments demarcating this particular round of sampling and write it to the YM output file
	std::string comments;
	std::cout << "What are user comments for this leakage yield test? (no spaces) \n";
	std::cin >> comments;
	fprintf(outputs[0], "UserComments---%s \n\n", comments.c_str());

	switchbox.openAll(); //in case some other grounded device has a leakage path
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y" && switchbox.portSpecs[5][devnum] != "n"){
			// write device identifier to the output text file, to be followed by the measurement recording
			FILE* sb_output = outputs[0]; // set the output file stream to yield monitor file
			fprintf(sb_output, "Device ID# %s Ig = ", switchbox.portSpecs[3][devnum].c_str());
			fflush(sb_output);
			std::cout << "Device ID# " << switchbox.portSpecs[3][devnum].c_str() << " Ig = ";
			switchbox.closeChan(devnum, "Leakage"); // all channels except DUT remain shorted to bias port, DUT (one of the S/D and one of the gate pads) is connected to relevant inputs
			SPA.yieldSingle(devnum, outputs); // similar to sampleSingle but always writes to output[0]
			switchbox.openChan(devnum, "Leakage", 0); // do NOT go back to the state of all output ports shorted to keithley ground
		}
	}
	switchbox.closeAll(); //get back to being all shorted to ground 
}

void menu::executeLeakageSweep(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]){

	switchbox.openAll(); //in case some other grounded device has a leakage path
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y" && switchbox.portSpecs[5][devnum] != "n"){ // make sure its marked _and_ a gated device
			switchbox.closeChan(devnum, "Leakage"); // all channels except DUT and its common drain group remain shorted to bias port, DUT S (first pad) is connected to SMU1 and gatepad1 is connected to SMU2 on SPA (input ports 01,02)
			int gateFlag = 0; // to call SPA.sweepSingle with the correct GPIB read commands
			SPA.sweepSingle(devnum, outputs, gateFlag); // writes IV data to output file.
			switchbox.openChan(devnum, "Leakage", 0); // do NOT go back to all devices grounded to shorting cap
			Sleep(200);
		}
	}
	switchbox.closeAll(); //get back to being all shorted to ground 
}


void menu::executeIV(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]){
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			switchbox.closeChan(devnum, "SPA"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
			int gateFlag = 0; // to call SPA.sweepSingle with the correct GPIB read commands
			SPA.sweepSingle(devnum, outputs, gateFlag); // writes IV data to output file. 
			switchbox.openChan(devnum, "SPA", 1); // get back to the state of all output ports shorted to switchbox ground
			Sleep(1000);
			// SPA.ShowGraph("Null");
		}
	}
}

void menu::executeGatedIV(int leakageFlag, SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]){
	switchbox.openAll(); //in case some other grounded device has a leakage path
	for (int devnum = 0; devnum < switchbox.ndev; devnum++ && switchbox.portSpecs[5][devnum] != "n"){
		if (switchbox.portSpecs[4][devnum] == "y"){
			switchbox.closeChan(devnum, "SPA"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
			switchbox.closeChan(devnum, "SPAgate"); // close both gate pads down to SMU3 of SPA
			SPA.sweepSingle(devnum, outputs, leakageFlag); // writes IV data to output file. 
			switchbox.openChan(devnum, "SPA", 0); // get back to the state of all output ports shorted to switchbox ground
			switchbox.openChan(devnum, "SPAgate", 0); // close both gate pads down to SMU3 of SPA
			Sleep(1000);
		}
	}
	switchbox.closeAll(); //get back to being all shorted to ground 
}


void menu::executeEM(int emType, K2400 keithley, SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]){
	// get a filename and open a file that will be used to write the summary info to
	std::string file_summary;
	std::cout << "What is the filename stem to use for the EM Outcome Summary? \n";
	std::cin >> file_summary;
	char filebuffer[1024] = "";
	sprintf(filebuffer, "%s_%s_EMSummary.txt", file_summary.c_str(), chip_ID.c_str());
	FILE *summary = fopen(filebuffer, "w+");
	if (summary == NULL) { // check that fopen worked correctly
		std::cout << "fopen has failed for the output file " << filebuffer << "\n";
	}

	typedef std::chrono::high_resolution_clock Clock;
	Clock::time_point t0;
	Clock::time_point t1;
	typedef std::chrono::minutes minutes;
	minutes Elapsed;

	////////////////////// Do EM On All Devices //////////////////////////
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			switchbox.closeChan(devnum, "keithley"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
			t0 = Clock::now();
			switchbox.exitSummary[0][devnum] = keithley.WrapEM(emType, devnum, outputs, switchbox); // writes IV data to output file. keithley will set his output to 0V after finishing EM run
			t1 = Clock::now();
			Elapsed = std::chrono::duration_cast<minutes>(t1 - t0);
			std::cout << "Device ID# " << switchbox.portSpecs[3][devnum].c_str() << " has completed active EM! \n";
			std::cout << "Elapsed time of electromigration was " << Elapsed.count() << " minutes. \n";
			std::cout << "Exit voltage of the electromigration was " << switchbox.exitSummary[0][devnum] << " V.\n";
			switchbox.openChan(devnum, "keithley", 1); // get back to all output ports shorted to keithley ground
			switchbox.exitSummary[1][devnum] = executeGapYield(devnum, SPA, switchbox); // prints out R sampled at 100mV 
			switchbox.exitSummary[2][devnum] = Elapsed.count();
			std::cout << "---------------------------------------------------\n";
		}
	}

	////////////////////// Write the Summary File //////////////////////////
	fprintf(summary, "Chip ID, Device Ports, Run Type, Temperature (K), EM exit V (V), R @ 100mV after EM (ohms), Elapsed Time of EM (min))\n"); // header
	char buffer[1024];
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			sprintf(buffer, "%s, %s_%s, EM, %.3f, %.3f, %.3f, %.1f\n", chip_ID.c_str(), ft_ID.c_str(), switchbox.portSpecs[3][devnum].c_str(), keithley.temperature, switchbox.exitSummary[0][devnum], switchbox.exitSummary[1][devnum], switchbox.exitSummary[2][devnum]);
			fprintf(summary, buffer);
		}
	}
	fclose(summary);

	//SendMail(emailAddress);

}

void menu::executeEMandIV(int emType, SPA4156B SPA, Switchbox switchbox, K2400 keithley){

	// get the port specs file for devices to measure - do this once!
	std::string file_in;
	int fileExists = 0;
	while (!fileExists){
		std::cout << "What is the input file containing pad specs of devices to measure?\n";
		std::cin >> file_in;
		file_in += ".txt";
		fileExists = switchbox.checkFileExists(file_in);
	}

	// let user set EM parameters for keithley
	std::cout << "\n\n>>>> Please set EM parameters...\n\n";
	int ymflag = 0; // is this a yield monitoring measurement? affects how switchbox.getPorts creates output files streams
	FILE* outputsEM[36]; // this array will hold all the file pointers to the output files being written for EM
	switchbox.getPorts(ymflag, outputsEM, file_in);


	////let user set the KEITHLEy sweep breaking parameters
	//std::cout << "\n\n>>>> Please set Keithley Sweep breaking parameters...\n\n";
	//FILE* outputsKS[36]; // this array will hold all the file pointers to the output files being written for SWeep breaking
	//switchbox.getPorts(ymflag, outputsKS, file_in);
	//int useEMV = 0;
	//keithley.setParamsSweep(&useEMV);
	//float Vstart_KS = -1;


	//let user set the SPA IV sweep parameters
	std::cout << "\n\n>>>> Please set IV Sweep parameters...\n";
	FILE* outputsIV[36]; // this array will hold all the file pointers to the output files being written for gated IV sweeps
	switchbox.getPorts(ymflag, outputsIV, file_in);
	float Vstart_IV, Vstop_IV, Vstep_IV;
	SPA.setIVParamsLater(&Vstart_IV, &Vstop_IV, &Vstep_IV);  //asks user for IV parameters and stores them to set up SPA later.



	// get a filename and open a file that will be used to write the summary info to
	std::string file_summary;
	std::cout << "What is the filename stem to use for the EM Outcome Summary?\n";
	std::cin >> file_summary;
	char filebuffer[1024] = "";
	sprintf(filebuffer, "%s_%s_EMSummary.txt", file_summary.c_str(), chip_ID.c_str());
	FILE *summary = fopen(filebuffer, "w+");
	if (summary == NULL) { // check that fopen worked correctly
		std::cout << "fopen has failed for the output file " << filebuffer << "\n";
	}


	//set up time keeping apparatus
	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::minutes minutes;
	Clock::time_point t0;
	Clock::time_point t1;
	minutes Elapsed;



	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			//
			// Perform EM on the device...
			//
			keithley.initializeEM(0);
			switchbox.closeChan(devnum, "keithley"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
			t0 = Clock::now();
			switchbox.exitSummary[0][devnum] = keithley.WrapEM(emType, devnum, outputsEM, switchbox); // keithley.emSingle IV data to output file. keithley will set his output to 0V after finishing EM run and return the exit voltage
			t1 = Clock::now();
			switchbox.openChan(devnum, "keithley", 1); // get back to the state of all output ports shorted to keithley ground
			Elapsed = std::chrono::duration_cast<minutes>(t1 - t0);
			switchbox.exitSummary[2][devnum] = Elapsed.count(); //save the total time to EM in the summary array
			std::cout << "---------------------------------------------------\n";
			std::cout << "Device ID# " << switchbox.portSpecs[3][devnum].c_str() << " has completed active EM! \n";
			std::cout << "Elapsed time of electromigration was " << Elapsed.count() << " minutes. \n";
			std::cout << "Exit voltage of the electromigration was " << switchbox.exitSummary[0][devnum] << " V.\n";
			switchbox.exitSummary[1][devnum] = executeGapYield(devnum, SPA, switchbox); // will print out the actual resistance sampled at 100mV 
			std::cout << "---------------------------------------------------\n";
		}
	}

	// close EM record files
	closeFiles(ymflag, switchbox.ndev, outputsEM, switchbox);
	// print out summary of EM to Summary.txt
	fprintf(summary, "Chip ID, Device Ports, Run Type, Temperature (K), exit V (V), R @ 100mV after Run (ohms), Elapsed Time of Run (min))\n"); // header
	char buffer[1024];
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			// print out the EM summary
			sprintf(buffer, "%s, %s_%s, EM, %.3f, %.3f, %.3f, %.1f\n", chip_ID.c_str(), ft_ID.c_str(), switchbox.portSpecs[3][devnum].c_str(), keithley.temperature, switchbox.exitSummary[0][devnum], switchbox.exitSummary[1][devnum], switchbox.exitSummary[2][devnum]);
			fprintf(summary, buffer);
		}
	}
	fclose(summary);

	//
	//Perform IV's on the Device...
	//
	SPA.configSweep(Vstart_IV, Vstop_IV, Vstep_IV);
	executeIV(SPA, switchbox, outputsIV);
	closeFiles(ymflag, switchbox.ndev, outputsIV, switchbox);
	SendMail(emailAddress);	//send mail so person knows everything is done

}


void menu::keithleyDEP(K2400 keithley, Switchbox switchbox, FILE* outputs[36]){

	float Vsample, holdTime;
	std::cout << "What is the voltage to hold for DEP? (in V) \n";
	std::cin >> Vsample;
	std::cout << "What is the length of time to hold voltage for DEP? (in Minutes) \n";
	std::cin >> holdTime;

	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			switchbox.closeChan(devnum, "keithley"); // connect all the devices at once
		}
	}

	keithley.holdDEPVoltage(Vsample, holdTime);

	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			switchbox.openChan(devnum, "keithley", 1); // get back to the state of all output ports shorted to keithley ground
		}
	}
}

void menu::holdGateVoltage(K2400 keithley, Switchbox switchbox, FILE* outputs[36], int devnum){
	switchbox.closeChan(devnum, "HoldGate"); //this will NOT first unground all the other devices sharing common drain see Switchbox::closeChan
	std::cout << "Device ID# " << switchbox.portSpecs[3][devnum].c_str() << " has had its gateline connected to Keithley.\n";
	int choice = 1;
	while (choice){
		keithley.holdGateVoltage();
		std::cout << "Hold another gate voltage? --- 1 for 'yes' , 0 for 'no'. \n";
		std::cin >> choice;
	}
}

void menu::executeIETS(K2400 keithley, Switchbox switchbox, FILE* outputs[36], bool gateFlag){
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			switchbox.closeChan(devnum, "IETS"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
			std::cout << "Device has been connected to IETS measurement on input ports 05 and 06.\n";
			if (gateFlag){
				holdGateVoltage(keithley, switchbox, outputs, devnum);
				switchbox.openChan(devnum, "HoldGate", 0);
			}
			std::cout << "=======================================================================\n";
			std::cout << "Press the 'F12' key to disconnect the device...\n";
			std::cout << "=======================================================================\n";
			while (!GetAsyncKeyState(VK_F12)){
			}
			switchbox.openChan(devnum, "IETS", 1);
		}
	}
}


void menu::executeEMandMonitor(int emType, K2400 keithley, SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]){

	// get the port specs file for devices to measure - do this once!
	std::string file_in;
	int fileExists = 0;
	while (!fileExists){
		std::cout << "What is the input file containing pad specs of devices to measure?\n";
		std::cin >> file_in;
		file_in += ".txt";
		fileExists = switchbox.checkFileExists(file_in);
	}

	// let user set EM parameters for keithley
	std::cout << "\n\n>>>> Please set EM parameters...\n\n";
	int ymflag = 0; // is this a yield monitoring measurement? affects how switchbox.getPorts creates output files streams
	FILE* outputsEM[36]; // this array will hold all the file pointers to the output files being written for EM
	switchbox.getPorts(ymflag, outputsEM, file_in);

	//let user set the Self-Break Monitoring parameters
	std::cout << "\n\n>>>> Please set Self-Break Monitoring parameters...\n";
	FILE* outputsSB[36]; // this array will hold all the file pointers to the output files being written for gated IV sweeps
	switchbox.getPorts(ymflag, outputsSB, file_in);
	float voltage, Rdead, Vgate;
	Vgate = 0;
	SPA.setParams(1); // sets the sampling parameters for monitoring the devices after EM (some user-input)
	int gatedSB;
	std::cout << "Self-Breaking: What voltage are monitoring at? (in V)\n";
	std::cin >> voltage;
	std::cout << "Self-Breaking: What Resistance is considered a dead device? (in Ohms)\n";
	std::cin >> Rdead;
	std::cout << "Is the self-breaking Gated? (0 for No, 1 for YES)\n";
	std::cin >> gatedSB;
	if (gatedSB){
		std::cout << "What is the voltage to hold on the gate? (in V) \n";
		std::cin >> Vgate;
	}


	// get a filename and open a file that will be used to write the summary info to
	std::string file_summary;
	std::cout << "What is the filename stem to use for the EM Outcome Summary?\n";
	std::cin >> file_summary;
	char filebuffer[1024] = "";
	sprintf(filebuffer, "%s_%s_EMSummary.txt", file_summary.c_str(), chip_ID.c_str());
	FILE *summary = fopen(filebuffer, "w+");
	if (summary == NULL) { // check that fopen worked correctly
		std::cout << "fopen has failed for the output file " << filebuffer << "\n";
	}

	//set up time keeping apparatus
	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::minutes minutes;
	Clock::time_point t0;
	Clock::time_point t1;
	minutes Elapsed;

	Sleep(500);
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){	
			//
			// Perform EM on the device...
			//
			SPA.configSample(0.1); // configure SPA to do it's one point sampling at 100 mV check after EM.
			keithley.initializeEM(0);
			switchbox.closeChan(devnum, "keithley"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
			t0 = Clock::now();
			switchbox.exitSummary[0][devnum] = keithley.WrapEM(emType, devnum, outputsEM, switchbox); // keithley.emSingle IV data to output file. keithley will set his output to 0V after finishing EM run and return the exit voltage
			t1 = Clock::now();
			switchbox.openChan(devnum, "keithley", 1); // get back to the state of all output ports shorted to keithley ground
			Elapsed = std::chrono::duration_cast<minutes>(t1 - t0);
			switchbox.exitSummary[2][devnum] = Elapsed.count(); //save the total time to EM in the summary array
			std::cout << "---------------------------------------------------\n";
			std::cout << "Device ID# " << switchbox.portSpecs[3][devnum].c_str() << " has completed active EM! \n";
			std::cout << "Elapsed time of electromigration was " << Elapsed.count() << " minutes. \n";
			std::cout << "Exit voltage of the electromigration was " << switchbox.exitSummary[0][devnum] << " V.\n";
			switchbox.exitSummary[1][devnum] = executeGapYield(devnum, SPA, switchbox); // will print out the actual resistance sampled at 100mV 
			std::cout << "---------------------------------------------------\n";
			// 
			// Perform Self-Break Monitoring
			//
			FILE* sb_output = outputsSB[devnum];
			std::cout << "break 1\n";
			fprintf(sb_output, "Gating Voltage (V) ; %.3f\n", Vgate);
			std::cout << "break 2\n";
			fprintf(sb_output, "voltage,current\n");
			std::cout << "break 3\n";
			fflush(sb_output);
			std::cout << "break 4\n";
			SPA.constconfigSample(); // configure SPA to do it's long sampling SB monitor measurements.
			std::cout << "break 5\n";
			switchbox.closeChan(devnum, "SPA"); // all channels except DUT remain shorted to bias port, DUT is connected to relevant inputs
			std::cout << "break 6\n";
			if (gatedSB){
				keithley.setGateVoltage(Vgate);
				switchbox.closeChan(devnum, "HoldGate"); //this will NOT first unground all the other devices sharing common drain see Switchbox::closeChan
			}
			std::cout << "=======================================================================\n";
			std::cout << "Self-break monitoring will continue until the 'F12' key is pressed...\n";
			std::cout << "=======================================================================\n";
			int deadFlag = 0;
			int cycle = 0;
			while (!GetAsyncKeyState(VK_F12) && deadFlag==0){
				deadFlag=SPA.constsampleSingle(devnum, outputsSB, cycle, Rdead, voltage);
				cycle++;
			}
			switchbox.openChan(devnum, "HoldGate",1);
			switchbox.openChan(devnum, "SPA", 1); // get back to the state of all output ports shorted to keithley ground
		}
	}

	// close EM record files
	closeFiles(ymflag, switchbox.ndev, outputsEM, switchbox);
	// close SB record files
	closeFiles(ymflag, switchbox.ndev, outputsSB, switchbox);
	// print out summary of EM to Summary.txt
	fprintf(summary, "Chip ID, Device Ports, Run Type, Temperature (K), exit V (V), R @ 100mV after Run (ohms), Elapsed Time of Run (min))\n"); // header
	char buffer[1024];
	for (int devnum = 0; devnum < switchbox.ndev; devnum++){
		if (switchbox.portSpecs[4][devnum] == "y"){
			// print out the EM summary
			sprintf(buffer, "%s, %s_%s, EM, %.3f, %.3f, %.3f, %.1f\n", chip_ID.c_str(), ft_ID.c_str(), switchbox.portSpecs[3][devnum].c_str(), keithley.temperature, switchbox.exitSummary[0][devnum], switchbox.exitSummary[1][devnum], switchbox.exitSummary[2][devnum]);
			fprintf(summary, buffer);
		}
	}
	fclose(summary);

	switchbox.closeAll(); // this will first open all channels

}

void menu::executeMonitor(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]){
	std::cout << "=======================================================================\n";
	std::cout << "Self-break monitoring will continue until the 'F12' key is pressed...\n";
	std::cout << "=======================================================================\n";
	int cycle = 0;

	// check if its just a single device we're monitoring
	int count = 0;
	int singlestore = 0;
	for (int counter = 0; counter < switchbox.ndev; counter++){
		if (switchbox.portSpecs[4][counter] == "y"){
			count = count + 1;
			switchbox.openChan(counter, "ShortCap",0); // all devices except the group of DUTs remain shorted to shorting cap
			singlestore = counter; // singlestore will be the devnum for the single DUT in the event that there is only one
		}
	}


	int deadFlag = 0;

	if (count == 1){ // if going to be monitoring only one device
		SPA.constconfigSample();
		switchbox.closeChanMonitor(singlestore, "SPA");
		while (!deadFlag && !GetAsyncKeyState(VK_F12)){
			deadFlag = SPA.constsampleSingle(singlestore, outputs, cycle, 0.005, 100000); // 
		}
	}
	else{
		while (!GetAsyncKeyState(VK_F12)){
			for (int devnum = 0; devnum < switchbox.ndev; devnum++){
				if (switchbox.portSpecs[4][devnum] == "y"){
					switchbox.closeChanMonitor(devnum, "SPA"); // assumes all channels in the group of interest have been opened away from shorting cap, this just closes the channel to measurement instrument
					deadFlag = SPA.sampleSingle(devnum, outputs, cycle);
					if (deadFlag == 1){
						switchbox.portSpecs[4][devnum] = "n";
					}
					switchbox.openChanMonitor(devnum, "SPA"); // assumes all channels in the group of interest should remain open away from shorting cap, this just opens the channel from the measurement instrument
				}
			}
			cycle++;
		}
	}

	switchbox.closeAll(); // this will first open all channels
}

void menu::executeGatedMonitor(SPA4156B SPA, K2400 keithley, Switchbox switchbox, FILE* outputs[36]){
	
	switchbox.openAll();

	// check if its just a single device we're monitoring
	int count = 0;
	int singlestore = 0;
	for (int counter = 0; counter < switchbox.ndev; counter++){
		if (switchbox.portSpecs[4][counter] == "y"){
			count = count + 1;
			switchbox.openChan(counter, "ShortCap", 0); // all devices except the group of DUTs remain shorted to shorting cap
			singlestore = counter; // singlestore will be the devnum for the single DUT in the event that there is only one
		}
	}
	
	float Vgate;
	std::cout << "What is the voltage to hold on the gate? (in V) \n";
	std::cin >> Vgate;
	keithley.setGateVoltage(Vgate);

	std::cout << "=======================================================================\n";
	std::cout << "Self-break monitoring will continue until the 'F12' key is pressed...\n";
	std::cout << "=======================================================================\n";
	int cycle = 0;

	if (count == 1){ // if going to be monitoring only one device
		switchbox.closeChan(singlestore, "HoldGate"); //this will NOT first unground all the other devices sharing common drain see Switchbox::closeChan
		SPA.constconfigSample();
		switchbox.closeChanMonitor(singlestore, "SPA"); // assumes all channels in the group of interest have been opened away from shorting cap, this just closes the channel to measurement instrument
	}


	while (!GetAsyncKeyState(VK_F12)){

		// if its just one device rather than a group, use the constant bias sampling approach
		if (count == 1){
			SPA.constsampleSingle(singlestore, outputs, cycle, 0.005, 100000); // 
		}

		// if its a group of device we have to just have a single sampling approach and switch between devices every time point
		else{
			for (int devnum = 0; devnum < switchbox.ndev; devnum++){
				if (switchbox.portSpecs[4][devnum] == "y"){
					switchbox.closeChan(devnum, "HoldGate"); //this will NOT first unground all the other devices sharing common drain see Switchbox::closeChan
					switchbox.closeChanMonitor(devnum, "SPA"); // assumes all channels in the group of interest have been opened away from shorting cap, this just closes the channel to measurement instrument
					int deadFlag = 0;
					deadFlag = SPA.sampleSingle(devnum, outputs, cycle);
					if (deadFlag == 1){
						switchbox.portSpecs[4][devnum] = "n";
					}
					switchbox.openChanMonitor(devnum, "SPA"); // assumes all channels in the group of interest should remain open away from shorting cap, this just opens the channel from the measurement instrument
				}
			}
		}
		cycle++;
	}

	switchbox.closeAll(); // this will first open all channels

	std::cout << "=======================================================================\n";
	std::cout << "PLEASE TURN OFF KEITHLEY OUTPUT...\n";
	std::cout << "=======================================================================\n";
}


void menu::closeFiles(int ymflag, int ndev, FILE* outputs[36], Switchbox switchbox){
	if (ymflag == 1){ fclose(outputs[0]);}
	else if (ymflag == 0){
		for (int devnum = 0; devnum < ndev; devnum++){
			if (switchbox.portSpecs[4][devnum] == "y"){
				fclose(outputs[devnum]);
			}
		}
	}
}

void menu::SendMail(std::string emailAddress){
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
	program += "Come check CryoPC! \n";
	program += "END_OF_MESSAGE\n";
	program += "Net::SMTP.start(\'smtp.gmail.com\', 587, \'gmail.com\', \n";
	program += "FROM_EMAIL, PASSWORD, :plain) do |smtp| \n";
	program += "smtp.send_message msgstr, FROM_EMAIL, TO_EMAIL\n";
	program += "end";
	FILE* mailing = fopen("mail.rb", "w+");
	fprintf(mailing, program.c_str());
	fclose(mailing);
	system("Ruby  mail.rb");
}

