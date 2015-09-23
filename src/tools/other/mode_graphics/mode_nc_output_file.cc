// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2015
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*


////////////////////////////////////////////////////////////////////////


using namespace std;

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <cmath>

#include "mode_nc_output_file.h"
#include "nc_var_info.h"


////////////////////////////////////////////////////////////////////////


   //
   //  Code for class ModeNcOutputFile
   //


////////////////////////////////////////////////////////////////////////


ModeNcOutputFile::ModeNcOutputFile()

{

init_from_scratch();

}


////////////////////////////////////////////////////////////////////////


ModeNcOutputFile::~ModeNcOutputFile()

{

close();

}


////////////////////////////////////////////////////////////////////////


ModeNcOutputFile::ModeNcOutputFile(const ModeNcOutputFile &)

{

// init_from_scratch();
// 
// assign(m);

mlog << Error << "\n\n  ModeNcOutputFile::ModeNcOutputFile(const ModeNcOutputFile &) -> should never be called!\n\n";

exit ( 1 );

}


////////////////////////////////////////////////////////////////////////


ModeNcOutputFile & ModeNcOutputFile::operator=(const ModeNcOutputFile &)

{

// if ( this == &m )  return ( * this );
// 
// assign(m);

mlog << Error << "\n\n  ModeNcOutputFile::operator=(const ModeNcOutputFile &) -> should never be called!\n\n";

exit ( 1 );

}


////////////////////////////////////////////////////////////////////////


void ModeNcOutputFile::init_from_scratch()

{

f = (NcFile *) 0;

_Grid = (Grid *) 0;

close();

return;

}


////////////////////////////////////////////////////////////////////////


void ModeNcOutputFile::close()

{

if ( f )  { delete f;  f = (NcFile *) 0; }

if ( _Grid )  { delete _Grid;  _Grid = (Grid *) 0; }

FcstObjId  = (NcVar *) 0;
FcstCompId = (NcVar *) 0;

ObsObjId   = (NcVar *) 0;
ObsCompId  = (NcVar *) 0;

FcstRaw    = (NcVar *) 0;
ObsRaw     = (NcVar *) 0;

Nx = Ny = 0;

ValidTime = InitTime = 0;

AccumTime = 0;

fcst_data_range_calculated = false;
 obs_data_range_calculated = false;

FcstDataMin = FcstDataMax = 0.0;
 ObsDataMin =  ObsDataMax = 0.0;

// NFcstObjs  = 0;
// NFcstComps = 0;
// 
// NObsObjs   = 0;
// NObsComps  = 0;

Filename.clear();

return;

}


////////////////////////////////////////////////////////////////////////


bool ModeNcOutputFile::open(const char * _filename)

{

// int x, y;
// int value;
NcDim * dim = (NcDim *) 0;
NcAtt * att = (NcAtt *) 0;
ConcatString s;


close();

Filename = _filename;

f = new NcFile(_filename);

if ( !(f->is_valid()) )  {

   close();

   return ( false );

}

   //
   //  get dimensions
   //


dim = f->get_dim("lon");

Nx = dim->size();


dim = f->get_dim("lat");

Ny = dim->size();


dim = (NcDim *) 0;

   //
   //  variables
   //

FcstRaw    = f->get_var("fcst_raw" );
ObsRaw     = f->get_var("obs_raw" );

FcstObjId  = f->get_var("fcst_obj_id" );
// FcstCompId = f->get_var("fcst_comp_id");
FcstCompId = f->get_var("fcst_clus_id");

ObsObjId   = f->get_var("obs_obj_id" );
// ObsCompId  = f->get_var("obs_comp_id");
ObsCompId  = f->get_var("obs_clus_id");

   //
   //  count objects
   //
/*
for (x=0; x<Nx; ++x)  {

   for (y=0; y<Ny; ++y)  {

      value = fcst_obj_id(x, y);

      if ( value > NFcstObjs )  NFcstObjs = value;

      value = obs_obj_id(x, y);

      if ( value > NObsObjs )  NObsObjs = value;

      value = fcst_comp_id(x, y);

      if ( value > NFcstComps )  NFcstComps = value;

      value = obs_comp_id(x, y);

      if ( value > NObsComps )  NObsComps = value;

   }

}
*/

   //
   //  get grid
   //

_Grid = new Grid;

read_netcdf_grid(f, *_Grid);

   //
   //  get init time, valid time, lead time from FcstRaw variable attributes
   //

att = FcstRaw->get_att("init_time_ut");

switch ( att->type() )  {

   case ncInt:
      InitTime = att->as_nclong(0);
      break;

   case ncChar:
      s = att->as_string(0);
      InitTime = string_to_unixtime(s);
      break;

   default:
      mlog << Error
           << "ModeNcOutputFile::open(const char *) -> init time should be an integer or a string!\n\n";
      exit ( 1 );
      break;

}   //  switch


att = FcstRaw->get_att("valid_time_ut");

switch ( att->type() )  {

   case ncInt:
      ValidTime = att->as_nclong(0);
      break;

   case ncChar:
      s = att->as_string(0);
      ValidTime = string_to_unixtime(s);
      break;

   default:
      mlog << Error
           << "ModeNcOutputFile::open(const char *) -> valid time should be an integer or a string!\n\n";
      exit ( 1 );
      break;

}   //  switch

// att = FcstRaw->get_att("accum_time_sec");
// 
// AccumTime = att->as_nclong(0);


   //
   //  done
   //

return ( true );

}


////////////////////////////////////////////////////////////////////////


void ModeNcOutputFile::dump(ostream & out) const

{

out << "\n";

if ( f )  out << "File is open\n";
else      out << "file is closed\n";

out << "\n";

out << "Nx = " << Nx << "\n";
out << "Ny = " << Ny << "\n";

out << "\n";

if ( f )  {

   int month, day, year, hour, minute, second;
   char junk[512];

   out << "ValidTime = ";
   unix_to_mdyhms(ValidTime, month, day, year, hour, minute, second);
   sprintf(junk, "%s %d %d  %02d:%02d:%02d", short_month_name[month], day, year, hour, minute, second);
   out << junk << '\n';

   out << "InitTime  = ";
   unix_to_mdyhms(InitTime, month, day, year, hour, minute, second);
   sprintf(junk, "%s %d %d  %02d:%02d:%02d", short_month_name[month], day, year, hour, minute, second);
   out << junk << '\n';

   out << "AccumTime = ";
   sprintf(junk, "%02d:%02d:%02d", AccumTime/3600, (AccumTime%3600)/60, AccumTime%60);
   out << junk << '\n';

}

// out << "NFcstObjs  = " << NFcstObjs  << "\n";
// out << "NObsObjs   = " << NObsObjs   << "\n";
// 
// out << "\n";
// 
// out << "NFcstComps = " << NFcstComps << "\n";
// out << "NObsComps  = " << NObsComps  << "\n";

out << "\n";


out.flush();

return;

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::n_fcst_simple_objs() const

{

if ( !f )  {

   mlog << Error << "\n\n  ModeNcOutputFile::n_fcst_simple_objs() const -> no file open!\n\n";

   exit ( 1 );

}

int n;

n = count_objects(FcstObjId);

return ( n );

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::n_obs_simple_objs() const

{

if ( !f )  {

   mlog << Error << "\n\n  ModeNcOutputFile::n_obs_simple_objs() const -> no file open!\n\n";

   exit ( 1 );

}

int n;

n = count_objects(ObsObjId);

return ( n );

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::n_fcst_comp_objs() const

{

if ( !f )  {

   mlog << Error << "\n\n  ModeNcOutputFile::n_fcst_comp_objs() const -> no file open!\n\n";

   exit ( 1 );

}

int n;

n = count_objects(FcstCompId);

return ( n );

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::n_obs_comp_objs() const

{

if ( !f )  {

   mlog << Error << "\n\n  ModeNcOutputFile::n_obs_comp_objs() const -> no file open!\n\n";

   exit ( 1 );

}

int n;

n = count_objects(ObsCompId);

return ( n );

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::count_objects(NcVar * var) const

{

int x, y, n, k;

n = 0;

for (x=0; x<Nx; ++x)  {

   for (y=0; y<Ny; ++y)  {

      k = get_int(var, x, y);

      if ( k > n )  n = k;

   }

}

return ( n );

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::get_int(NcVar * var, int x, int y) const

{

if ( (x < 0) || (x >= Nx) || (y < 0) || (y >= Ny) )  {

   mlog << Error << "\n\n  ModeNcOutputFile::get_int() -> range check error ... "
        << "(" << x << ", " << y << ")"
        << "\n\n";

   exit ( 1 );

}

int i[2];
int status;

status = var->set_cur(y, x);   //  NOT (x, y)!

if ( !status )  {

   mlog << Error << "\n\n  ModeNcOutputFile::get_int() const -> can't set_cur for (" << x << ", " << y << ")!\n\n";

   exit ( 1 );

}

status = var->get(i, 1, 1);

if ( !status )  {

   mlog << Error << "\n\n  ModeNcOutputFile::get_int() const -> can't get value!\n\n";

   exit ( 1 );

}

return ( i[0] );

}


////////////////////////////////////////////////////////////////////////


double ModeNcOutputFile::get_float(NcVar * var, int x, int y) const

{

if ( (x < 0) || (x >= Nx) || (y < 0) || (y >= Ny) )  {

   mlog << Error << "\n\n  ModeNcOutputFile::get_float() -> range check error ... "
        << "(" << x << ", " << y << ")"
        << "\n\n";

   exit ( 1 );

}

float ff[2];
int status;

status = var->set_cur(y, x);   //  NOT (x, y)!

if ( !status )  {

   mlog << Error << "\n\n  ModeNcOutputFile::get_float() const -> can't set_cur for (" << x << ", " << y << ")!\n\n";

   exit ( 1 );

}

status = var->get(ff, 1, 1);

if ( !status )  {

   mlog << Error << "\n\n  ModeNcOutputFile::get_float() const -> can't get value!\n\n";

   exit ( 1 );

}

return ( (double) (ff[0]) );

}


////////////////////////////////////////////////////////////////////////


double ModeNcOutputFile::fcst_raw(int x, int y) const

{

double z;

z = get_float(FcstRaw, x, y);

return ( z );

}


////////////////////////////////////////////////////////////////////////


double ModeNcOutputFile::obs_raw(int x, int y) const

{

double z;

z = get_float(ObsRaw, x, y);

return ( z );

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::fcst_obj_id(int x, int y) const

{

int k;

k = get_int(FcstObjId, x, y);

return ( k );

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::fcst_comp_id(int x, int y) const

{

int k;

k = get_int(FcstCompId, x, y);

return ( k );

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::obs_obj_id(int x, int y) const

{

int k;

k = get_int(ObsObjId, x, y);

return ( k );

}


////////////////////////////////////////////////////////////////////////


int ModeNcOutputFile::obs_comp_id(int x, int y) const

{

int k;

k = get_int(ObsCompId, x, y);

return ( k );

}


////////////////////////////////////////////////////////////////////////


DataPlane ModeNcOutputFile::select_obj(ModeObjectField field, int n) const

{

if ( (n == 0) || (n < -1) )  {

   mlog << Error << "\n\n  ModeNcOutputFile::select_obj() const -> bad value\n\n";

   exit ( 1 );

}

if ( !f )  {

   mlog << Error << "\n\n  ModeNcOutputFile::select_obj() const -> no data!\n\n";

   exit ( 1 );

}

int x, y;
int value;
int count;
DataPlane fdata;

fdata.set_size(Nx, Ny);

count = 0;

for (x=0; x<Nx; ++x)  {

   for (y=0; y<Ny; ++y)  {

      fdata.set(0.0, x, y);


      switch ( field )  {

         case mof_fcst_obj:
            value = fcst_obj_id(x, y);
            break;

         case mof_fcst_comp:
            value = fcst_comp_id(x, y);
            break;

         case mof_obs_obj:
            value = obs_obj_id(x, y);
            break;

         case mof_obs_comp:
            value = obs_comp_id(x, y);
            break;

         default:
            mlog << Error << "\n\n  ModeNcOutputFile::select_obj() const -> bad field\n\n";
            exit ( 1 );
            break;

      }   //  switch


      if ( value < 0 )  continue;

      if ( n < 0 )  { fdata.set(1.0, x, y);  ++count;  continue; }

      if ( n == value )  { fdata.set(1.0, x, y);  ++count; }

   } 

}

// cout << "ModeNcOutputFile::select_obj() -> count = " << count << "\n" << flush;

return ( fdata );

}


////////////////////////////////////////////////////////////////////////


DataPlane ModeNcOutputFile::select_fcst_obj  (int n) const

{

DataPlane fdata;

fdata = select_obj(mof_fcst_obj, n);

return ( fdata );

}


////////////////////////////////////////////////////////////////////////


DataPlane ModeNcOutputFile::select_fcst_comp (int n) const

{

DataPlane fdata;

fdata = select_obj(mof_fcst_comp, n);

return ( fdata );

}


////////////////////////////////////////////////////////////////////////


DataPlane ModeNcOutputFile::select_obs_obj   (int n) const

{

DataPlane fdata;

fdata = select_obj(mof_obs_obj, n);

return ( fdata );

}


////////////////////////////////////////////////////////////////////////


DataPlane ModeNcOutputFile::select_obs_comp  (int n) const

{

DataPlane fdata;

fdata = select_obj(mof_obs_comp, n);

return ( fdata );

}


////////////////////////////////////////////////////////////////////////


bool ModeNcOutputFile::x_line_valid (const int x) const

{

if ( (x < 0) || (x >= Nx) )  return ( false );

int y;
double v;
const double bad = -9999.0;
const double tol = 0.01;

for (y=0; y<Ny; ++y)  {

   v = obs_raw(x, y);

   if ( fabs(v - bad) > tol )  return ( true );

}

return ( false );

}


////////////////////////////////////////////////////////////////////////


bool ModeNcOutputFile::y_line_valid (const int y) const

{

if ( (y < 0) || (y >= Ny) )  return ( false );

int x;
double v;
const double bad = -9999.0;
const double tol = 0.01;

for (x=0; x<Nx; ++x)  {

   v = obs_raw(x, y);

   if ( fabs(v - bad) > tol )  return ( true );

}

return ( false );

}


////////////////////////////////////////////////////////////////////////


void ModeNcOutputFile::calc_data_range(NcVar *, double & min_value, double & max_value)

{

int x, y;
int count;
double value;

min_value =  1.0e30;
max_value = -1.0e30;

count = 0;

for (x=0; x<Nx; ++x)  {

   for (y=0; y<Ny; ++y)  {

      value = (double) get_float(ObsRaw, x, y);

      if ( value < -9000.0 )  continue;

      ++count;

      if ( value < min_value )  min_value = value;
      if ( value > max_value )  max_value = value;

   }

}

if ( count == 0 )  {

   mlog << Error << "\n\n  ModeNcOutputFile::calc_data_range() -> no valid data points!\n\n";

   exit ( 1 );

}

return;

}


////////////////////////////////////////////////////////////////////////


void ModeNcOutputFile::get_fcst_raw_range(double & min_value, double & max_value)

{

if ( !f )  {

   mlog << Error << "\n\n  ModeNcOutputFile::get_fcst_raw_range() -> no data file open!\n\n";

   exit ( 1 );

}

if ( fcst_data_range_calculated )  {

   min_value = FcstDataMin;
   max_value = FcstDataMax;

   return;

}

calc_data_range(FcstRaw, min_value, max_value);

FcstDataMin = min_value;
FcstDataMax = max_value;

fcst_data_range_calculated = true;

return;

}


////////////////////////////////////////////////////////////////////////


void ModeNcOutputFile::get_obs_raw_range(double & min_value, double & max_value)

{

if ( !f )  {

   mlog << Error << "\n\n  ModeNcOutputFile::get_obs_raw_range() -> no data file open!\n\n";

   exit ( 1 );

}

if ( obs_data_range_calculated )  {

   min_value = ObsDataMin;
   max_value = ObsDataMax;

   return;

}

calc_data_range(ObsRaw, min_value, max_value);

ObsDataMin = min_value;
ObsDataMax = max_value;

obs_data_range_calculated = true;

return;

}


////////////////////////////////////////////////////////////////////////


ConcatString ModeNcOutputFile::short_filename() const

{

if ( Filename.empty() )  {

   mlog << Error << "\n\n  ModeNcOutputFile::short_filename() const -> no filename set!\n\n";

   exit ( 1 );

}

ConcatString s;

s = get_short_name(Filename);

return ( s );

}


////////////////////////////////////////////////////////////////////////




