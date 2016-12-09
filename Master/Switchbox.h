#ifndef SWITCHBOX_H
#define SWITCHBOX_H

// as long as graph.h file and iostream.h is guarded, this is fine and we don't have to worry about whether it was already
// #included in e.g. main.cpp.
#include <windows.h>
#include "ni488.h"
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <math.h> //for floor
#include <fstream>


#define TIMEOUT T10s   // Timeout value = 10 seconds
#define NO_SECONDARY_ADDR 0 // PNA has no Secondary address
#define ARRAYSZ 1024
#define ERRMSGSIZE 128
#define EOTMODE 1
#define EOSMODE 0

#define ID_OF_BOARD 0 // board ID. To find it go to NI max and see GPIB. if it has 0 by it its 0, if 1, its 1 etc.
#define PRIMARY_ADDR_OF_PNA_SW 22 // GPIB address of PNA



class Switchbox
{
public:
	Switchbox();
	virtual ~Switchbox();
	int ndev;
	void closeAll();
	void openAll();
	void closeChan(int devnum, std::string instrument);
	void closeChanMonitor(int devnum, std::string instrument);
	void openChan(int devnum, std::string instrument, int groundingFlag);
	void openChanMonitor(int devnum, std::string instrument);
	void checkChan(int devnum, std::string instrument);
	void Switchbox::checkAllChan();
	void getPorts(int outputfileFlag, FILE* outputs[36], std::string file_in = "nullstring.txt");
	void getAllPorts();
	int checkFileExists(std::string fileName);
	std::string portSpecs[10][36]; //can read in port specifications for up to 36 devices. the first row are channels to keithley, the second row are channels to SPA.
	std::string ft_ID;
	float exitSummary[6][36]; //holds various summary values from the most recent EM and SWBR attempts on each device
protected:
private:
	int pna;     // this is gonna be our holder of Unique iDentifier
	int comDrain[36]; //to keep track of which devices are common drain
	int configure();
	int GPIBWrite(int ud, char* cmd);
	int GPIBRead(int ud, char* message);
};


#endif // Switchbox_H