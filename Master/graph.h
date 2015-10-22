#ifndef GRAPH_H
#define GRAPH_H
#include <cstdio>

class graph
{
    public:
        graph();
        virtual ~graph();
        void replot();
    protected:
    private:
        FILE* gnupipe;
        char path_gnu[100];
};

#endif // GRAPH_H
