#include "K2400.h"
#include "SPA4156B.h"
#include "Switchbox.h"
#include "SPA4145B.h"
#include "menu.h"

#include <iostream>
#include <windows.h>
#include "ni488.h"
#include <cstdlib>
#include "engine.h"

#pragma comment (lib, "libmex.lib")
#pragma comment (lib, "libmx.lib")
#pragma comment (lib, "libeng.lib")
#pragma comment (lib, "libmat.lib")

using namespace std;


int main()
{
	std::cout << "=======================================================================\n";
	K2400 keithley;            // initialize the keithley (set some parameters, verify it is listening on GPIB)
	std::cout << "=======================================================================\n";
	SPA4156B SPA;            // initialize the SPA (set some parameters, verify it is listening on GPIB)
	std::cout << "=======================================================================\n";
	Switchbox switchbox;            // initialize the switchbox (set some parameters, verify it is listening on GPIB)
	std::cout << "=======================================================================\n";
	// SPA4145B SPA_probeStation;            // initialize the switchbox (set some parameters, verify it is listening on GPIB)
	std::cout << "=======================================================================\n";
	// 150725 - apparently you can't talk to 64bit matlab with 32bit win c++ .exe's 
	//Engine *matlabEng = engOpen("null");          // open the matlab engine
	//std::cout << "Matlab Engine has been opened successfully.\n";
	//engEvalString(matlabEng, "2+2;");
	std::cout << "=======================================================================\n";
	std::cout << "All instruments on GPIB board '0' with GPIB addresses:\n";
	std::cout << "Keithley = 16  ;  SPA = 30  ;  Switchbox = 22 ; Probe Station SPA = 17. \n";
	std::cout << "=======================================================================\n\n\n";

	menu display; // create object that allows user to execute all the various routines for the different instruments
	display.menu_start(keithley, SPA, switchbox); // display the menu of what routines we can do and accept user input
	return 0;
}
