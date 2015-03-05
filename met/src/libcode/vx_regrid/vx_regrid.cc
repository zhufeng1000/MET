// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2015
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*



////////////////////////////////////////////////////////////////////////


using namespace std;

#include "vx_regrid.h"

#include "interp_mthd.h"


////////////////////////////////////////////////////////////////////////


DataPlane met_regrid (const DataPlane & in, const Grid & from_grid, const Grid & to_grid, const RegridInfo & info)

{

DataPlane out;


switch ( info.method )  {

   case InterpMthd_Bilin:
      out = met_regrid_bilinear (in, from_grid, to_grid, info);
      break;

   case InterpMthd_NearestNbr:
      out = met_regrid_nearest_nbr (in, from_grid, to_grid, info);
      break;

   case InterpMthd_Budget:
      out = met_regrid_budget (in, from_grid, to_grid, info);
      break;

   default:
      mlog << Error
           << "met_regrid() -> bad interpolation method ... "
           << interpmthd_to_string(info.method) 
           << '\n';
      break;

}   //  switch info.method

   //
   //  done
   //

return ( out );

}


////////////////////////////////////////////////////////////////////////


