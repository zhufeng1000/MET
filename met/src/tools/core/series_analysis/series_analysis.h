// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2012
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

////////////////////////////////////////////////////////////////////////
//
//   Filename:   series_analysis.h
//
//   Description:
//
//   Mod#   Date      Name            Description
//   ----   ----      ----            -----------
//   000    12/10/12  Halley Gotway   New
//
////////////////////////////////////////////////////////////////////////

#ifndef  __SERIES_ANALYSIS_H__
#define  __SERIES_ANALYSIS_H__

////////////////////////////////////////////////////////////////////////

using namespace std;

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <ctype.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "netcdf.hh"

#include "series_analysis_conf_info.h"

#include "vx_data2d_factory.h"
#include "vx_grid.h"
#include "vx_util.h"
#include "vx_stat_out.h"
#include "vx_gsl_prob.h"

////////////////////////////////////////////////////////////////////////
//
// Constants
//
////////////////////////////////////////////////////////////////////////

static const char * program_name = "series_analysis";

// Default configuration file name
static const char * default_config_filename =
   "MET_BASE/data/config/SeriesAnalysisConfig_default";

////////////////////////////////////////////////////////////////////////
//
// Variables for Command Line Arguments
//
////////////////////////////////////////////////////////////////////////

// Input files
static StringArray fcst_files;
static StringArray obs_files;
static GrdFileType ftype = FileType_None;
static GrdFileType otype = FileType_None;

// Output file
static ConcatString out_file;

// Input Config file
static ConcatString config_file;
static SeriesAnalysisConfInfo conf_info;

////////////////////////////////////////////////////////////////////////
//
// Variables for Output Files
//
////////////////////////////////////////////////////////////////////////

// Output NetCDF file
static NcFile *nc_out  = (NcFile *) 0;
static NcDim  *lat_dim = (NcDim *)  0;
static NcDim  *lon_dim = (NcDim *)  0;

// Structure to store computed statistics and corresponding metadata
struct StatInfo {
   ConcatString name_att;        // Name attribute
   ConcatString long_name_att;   // Long name attribute
   ConcatString fcst_thresh_att; // Forecast threshold attribute
   ConcatString obs_thresh_att;  // Observation threshold attribute
   double       alpha_att;       // Alpha value attribute
   float * data;                 // Data values
};

// Mapping of NetCDF variable name to computed statistic
map<ConcatString, StatInfo> stat_data;

////////////////////////////////////////////////////////////////////////
//
// Miscellaneous Variables
//
////////////////////////////////////////////////////////////////////////

// Grid variables
static Grid grid;
static bool is_first_pass = true;

// Data file factory and input files
static Met2dDataFileFactory mtddf_factory;
static Met2dDataFile *fcst_mtddf = (Met2dDataFile *) 0;
static Met2dDataFile *obs_mtddf  = (Met2dDataFile *) 0;

// Pointer to the random number generator to be used
static gsl_rng *rng_ptr = (gsl_rng *) 0;

// Enumeration of ways that a series can be defined
enum SeriesType {
   SeriesType_None,       // Undefined series type
   SeriesType_Fcst_Conf,  // Defined by fcst.field configuration
   SeriesType_Obs_Conf,   // Defined by obs.field configuration
   SeriesType_Fcst_Files, // Defined by -fcst command line option
   SeriesType_Obs_Files   // Defined by -obs command line option
};
static SeriesType series_type = SeriesType_None;

// Series length
static int n_series = 0;

// Is this a time series
static bool is_time_series = false;

// Range of timing values encountered in the data
static unixtime fcst_init_beg  = (unixtime) 0;
static unixtime fcst_init_end  = (unixtime) 0;
static unixtime fcst_valid_beg = (unixtime) 0;
static unixtime fcst_valid_end = (unixtime) 0;
static int      fcst_lead_beg  = bad_data_int;
static int      fcst_lead_end  = bad_data_int;

static unixtime obs_init_beg   = (unixtime) 0;
static unixtime obs_init_end   = (unixtime) 0;
static unixtime obs_valid_beg  = (unixtime) 0;
static unixtime obs_valid_end  = (unixtime) 0;
static int      obs_lead_beg   = bad_data_int;
static int      obs_lead_end   = bad_data_int;

////////////////////////////////////////////////////////////////////////

#endif   //  __SERIES_ANALYSIS_H__

////////////////////////////////////////////////////////////////////////
