

////////////////////////////////////////////////////////////////////////


#ifndef  __MERCATOR_GRID_DEFINITIONS_H__
#define  __MERCATOR_GRID_DEFINITIONS_H__


////////////////////////////////////////////////////////////////////////


struct MercatorData {

   const char * name;

   double lat_ll_deg;
   double lon_ll_deg;

   double lat_ur_deg;
   double lon_ur_deg;

   // double delta_i;
   // double delta_j;

   int nx;
   int ny;

};


////////////////////////////////////////////////////////////////////////


#endif   /*  __MERCATOR_GRID_DEFINITIONS_H__  */


////////////////////////////////////////////////////////////////////////



