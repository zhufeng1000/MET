// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2019
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

////////////////////////////////////////////////////////////////////////
//
//    Filename:    tc_rmw.h
//
//    Description:
//
//    Mod#  Date      Name      Description
//    ----  ----      ----      -----------
//    000   04/18/19  Fillmore  New
//
////////////////////////////////////////////////////////////////////////

#ifndef  __TC_RMW_H__
#define  __TC_RMW_H__

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

#include <netcdf>
using namespace netCDF;

#include "tc_rmw_conf_info.h"

#include "vx_data2d_factory.h"
#include "vx_tc_util.h"
#include "vx_grid.h"
#include "vx_util.h"

////////////////////////////////////////////////////////////////////////
//
// Constants
//
////////////////////////////////////////////////////////////////////////

// Program name
static const char* program_name = "tc_rmw";

// ATCF file suffix
static const char* atcf_suffix = ".dat";

// Default configuration file name
static const char* default_config_filename =
    "MET_BASE/config/TCRMWConfig_default";

////////////////////////////////////////////////////////////////////////
//
// Variables for Command Line Arguments
//
////////////////////////////////////////////////////////////////////////

// Input files
static ConcatString   fcst_file;
static StringArray    fcst_files, found_fcst_files;
static StringArray    adeck_source, adeck_model_suffix;
static StringArray    bdeck_source, bdeck_model_suffix;
static StringArray    edeck_source, edeck_model_suffix;
static ConcatString   config_file;
static TCRMWConfInfo  conf_info;

// Optional arguments
static ConcatString out_dir;

////////////////////////////////////////////////////////////////////////
//
// Variables for Output Files
//
////////////////////////////////////////////////////////////////////////

// Output NetCDF file
static ConcatString out_nc_file;
static NcFile*      nc_out = (NcFile*) 0;
static NcDim        range_dim;
static NcDim        azimuth_dim;
static NcDim        track_point_dim;
static NcVar        lat_grid_var;
static NcVar        lon_grid_var;
static NcVar        valid_time_var;

// List of output NetCDF variable names
static StringArray nc_var_sa;

////////////////////////////////////////////////////////////////////////
//
// Miscellaneous Variables
//
////////////////////////////////////////////////////////////////////////

static StringArray  out_files;
static DataPlane    dp;
static TcrmwData    grid_data;
static TcrmwGrid    grid;
static ConcatString wwarn_file;

// Data file factory and input files
static Met2dDataFileFactory mtddf_factory;
static Met2dDataFile* fcst_mtddf = (Met2dDataFile*) 0;

// Grid coordinate arrays
static double* lat_grid;
static double* lon_grid;

////////////////////////////////////////////////////////////////////////

#endif  //  __TC_RMW_H__

////////////////////////////////////////////////////////////////////////