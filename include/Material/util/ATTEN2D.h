#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
const unsigned int MYTRUE  = 1;
const unsigned int MYFALSE = 0;

int flag_SetArray = 0;
const int ny_max = 7;
const int nx_max = 33;
double dataX[7][33];
double dataY[7][33];
double dataG[7][33];

int inMAGRO(double x,double y) {
  double absx = fabs(x);
  if( absx<=600 )
    return (fabs(y) <= 200. -  50./600. * absx       ) ? MYTRUE : MYFALSE;
  if( absx<=800 )
    return (fabs(y) <= 150. - 130./200. * (absx-600.)) ? MYTRUE : MYFALSE;
  return MYFALSE;
}

int numY(double y) {
  if( y <= dataY[0][0] )        return 0;
  if( y >= dataY[ny_max-1][0] ) return ny_max-1;

#define MinVal(val1, val2) ( val1 < val2 ? val1 : val2 )
  int numy;
  for(numy=0;numy<ny_max-1;numy++) {
    double dis1 = y - dataY[numy][0];
    double dis2 = y - dataY[numy+1][0];
    if( dis1*dis2 <= 0 ) {
      if( MinVal(fabs(dis1),fabs(dis2))==fabs(dis1) ) {
        break;
      }
      if( MinVal(fabs(dis1),fabs(dis2))==fabs(dis2) ) {
        numy = numy+1;
        break;
      }
    }
  }
#undef MinVal
  return numy;
}

int numX(int numy,double x) {
  if( x <= dataX[numy][0] )        return 0;
  if( x >= dataX[numy][nx_max-1] ) return nx_max-1;

#define MinVal(val1, val2) ( val1 < val2 ? val1 : val2 )
  int numx;
  for(numx=0;numx<nx_max;numx++) {
    double dis1 = x - dataX[numy][numx];
    double dis2 = x - dataX[numy][numx+1];
    if( dis1*dis2 <= 0 ) {
      if( MinVal(fabs(dis1),fabs(dis2))==fabs(dis1) ) {
        break;
      }
      if( MinVal(fabs(dis1),fabs(dis2))==fabs(dis2) ) {
        numx++;
        break;
      }
    }
  }
#undef MinVal
  return numx;
}

int SetArray()
{
  if( flag_SetArray ) return MYTRUE;

  const char* fname = "../include/util/ATTEN2D.txt";
  FILE *fi = fopen(fname,"r");
  if( fi==NULL ) {
    printf(" #E : faled to read %s\n",fname);
    return MYFALSE;
  }

  int ny = 0, nx = 0;

  char line[1000];
  while( fgets(line,1000,fi)!=NULL ) {
    if( strlen(line)<3 ) {
      ny++;
      nx = 0;
    }
    else {
      double x,y,sig;
      if( sscanf(line,"%lf %lf %lf",&y,&x,&sig)==3 ) {
        dataX[ny][nx] = x;
        dataY[ny][nx] = y;
        dataG[ny][nx] = sig;
        nx++;
      }
    }
  }
  printf(" \"%s\" was correctly read\n",fname);

  flag_SetArray = MYTRUE;
  return MYTRUE;
}

//////////////////////////////////////////////////////
extern "C"  {
#endif

  //////////////////////////////////////
  //// Difinition of axies :        ////       
  ////      -------------------     ////
  ////     /                   \    ////
  ////    /                     \   ////
  ////   |         MAGRO         |  ////
  ////    \                     /   ////
  ////     \                   /    ////
  //// Y^   -------------------     ////
  ////  |                           ////
  ////   --> X (arclength)          ////
  //////////////////////////////////////
  float ATTEN2D(float x/*mm*/,float y/*mm*/,int num/*0 or 1*/) {
    //// x,y : -800mm ~ +800mm
    //// num : 0 = up, 1 = down
    if( num==1 ) x = -x; 

    /* if( fabs(x)>624 ) return 0; */

    if( num!=0 && num!=1 ) return 0;
    if( !inMAGRO(x,y) )    return 0;
    if( !SetArray() )      return 0;

    int ny = numY(y);
    if( ny==-1 )     ny = 0;
    if( ny==ny_max ) ny = ny_max-1;

    int nx = numX(ny,x);
    if( nx==-1 )     nx = 0;
    if( nx==nx_max ) nx = nx_max-1;

    int dnx = ( x - dataX[ny][nx] < 0 ) ? -1 : 1;
    int dny = ( y - dataY[ny][nx] < 0 ) ? -1 : 1;
    if( nx + dnx < 0 || nx + dnx >= nx_max ) dnx = 0;
    if( ny + dny < 0 || ny + dny >= ny_max ) dny = 0;

    int ny1 = ny; 
    int nx1 = nx + dnx;
    int ny2 = ny + dny;
    int nx2 = numX(ny2,x);

    double vec1[3];
    double vec2[3];
    vec1[0] = dataX[ny1][nx1] - dataX[ny][nx]; 
    vec1[1] = dataY[ny1][nx1] - dataY[ny][nx]; 
    vec1[2] = dataG[ny1][nx1] - dataG[ny][nx]; 

    vec2[0] = dataX[ny2][nx2] - dataX[ny][nx]; 
    vec2[1] = dataY[ny2][nx2] - dataY[ny][nx]; 
    vec2[2] = dataG[ny2][nx2] - dataG[ny][nx]; 

    double len1 = ( dnx == 0 ) ? 0 : fabs(x-dataX[ny][nx])/( sqrt( vec1[0]*vec1[0] + vec1[1]*vec1[1] ) );
    double len2 = ( dny == 0 ) ? 0 : fabs(y-dataY[ny][nx])/( sqrt( vec2[0]*vec2[0] + vec2[1]*vec2[1] ) );
    if( fabs(len1-0.5) > 0.5 ) len1 = 0; 
    if( fabs(len2-0.5) > 0.5 ) len2 = 0; 

    return dataG[ny][nx] + len1*vec1[2] + len2*vec2[2]; 
  }

#ifdef __cplusplus
}
#endif

