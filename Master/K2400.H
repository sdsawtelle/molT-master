
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
#include "Switchbox.h"

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
	void setParamsEM_fromfile(int emType); // set EM parameters from a user-specified file
	void initializeEM(int emType);  // initialize integration, range, delay and turn OUTPUT ON
	float WrapEM(int emType, int devnum, FILE* outputs[36], Switchbox switchbox); // write headers and execute EM on a single device
	float temperature; //temperature we are breaking wire at

	void setGateVoltage(float Vgate);
	void holdGateVoltage();
	void holdDEPVoltage(float voltage, float holdTime);

	void display_parameters();
	void Spawn(int pick_program, std::string emailAddress); // i will be the parameter that chooses which program to Spawn!
	void SendMail(std::string emailAddress);

protected:
private:
	// ---------------------------------------------------------------------------------------------------------
	// ---------------------------------------  GPIB -----------------------------------
	// ---------------------------------------------------------------------------------------------------------
	int pna;     // Unique iDentifier for GPIB communication
	int GPIBWrite(int ud, char* cmd);
	int GPIBRead(int ud, char* message);


	// ---------------------------------------------------------------------------------------------------------
	// --------------------------------------- STANDARD EM -----------------------------------
	// ---------------------------------------------------------------------------------------------------------
	float resistance_timely; //resistance of the most recent measurement
	float initial_res; //the first resistance measured for the device on beginning EM
	float benchmark_resistance; //average resistance at beginning of a ramp cycle
	float target_resistance; //the target resistance for EM to exit
	float avg_series_res; //an average series resistance presented by devices of this type
	int rampType; //determines what events we look at for triggering ramp back

	float bench_percent_tols[4]; //possible values of "tol" for R>R_bench*(1+tol) events (present value depends on Rjct) 
	float delta_tols[4]; // possible values of "delta" delta(R) > delta event counter
	int n_percent_trigger[4];  //possible number of consecutive R > R_bench*(1+tol) events to trigger ramp down  (present value depends on Rjct)
	int n_ndr_trigger[4]; //possible number consecutive NDR events to trigger rampdown (present value depends on Rjct)
	int n_delta_trigger[4]; //possible number consecutive delta(R) > delta events to trigger ramp down (present value depends on Rjct)
	float res_jct_switch[4]; // Rjct at which to switch from between the possible values in the above arrays (e.g. Rjct < res_switch[0] means use n_ndr_trigger[0])

	float bench_tol; //present value of "tol" for R>R_bench*(1+tol) and R > (Rjct + R_avg_series)*(1+tol) events
	float delta_tol; //present value of "delta" for delta(R) > delta
	int n_percent; //present number of consecutive R > R_bench*(1+tol) events to trigger ramp down
	int n_ndr; //present number consecutive NDR events to trigger rampdown
	int n_delta; //present number consecutive delta(R) > delta events to trigger ramp down

	float delay;   // delay between ramping the voltage every voltage (See the variable down here)
	float volt_ramp; //voltage step for ramping up
	float volt_start; // voltage at which to start ramp
	float volt_stop;  // max voltage allowed
	float volt_down;  //voltage by which we ramp down
	int nplc; //number of power line cycles to integrate over
	int rampdown_dwell; //dwell time (in msec) for each voltage point in the rampdown - N.B. rampdown occurs in 1mV step so rate is 1mV per rampdown_dwell msec.


	float DoEM(FILE* data, FILE* reading, FILE* output, int devnum, Switchbox switchbox);
	float DoConstVoltEM(FILE* data, FILE* reading, FILE* output, int devnum, Switchbox switchbox);
	void GetReading(char* buffer, std::string &temporary, FILE* output, FILE* log_keithley);
	float CalcResTimely(FILE* reading_log_keithley, FILE* output);
	float CalcMetrics(float *diffres, float *delta_res, float *old_resistance, float &voltage_read, float &current_read, FILE* reading_log_keithley, FILE* output);
	void CalcBenchmark(int &counter);
	float CalcInitialRes(float at_voltage, FILE* log_keithley, FILE* reading_log_keithley, FILE* output);
	void UpdateCounters(int &counter_delta, int &counter_percent, int &counter_ndr, int &counter_normed_percent, int &counter, float &diffres, float &delta_res);
	void UpdateTolerances();
	bool CheckRampBack(int &counter_delta, int &counter_percent, int &counter_ndr, int &counter_normed_percent);

	// ---------------------------------------------------------------------------------------------------------
	// --------------------------------------- CONSTANT-V CURVATURE EM -----------------------------------
	// ---------------------------------------------------------------------------------------------------------
	float dwell_rampback_percent;
	float partial_target_resistance;


	// ---------------------------------------------------------------------------------------------------------
	// --------------------------------------- RANGE HANDLING -----------------------------------
	// ---------------------------------------------------------------------------------------------------------
	float present_volt_range; //range it is present on
	float determine_voltage_range(float src_volt); //optimal range for present sourced voltage
	int set_voltage_range(float range_volt); //set the range
	int switch_voltage_range(Switchbox switchbox, int devnum, float range_volt); //wrapper for setting the range that opens/closes switchbox connection

	float present_curr_range; //range it is presently on
	float determine_current_range(float meas_curr); //optimal range for present measure current
	int set_current_range(float range_curr); //set the range
	int switch_current_range(Switchbox switchbox, int devnum, float range_curr); //wrapper for setting the range that opens/closes switchbox connection
};

#endif // K2400_H
