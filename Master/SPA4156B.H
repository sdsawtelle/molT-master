#ifndef SPA4156B_H
#define SPA4156B_H

#include <iostream>
#include <windows.h>
#include "ni488.h"
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <math.h> //for floor
#include <chrono>

#define TIMEOUT T10s   // Timeout value = 10 seconds
#define NO_SECONDARY_ADDR 0 // PNA has no Secondary address
#define ARRAYSZ 140000
#define ERRMSGSIZE 128
#define EOTMODE 1
#define EOSMODE 0
#define DELAY 0
// #define ID_OF_BOARD 0 // board ID. To find it go to NI max and see GPIB. if it has 0 by it its 0, if 1, its 1 etc.
#define PRIMARY_ADDR_OF_PNA_SPA 30 // GPIB address of PNA

class SPA4156B
{
public:
	SPA4156B();
	virtual ~SPA4156B();
	void setParams(int mode); // configure the SPA to do a sweep, sample for self-break of sample for yield depending on which mode is inputted
	void setGatedIVParamsLater(float* Vstart, float* Vstop, float* Vstep, float* Vstart_gate, float* Vstep_gate, float* Vstop_gate);
	void setSwBrParamsLater(int* useEMVFlag, float* minStart, float* maxStart, float* targetR, float* stepBias, float* stepV);
	void setIVParamsLater(float* Vstart, float* Vstop, float* Vstep);
	int sampleSingle(int devnum, FILE* outputs[36], int cycle);
	int constsampleSingle(int devnum, FILE* outputs[36], int cycle);
	void yieldSingle(int devnum, FILE* outputs[36]);
	float yieldGapSingle();
	void sweepSingle(int devnum, FILE* outputs[36], int gateFlag);
	float sweepBreak(int devnum, FILE* outputs[36], float maxStart, float minStart, float targetR, float stepBias, float stepV, float temperature);
	void constconfigSample();
	float wait_time;	//delay between subsequent sweeps through all device
	void Spawn(int pick_program, std::string emailAddress); // i will be the parameter that chooses which program to Spawn!
	void SendMail(std::string emailAddress);
	void ShowGraph(std::string emailAddress);

	void configGatedSweeps(float Vstart, float Vstop, float Vstep, float Vstart_gate, float Vstep_gate, float Vstop_gate);
	void configSweep(float Vstart, float Vstop, float Vstep);

protected:
private:
	int configure();
	int portVoltages[36];	// this will store whether to use the high or low voltage value for each device
	int voltageFlag; // a flag that indicates what is the present set value of bias for the SPA
	int pna;     // this is gonna be our holder of Unique iDentifier
	void configSample();
	void configGatedSweepsNoIG(float Vstart, float Vstop, float Vstep, float Vstart_gate, float Vstep_gate, int Nsteps_gate);
	int GetReading(char* buffer, std::string &temporary, FILE* sb_output);
	int GetParsedReading(char* buffer, std::string &temporary, FILE* sb_output);
	int GetGatedReading(char* buffer, std::string &temporary, FILE* sb_output);
	int GetGatedReadingNoIG(char* buffer, std::string &temporary, FILE* sb_output);
	int GetYieldReading(char* buffer, std::string &temporary, FILE* sb_output);
	// int SingleGetReading(char* buffer, std::string &temporary, FILE* sb_output);
	int GPIBWrite(int ud, char* cmd);
	int GPIBRead(int ud, char* message);
};


#endif // SPA4156B_H