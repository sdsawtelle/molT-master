
#ifndef K2400_H
#define K2400_H

// #include "graph.h"
#include <iostream>
#include <windows.h>
#include "ni488.h"
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <math.h> //for floor
#include <chrono>

#define TIMEOUT T10s   // Timeout value = 10 seconds
#define NO_SECONDARY_ADDR 0 // PNA has no Secondary address
#define PRIMARY_ADDR_OF_KEITHLEY 16 // GPIB address of PNA
#define ARRAYSZ 1024
#define ERRMSGSIZE 128
#define EOTMODE 1
#define EOSMODE 0

class K2400
{

public:
	K2400();
	virtual ~K2400();

	int start();
	int configure();
	void setParams();
	void initializeEM();
	void setParamsSweep(float* volt_ramp_temp, float* volt_start_temp, float* volt_stop_temp, float* target_resistance_temp);
	void initializeSweep(float volt_ramp_temp, float volt_start_temp, float volt_stop_temp, float target_resistance_temp);
	void setParamsDwell();
	void display_parameters();
	void measurement(int ifVoltage);
	float emSingle(int devnum, FILE* outputs[36]);
	float sweepSingle(int devnum, FILE* outputs[36]);
	float dwellSingle(int devnum, FILE* outputs[36], float dwellV);
	int emSingleProbeStation();
	void holdGateVoltage();
	void Spawn(int pick_program, std::string emailAddress); // i will be the parameter that chooses which program to Spawn!
	void SendMail(std::string emailAddress);


protected:
private:
	std::string file_name;
	std::string timelog_name;

	float resistance_timely; //resistance of the last measurement
	float resistance_benchmark;        //resistance at the bottom of current "hill"
	float resistance_tolerance;
	float resistance_tolerance_high;
	float target_resistance;
	float target_resistance_tolerance;

	float delay;   // delay between ramping the voltage every voltage (See the variable down here)
	float volt_ramp; //voltage ramp
	float volt_start;
	float volt_stop;  // voltage stop
	float volt_down;  //voltage by which we ramp down
	int ntrigger;  //length of consecutive string to trigger ramp down
	int nnegres;  //# consecutive NDR to trigger rampdown
	int ntrigger_high;  //length of consecutive string to trigger ramp down
	int nnegres_high;  //# consecutive NDR to trigger rampdown
	int nplc; //number of power line cycles to integrate over
	int rampdown_dwell; //dwell time (in msec) for each voltage point in the rampdown - N.B. rampdown occurs in 1mV step so rate is 1mV per rampdown_dwell msec.
	float res_switch; // resistance at which to switch from using low-R regime parameters to high-R regime parameters
	float temperature; //temperature reading on lakeshore to record what temp we are breaking wire at

	float volt_ramp_KS; //voltage ramp
	float volt_start_KS;
	float volt_stop_KS;  // voltage stop
	float target_resistance_KS; 



	int pna;     // this is gonna be our holder of Unique iDentifier

	float DoMeasurement(FILE* data, FILE* reading, FILE* output, float delay);
	float DoSweep(FILE* data, FILE* reading, FILE* output, float delay);
	float DoDwell(FILE* data, FILE* reading, FILE* output, float delay, float dwellV);
	void GetReading(char* buffer, std::string &temporary, FILE* output, FILE* log_keithley);
	void KeepTrackIndicators(int &res_consecutive, int &targ_res_consecutive, int &neg_res_consecutive, int &bench_res_consecutive, float &resistance_benchmark, float &resistance_timely, float &diffres, float target_resistance, float target_resistance_tolerance, int &counter);

	// void ShowGraph();
	float GetResistance(float *diffres, float &voltage, float &resistance, FILE* stream, FILE* output, int counter);
	int GPIBWrite(int ud, char* cmd);
	int GPIBRead(int ud, char* message);
	bool CheckRampDown(float resistance_timely, int &res_consecutive, int &bench_res_consecutive, int &neg_res_consecutive);
	bool GotTargetResistance(float &resistance_timely, int &targ_res_consecutive, float &volt_stop, float &voltage);
};

#endif // K2400_H
