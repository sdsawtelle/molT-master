#ifndef SPA4145B_H
#define SPA4145B_H

#include <iostream>
#include <windows.h>
#include "ni488.h"
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <math.h> //for floor

#define TIMEOUT T10s   // Timeout value = 10 seconds
#define NO_SECONDARY_ADDR 0 // PNA has no Secondary address
#define ARRAYSZ 1024
#define ERRMSGSIZE 128
#define EOTMODE 1
#define EOSMODE 0
#define DELAY 0
// #define ID_OF_BOARD 0 // board ID. To find it go to NI max and see GPIB. if it has 0 by it its 0, if 1, its 1 etc.
#define PRIMARY_ADDR_OF_PNA_SPA 17 // GPIB address of PNA

class SPA4145B
{
public:
	SPA4145B();
	virtual ~SPA4145B();
	void setParams(int mode); // configure the SPA to do a sweep, sample for self-break of sample for yield depending on which mode is inputted
	int sampleSingle(int devnum, FILE* outputs[36], int cycle);
	void constsampleSingle(FILE* output, int cycle);
	void yieldSingle(int devnum, FILE* outputs[36]);
	void sweepSingle(int devnum, FILE* outputs[36], int gateFlag);
	void sweepBreak(FILE* output, float maxStart, float minStart, float targetR, float stepBias, float stepV);
	void constconfigSample();
	float wait_time;	//delay between subsequent sweeps through all device
protected:
private:
	int configure();
	int portVoltages[36];	// this will store whether to use the high or low voltage value for each device
	int voltageFlag; // a flag that indicates what is the present set value of bias for the SPA
	int pna;     // this is gonna be our holder of Unique iDentifier
	void configSample();
	void configSweep(float Vstart, float Vstop, float Vstep);
	void configGatedSweeps(float Vstart, float Vstop, float Vstep, float Vstart_gate, float Vstep_gate, int Nsteps_gate);
	int GetReading(char* buffer, std::string &temporary, FILE* sb_output);
	int GetParsedReading(char* buffer, std::string &temporary, FILE* sb_output);
	int GetGatedReading(char* buffer, std::string &temporary, FILE* sb_output);
	int GetYieldReading(char* buffer, std::string &temporary, FILE* sb_output);
	// int SingleGetReading(char* buffer, std::string &temporary, FILE* sb_output);
	int GPIBWrite(int ud, char* cmd);
	int GPIBRead(int ud, char* message);
};


#endif // SPA4145B_H