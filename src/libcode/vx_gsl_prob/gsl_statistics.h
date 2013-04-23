// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2013
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*


////////////////////////////////////////////////////////////////////////

#ifndef  __MET_GSL_STATISTICS_H__
#define  __MET_GSL_STATISTICS_H__

////////////////////////////////////////////////////////////////////////

#include "vx_util.h"
#include "gsl/gsl_statistics_double.h"

////////////////////////////////////////////////////////////////////////
//
// Wrapper functions for GSL statistics computations
//
////////////////////////////////////////////////////////////////////////

extern double stats_lag1_autocorrelation(const NumArray &);

////////////////////////////////////////////////////////////////////////

#endif   /*  __MET_GSL_STATISTICS_H__  */

////////////////////////////////////////////////////////////////////////

