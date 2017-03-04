
#include "cacode.h"

void cacode(int prn, int * pcode)
{
    int g2s[33] = {0,5,6,7,8,17,18,139,140,141,251,252,254,255,256,257,258,469,470,471,472,473,474,509,512,513,514,515,516,859,860,861,862};
    int g2shift;
    int reg1[10], reg2[10];
    int g1[1023],g2[1023],g2tmp[1023];
    int i,j,save1, save2;

    g2shift = g2s[prn];
    // Initialize reg, g1, and g2
    for(i = 0; i< 10; i++)
    {
        reg1[i] = -1;
        reg2[i] = -1;
    }
    for(i =0; i<1023; i++)
    {
        g1[i] = 0;
        g2[i] = 0;
    }

    // generate the g1 and g2
    for(i = 0; i<1023; i++)
    {
        g1[i]   =   reg1[9];
        g2[i]   =   reg2[9];

        save1   =   reg1[2]*reg1[9];
        save2   =   reg2[1]*reg2[2]*reg2[5]*reg2[7]*reg2[8]*reg2[9];

        for(j = 9; j>0; j--)
        {
            reg1[j] = reg1[j-1];
            reg2[j] = reg2[j-1];
        }
        reg1[0] = save1;
        reg2[0] = save2;
    }

    // shift G2 code
    for(i = 0; i< g2shift; i++)
        g2tmp[i] = g2[1023-g2shift+i];
    for(i = g2shift; i< 1023; i++)
        g2tmp[i] = g2[i-g2shift];
    // generate CA code
    for(i = 0; i < 1023; i++)
        *(pcode+i) = g1[i]*g2tmp[i];
}
