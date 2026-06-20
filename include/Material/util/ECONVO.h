#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
bool seed = false;
double urand() { return rand()/(RAND_MAX+1.); }
double grand() { return sqrt(-2*log(urand()))*sin(2*M_PI*urand()); }

extern "C" {
#endif

  float ECONVO(float EL) {
    if( !seed ) {
      srand((unsigned)(time(NULL)+10));
      for(int i=0;i<10;i++) rand();
      seed = true;
    }
    double MUe = EL * 10000.0 * 0.2 * 0.01;
    double PHOTONEMISSION = 1.0 - exp(-MUe);
    float  ret = 0;
    if( urand() < PHOTONEMISSION ) 
    {
      double Reso = EL/sqrt(MUe);
      ret = fabs(EL+Reso*grand());
    }
    return ret;
  }

#ifdef __cplusplus
}
#endif

