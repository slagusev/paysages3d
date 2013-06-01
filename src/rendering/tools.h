#ifndef _PAYSAGES_TOOLS_H_
#define _PAYSAGES_TOOLS_H_

#include "shared/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED(_x_) ((void)(_x_))

double toolsRandom();
double toolsCubicInterpolate(double stencil[4], double x);
double toolsBicubicInterpolate(double stencil[16], double x, double y);
void toolsFloat2DMapCopy(double* src, double* dest, int src_xstart, int src_ystart, int dest_xstart, int dest_ystart, int xsize, int ysize, int src_xstep, int src_ystep, int dest_xstep, int dest_ystep);

#ifdef __cplusplus
}
#endif

#endif