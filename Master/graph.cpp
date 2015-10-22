#include "graph.h"
#include <cstdio>
#include <iostream>

graph::graph()
{ //define and open the pipe to gnu
    gnupipe = _popen("","w");
    fprintf(gnupipe, "chdir \"C:\\Program Files\\gnuplot\\bin\" \n");
    fprintf(gnupipe, ".\\gnuplot.exe \n");
  //set the directory to gnuplox.exe
  fprintf(gnupipe, "cd 'C:\\Users\\Sens\\Desktop\\Stan\\measuring I' \n ");
  //set the separators to ","
  fprintf(gnupipe, "set datafile separator \",\" \n");
  // assuming your data file is placed in the gnuplot bin directory
  fprintf(gnupipe, "plot \"Data.txt\" \n");
  //flush the buffer
  fflush(gnupipe);
  //ready to replot if needed!
  std::cout<< "Ready to replot if needed!";
}

graph::~graph()
{ std::cout <<"closing the pipe!";
  fprintf(gnupipe,"exit \n");
  _pclose(gnupipe);
}

void graph::replot(){
  //replot and flush the stream
fprintf(gnupipe, "replot \n");
fflush(gnupipe);
}
