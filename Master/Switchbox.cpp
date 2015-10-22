#include "Switchbox.h"
#include <iostream>
#include <windows.h>
#include "ni488.h"
#include <cstdlib>


Switchbox::Switchbox(){
	configure();
	// initialize the array holding the channel specifications and EM exit voltages
	for (int i = 0; i < 10; i++){
		for (int j = 0; j < 36; j++){
			portSpecs[i][j] = "(@0000,0000)";
			if (i < 6){
				exitSummary[i][j] = 0; //holds various summary values from the most recent EM and SWBR attempts on each device (EM exit V, EM exit R, EM time elapsed, SWBR exit V, SWBR exit R, SWBR time elapsed)
			}
		}
	}
}


Switchbox::~Switchbox(){
	//dtor
}


int Switchbox::configure(){

	int ERROR1 = 0; // error information. if any happens, this variable will go to 1;
	

	//Now we will initialize the sourcemeters
	printf("Initializing Switchbox...\n");

	this->pna = ibdev(ID_OF_BOARD, PRIMARY_ADDR_OF_PNA_SW, NO_SECONDARY_ADDR, TIMEOUT, EOTMODE, EOSMODE); //ibdev gives back ud! Unique iDentifier based on info given in

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

	GPIBWrite(pna, "*RST");
	GPIBWrite(pna, ":ROUT:FUNC ACON;");
	GPIBWrite(pna, ":ROUT:CONN:RULE 0,FREE;");
	//GPIBWrite(pna, ":ROUT:CONN:SEQ ALL,BBM;");
	closeAll();

	if (ERROR1 == 1){
		std::cout << "I am pausing the program! There is an error somewhere! Quit the program and check it! Do not press any other key, otherwise you will just unpause. BUM!\n";
		system("Pause"); //pausing execution if I have an error!
	}
	else printf("Initialization of Switchbox was successful.\n");

	return 1;
}


int Switchbox::GPIBWrite(int ud, char* cmd){
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


int Switchbox::GPIBRead(int ud, char *message){
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

void Switchbox::getAllPorts(){
	// get input file containg pad numbers for devices. 
	// each device should be its own row in the text file, each pad should be a two digit number
	// the format is ##,##,##,## where the first two are the source and drain pads, and the next two are the gateline pads
	// ungated devices should have ##,##,00,00 to indicate no gateline

	std::string file_in;
	int fileExists = 0;
	while (!fileExists){
		std::cout << "What is the input file containing ALL pad specifications for this chip?\n";
		std::cin >> file_in;
		file_in += ".txt";
		fileExists = checkFileExists(file_in);
	}
	std::ifstream infile(file_in);

	// read in and reformat the port specifications, create the output file streams for non-yield monitoring measurements
	int ndevs = 0;
	std::string comdrainCheck[72]; // will hold pad specs to identify common drain devices

	while (!infile.eof()){
		std::string buffer1;
		char buffer2[1024] = "";
		getline(infile, buffer1); // read in a single line of the port specs file
		std::string temp(buffer1);
		std::string pad1 = temp.substr(0,2);
		std::string pad2 = temp.substr(3,2);
		std::string gatepad1 = temp.substr(6,2);
		std::string gatepad2 = temp.substr(9,2);

		// making an array of pads to later identify common drain devices
		comdrainCheck[2 * ndevs] = pad1;
		comdrainCheck[2 * ndevs + 1] = pad2;

		// close S/D pads to KEITHLEY HI and LOW
		sprintf(buffer2, "(@09%s,10%s)", pad1.c_str(), pad2.c_str());
		portSpecs[0][ndevs] = buffer2;
		// close S/D pads to SPA smu1 and smu2
		sprintf(buffer2, "(@01%s,02%s)", pad1.c_str(), pad2.c_str());
		portSpecs[1][ndevs] = buffer2;
		// close S/D pads to shorting cap 
		sprintf(buffer2, "(@04%s,04%s)", pad1.c_str(), pad2.c_str());
		portSpecs[2][ndevs] = buffer2;
		// create a string for identifying devices
		sprintf(buffer2, "%s%s", pad1.c_str(), pad2.c_str());
		portSpecs[3][ndevs] = buffer2;
		// create a string for marking a device as 'to be measured' for a particular operation. initialize to NO ("n")
		portSpecs[4][ndevs] = "n";

		// Create strings dealing with closing the gateline - if device is ungated all these strings are 'n'
		if (gatepad1 == gatepad2){	// check if its an ungated device i.e. the gate pads are both 00
			sprintf(buffer2, "n");
			portSpecs[5][ndevs] = buffer2; // mark as ungated by placeholder 'n' instead of a switchbox command string
			portSpecs[6][ndevs] = buffer2;
			portSpecs[7][ndevs] = buffer2;
			portSpecs[8][ndevs] = buffer2;
		} 
		else{
			// close both gate pads to the SPA smu3 (gate for 3-terminal sweeps) input port(07)
			sprintf(buffer2, "(@07%s,07%s)", gatepad1.c_str(), gatepad2.c_str());
			portSpecs[5][ndevs] = buffer2;
			//close first gate pad to SPA smu1 and BOTH the device common drain and source pad to SPA smu2
			sprintf(buffer2, "(@01%s,02%s,02%s)", gatepad1.c_str(), pad1.c_str(), pad2.c_str());
			portSpecs[6][ndevs] = buffer2;
			// close one gate pad to SPA smu1 and the other gate pad to SPA smu2
			sprintf(buffer2, "(@01%s,02%s)", gatepad1.c_str(), gatepad2.c_str());
			portSpecs[7][ndevs] = buffer2;
			//close both pads to keithley high and keithley low to a shorting cap on one of the output ports
			sprintf(buffer2, "(@03%s,03%s)", gatepad1.c_str(), gatepad2.c_str());
			portSpecs[8][ndevs] = buffer2;
		}
			

			// close S/D pads to IETS measurement
			sprintf(buffer2, "(@05%s,06%s)", pad1.c_str(), pad2.c_str());
			portSpecs[9][ndevs] = buffer2;

			++ndevs; // will stop incrementing at n = # devices i.e. # of lines in input file;
		}

	// identify and mark common drain devices
	for (int devnum = 0; devnum < ndevs; devnum++){
		comDrain[devnum] = 0;
		for (int j = 0; j < ndevs; j++){
			if ((comdrainCheck[2 * devnum] == comdrainCheck[2 * j]) | (comdrainCheck[2 * devnum + 1] == comdrainCheck[2 * j]) | (comdrainCheck[2 * devnum] == comdrainCheck[2 * j + 1]) | (comdrainCheck[2 * devnum + 1] == comdrainCheck[2 * j + 1])){
				comDrain[j] = devnum; // note that this will get overwritten several times, and the final value will be the array index for the com drain device closest to the end of the list
			}
		}
	}

	// show the user the result of checking which devices have common drain for debugging purposes
	//std::cout << "The common drain check shows...\n";
	//for (int i = 0; i < ndevs; i++){
	//	std::cout << "Device ID# " << portSpecs[3][i].c_str() << " is in group #" << comDrain[i] << "\n";
	//}

	// the Switchbox class variable 'ndev' is initialized by the final value of the incrementer after the while loop that reads through the the list of pads
	ndev = ndevs;


}


void Switchbox::getPorts(int outputfileFlag, FILE* outputs[36]){
	// get input file containg pad numbers for devices. 
	// each device should be its own row in the text file, each pad should be a two digit number
	// the format is ##,##,##,## where the first two are the source and drain pads, and the next two are the gateline pads
	// ungated devices should have ##,##,00,00 to indicate no gateline

	//outputfileFlag is 0 if we want output files for each specific device, 1 if we want just a single output file for yield monitoring, and 2 if we don't need any files

	std::string file_in;
	int fileExists = 0;
	while (!fileExists){
		std::cout << "What is the input file containing pad specs of devices to measure?\n";
		std::cin >> file_in;
		file_in += ".txt";
		fileExists = checkFileExists(file_in);
	}
	
	std::ifstream infile(file_in);

	std::string file_out;
	// if we want either output files for each device OR a single output file for yield monitoring then ask for the file stem from user
	if (outputfileFlag == 0 || outputfileFlag == 1){
		// need error handling here to prevent program from crashing if user inputs a nonexistent file name
		std::cout << "What is the stem to use for output file names?\n";
		std::cin >> file_out;
	}

	// reset all the devices on the chip to having 'no' in the field which asks if they are in the group under test
	for (int i = 0; i < ndev; i++){
		portSpecs[4][i] = "n";
	}

	// read in the port specifications for the group of devices, mark them as under test, create the output file streams for non-yield monitoring measurements
	while (!infile.eof()){
		std::string buffer1;
		char buffer2[1024] = "";
		getline(infile, buffer1); // read in a single line of the port specs file
		std::string temp(buffer1);
		std::string pad1 = temp.substr(0, 2);
		std::string pad2 = temp.substr(3, 2);
		// create a string for identifying devices
		sprintf(buffer2, "%s%s", pad1.c_str(), pad2.c_str());

		for (int i = 0; i < ndev; i++){
			if (buffer2 == portSpecs[3][i]){
				portSpecs[4][i] = "y";
				std::cout << "Device ID# " << portSpecs[3][i].c_str() << " is marked for test.\n";

				// create the output file if the measurement is not a yield monitoring measurement
				if (outputfileFlag == 0){
					char filebuffer[1024] = "";
					sprintf(filebuffer, "%s_%s.txt", file_out.c_str(), buffer2);
					outputs[i] = fopen(filebuffer, "w+");
					if (outputs[i] == NULL) { // check that fopen worked correctly
						std::cout << "fopen has failed for the output file " << filebuffer << "\n";
					}
				}

			}
		}


	}

	if (outputfileFlag == 1){
		char filebuffer[1024] = "";
		sprintf(filebuffer, "%s_YM.txt", file_out.c_str());
		outputs[0] = fopen(filebuffer, "w+");
	}

}


void Switchbox::openAll(){
	// this will need to loop through all source drain ports and close them to input port 04
	//GPIBWrite(pna, ":ROUT:BIAS:STAT 0, OFF;"); // turns off bias mode (opens channels between designated input bias port and all bias enabled output ports);
	//GPIBWrite(pna, ":ROUT:OPEN:CARD 0;");
	// GPIBWrite(pna, ":ROUT : BIAS : STAT ALL, OFF;");
	// GPIBWrite(pna, ": ROUT : OPEN : CARD ALL;");

	//char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	//for (int devnum = 0; devnum < ndev; devnum++){
	//	sprintf(buffer, ":ROUT:OPEN %s;", portSpecs[2][devnum].c_str());    //create a new string of a command to close to keithley's ground line
	//	GPIBWrite(pna, buffer);
	//}

	GPIBWrite(pna, ":ROUT:OPEN (@0101:1028);");
}


void Switchbox::closeAll(){
	// When Bias Mode is ON, the input Bias Port is connected to all bias enabled output ports that are not connected to any other input ports.You cannot directly control which output ports are connected to the input Bias Port :
	// If another input port is disconnected(:ROUT : OPEN command) from a bias enabled output port, the output port is automatically connected to the input Bias Port.
	// If another input port is connected(:ROUT : CLOS command) to a bias enabled output port, the output port is automatically disconnected from the input Bias Port.
	// Bias disabled output ports are never connected to the input Bias Port when Bias Mode is ON.
	// When Bias Mode is OFF, the input Bias Port is the same as the other input ports, so relays can be controlled directly to connect to output ports
	// GPIBWrite(pna, ":ROUT:CONN:RULE ALL,FREE;");
	//GPIBWrite(pna, ":ROUT:BIAS:PORT 0, 10;"); // specifies which input port (on specified card) to be the input BiasPort.
	//GPIBWrite(pna, ":ROUT:BIAS:CHAN:ENAB:LIST (@1001:1016)"); //enables bias port connecting mode for the output ports 01 through 16
	//Sleep(100);
	//GPIBWrite(pna, ":ROUT:BIAS:CHAN:DIS:LIST (@1017:1036)");
	//Sleep(100);
	//GPIBWrite(pna, ":ROUT:OPEN:CARD 0;"); // 'BIAS ON' won't override any existing connections so kill them off first
	//Sleep(100);
	//GPIBWrite(pna, ":ROUT:BIAS:STAT 0, ON;"); // turns on bias mode (creates connection between designated input bias port and all bias enabled output ports);
	//Sleep(50);

	// first make sure _all_ channels from all outputs/inputs are open
	GPIBWrite(pna, ":ROUT:OPEN (@0101:1028);");

	// then close down the relevant outputs to shorting cap
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	for (int devnum = 0; devnum < ndev; devnum++){
		sprintf(buffer, ":ROUT:CLOS %s;", portSpecs[2][devnum].c_str());    //create a new string of a command to close to keithley's ground line
		GPIBWrite(pna, buffer);
	}
}


void Switchbox::closeChan(int devnum, std::string instrument){
	// which input ports are we closing to the device output ports? input bias port will automatically be disconnected from these outputs.
	int index;
	if (instrument == "SPAgate"){ index = 5; } //close both gate pads to the SPA gate input port(03)	
	if (instrument == "Leakage"){ index = 6; } //close gatepad #1 to smu1 on 01 and S pad (pad #1) to smu2 for a sweep with S at GND and sweeping Gate
	if (instrument == "Gateline"){ index = 7; } //close one gate pad to SPA input port 01 and the other gate pad to SPA input port 02
	if (instrument == "SPA"){ index = 1; } //close S pad (Pad #1) to smu1 on 01 and D pad (pad#2) to smu2 on 02
	if (instrument == "keithley"){ index = 0; } //close S pad (pad #1) to keithley HI on 09 and drain pad (pad#2) to keithley LOW on 10
	if (instrument == "HoldGate"){ index = 8; }
	if (instrument == "IETS"){ index = 9; }

	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning

	// first open the channel that keeps this device shorted to shorting cap along with any shared common drains
	// this loop will open the device of interest, including all other com drain devices it is common with (if any) away from the shorting cap
	// only do this if it's not an order to close a gateline to keithley. this prevents a situation where the order to close the gateline overwrites a previous order to connect to e.g. IETS setup
	if (instrument != "HoldGate"){
		for (int i = 0; i < ndev; i++){
			if (comDrain[i] == comDrain[devnum]){
				sprintf(buffer, ":ROUT:OPEN %s;", portSpecs[2][i].c_str());
				GPIBWrite(pna, buffer);
			}
		}
	}

	
	// then close the DUT to the specified measurement instrument
	sprintf(buffer, ":ROUT:CLOS %s;", portSpecs[index][devnum].c_str());    //create a new string of a command
	GPIBWrite(pna, buffer);
	Sleep(20);
}


void Switchbox::closeChanMonitor(int devnum, std::string instrument){
	// which input ports are we closing to the device output ports? input bias port will automatically be disconnected from these outputs.
	int index;
	if (instrument == "SPA"){ index = 1; }
	if (instrument == "keithley"){ index = 0; }
	// close the channels
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning

	// not necessary to first open the channel that keeps this device shorted to shorting cap b/c this funciton only gets called
	// when openAll() has already been used

	// then close the DUT to the specified measurement instrument
	sprintf(buffer, ":ROUT:CLOS %s;", portSpecs[index][devnum].c_str());    //create a new string of a command
	GPIBWrite(pna, buffer);
}


void Switchbox::openChan(int devnum, std::string instrument, int groundingFlag){

	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	// which input ports are we opening from the device output ports? input bias port will automatically be reconnected to these outputs ports.
	int index;
	if (instrument == "SPA"){ index = 1; }
	if (instrument == "keithley"){ index = 0; }
	if (instrument == "ShortCap"){ index = 2; }
	if (instrument == "SPAgate"){ index = 5; }
	if (instrument == "Leakage"){ index = 6; }
	if (instrument == "Gateline"){ index = 7; }
	if (instrument == "HoldGate"){ index = 8; }
	if (instrument == "IETS"){ index = 9; }

	if (index == 2){
		sprintf(buffer, ":ROUT:OPEN %s;", portSpecs[index][devnum].c_str());    //open the channel between the device and the shorting cap
		GPIBWrite(pna, buffer);
	}
	else{
		// first open the channel between the device and whatever instrument it was being measured with
		sprintf(buffer, ":ROUT:OPEN %s;", portSpecs[index][devnum].c_str());    //create a new string of a command
		GPIBWrite(pna, buffer);
		// then close it (and it's common drain friends) back down to shorting cap for grounding depending on whether groundingFlag is up (=1)
		if (groundingFlag){
			// this loop will ground the DUT, including all other com drain devices it is common with (if any)
			// don't want things to go back to ground if we are doing anything involving gating (except for measuring gateline IV)
			for (int i = 0; i < ndev; i++){
				if (comDrain[i] == comDrain[devnum]){
					sprintf(buffer, ":ROUT:CLOS %s;", portSpecs[2][i].c_str());
					GPIBWrite(pna, buffer);
				}
			}
		}
	}
}


void Switchbox::openChanMonitor(int devnum, std::string instrument){

	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning

	// which input ports are we opening from the device output ports? input bias port will automatically be reconnected to these outputs ports.
	int index;
	if (instrument == "SPA"){ index = 1; }
	if (instrument == "keithley"){ index = 0; }

	// first open the channel between the device and whatever instrument it was being measured with
	sprintf(buffer, ":ROUT:OPEN %s;", portSpecs[index][devnum].c_str());    //create a new string of a command
	GPIBWrite(pna, buffer);

	// not necessary to close it back down to keithley ground for shorting b/c this function only gets called
	// when openAll() is in use

}



void Switchbox::checkChan(int devnum, std::string instrument){
	// which input ports are we closing to the device output ports?
	int index;
	if (instrument == "SPA"){ index = 0; }
	if (instrument == "keithley"){ index = 1; }
	if (instrument == "Leakage"){ index = 6; }
	// check that the channel is actually closed and display the result
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	char bufferread[ARRAYSZ] = ""; // size of buffer is given at the beginning
	sprintf(buffer, ":ROUT:CLOS? %s;", portSpecs[index][devnum].c_str());    //create a new string of a command
	GPIBWrite(pna, buffer);
	GPIBRead(pna, bufferread);
	std::cout << "checking " << buffer << "  --- " << bufferread << "\n";
}

void Switchbox::checkAllChan(){
	// check whether any and all the channels are actually closed and display the result
	char buffer[ARRAYSZ] = ""; // size of buffer is given at the beginning
	char bufferread[ARRAYSZ] = ""; // size of buffer is given at the beginning
	sprintf(buffer, ":ROUT:CLOS? (@0101:1028);");    //create a new string of a command
	GPIBWrite(pna, buffer);
	GPIBRead(pna, bufferread);
	std::cout << "checking all channels 0101:1028 for CLOS? ---  \n" << bufferread << "\n";
}

int Switchbox::checkFileExists(std::string fileName){
	FILE * fileCheck;
	fileCheck = fopen(fileName.c_str(), "r");
	if (fileCheck != NULL)
	{
		fclose(fileCheck);
		return 1;
	}
	else{
		std::cout << "File does not exist.\n";
		return 0;
	}
}