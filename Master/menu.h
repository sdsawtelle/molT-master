#ifndef MENU_H
#define MENU_H

#include "K2400.h"
#include "SPA4156B.h"
#include "SPA4145B.h"
#include "Switchbox.h"
#include <iostream>
#include <chrono>
#include "engine.h"

class menu
{
public:
	menu();
	virtual ~menu();
	void menu_start(K2400 keithley, SPA4156B SPA, Switchbox switchbox);
	std::string chip_ID;
	std::string emailAddress;
	void SendMail(std::string emailAddress);
protected:
private:
	void executeEM(K2400 keithley, SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeKeithleySweep(K2400 keithley, SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeKeithleyDwell(K2400 keithley, SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeEMandMonitor(K2400 keithley, SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeMonitor(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeSweep(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeLeakageSweep(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeSweepBreak(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeEMandSweepBreakandGatedIV(SPA4156B SPA, Switchbox switchbox, K2400 keithley);
	void executeYield(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	float executeGapYield(int devnum, SPA4156B SPA, Switchbox switchbox);
	void executeLeakageYield(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeGatelineCheck(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeGatelineSweep(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeGatedSweeps(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeGatedSweepsNoIG(SPA4156B SPA, Switchbox switchbox, FILE* outputs[36]);
	void executeIETS(K2400 keithley, Switchbox switchbox, FILE* outputs[36], bool gateFlag);
	void holdGateVoltage(K2400 keithley, Switchbox switchbox, FILE* outputs[36], int devnum);
	void executeSweepBreakAndSelfBreak_PS(SPA4145B SPA_probeStation, FILE* output);
	void closeFiles(int ymflag, int ndev, FILE* outputs[36], Switchbox switchbox);
};

#endif // MENU_H

