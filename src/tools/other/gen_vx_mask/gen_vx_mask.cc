// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2015
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

////////////////////////////////////////////////////////////////////////
//
//   Filename:   gen_vx_mask.cc
//
//   Description:
//
//   Mod#   Date      Name            Description
//   ----   ----      ----            -----------
//   000    12/09/14  Halley Gotway   New
//   001    05/25/16  Halley Gotway   Allow -value to be set negative
//
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

#include "gen_vx_mask.h"

#include "netcdf.hh"
#include "grib_classes.h"

#include "vx_log.h"
#include "nav.h"
#include "vx_math.h"
#include "vx_nc_util.h"
#include "data2d_nc_met.h"

////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
   static DataPlane dp_data, dp_mask, dp_out;

   // Set handler to be called for memory allocation error
   set_new_handler(oom);

   // Process the command line arguments
   process_command_line(argc, argv);

   // Process the data file and extract the grid information
   process_input_file(dp_data);

   // Process the mask file
   process_mask_file(dp_mask);

   // Apply combination logic if the current mask is binary
   if(mask_type == MaskType_Poly ||
      mask_type == MaskType_Grid ||
      thresh.get_type() != thresh_na) {
      dp_out = combine(dp_data, dp_mask, set_logic);
   }
   // Otherwise, pass through the distance or raw values
   else {
      dp_out = dp_mask;
   }


   // Write out the mask file to NetCDF
   write_netcdf(dp_out);

   return(0);
}

////////////////////////////////////////////////////////////////////////

void process_command_line(int argc, char **argv) {
   CommandLine cline;

   // Check for zero arguments
   if(argc == 1) usage();

   // Initialize the configuration object
   config.read(replace_path(config_const_filename));

   // Parse the command line into tokens
   cline.set(argc, argv);

   // Set the usage function
   cline.set_usage(usage);

   // Add the options function calls
   cline.add(set_type,         "-type",         1);
   cline.add(set_input_field,  "-input_field",  1);
   cline.add(set_mask_field,   "-mask_field",   1);
   cline.add(set_complement,   "-complement",   0);
   cline.add(set_union,        "-union",        0);
   cline.add(set_intersection, "-intersection", 0);
   cline.add(set_symdiff,      "-symdiff",      0);
   cline.add(set_thresh,       "-thresh",       1);
   cline.add(set_value,        "-value",        1);
   cline.add(set_name,         "-name",         1);
   cline.add(set_logfile,      "-log",          1);
   cline.add(set_verbosity,    "-v",            1);

   cline.allow_numbers();

   // Parse the command line
   cline.parse();

   // There should be two arguments left, the input/output file names
   if(cline.n() != 3) usage();

   // Store the arguments
   input_filename = cline[0];
   mask_filename  = cline[1];
   out_filename   = cline[2];

   // List the input files
   mlog << Debug(1)
        << "Input File:\t\t" << input_filename << "\n"
        << "Mask File:\t\t"  << mask_filename  << "\n";

   return;
}

////////////////////////////////////////////////////////////////////////

void process_input_file(DataPlane &dp) {
   Met2dDataFileFactory mtddf_factory;
   Met2dDataFile *mtddf_ptr = (Met2dDataFile *) 0;
   GrdFileType ftype = FileType_None;

   // Get the gridded file type from the data config string, if present
   if(input_field_str.length() > 0) {
      config.read_string(input_field_str);
      ftype = parse_conf_file_type(&config);
   }

   mtddf_ptr = mtddf_factory.new_met_2d_data_file(input_filename, ftype);
   if(!mtddf_ptr) {
      mlog << Error << "\nprocess_input_file() -> "
           << "can't open input file \"" << input_filename << "\"\n\n";
      exit(1);
   }

   // Extract the grid
   grid = mtddf_ptr->grid();

   // Read the input data plane, if requested
   if(input_field_str.length() > 0) {
      get_data_plane(mtddf_ptr, input_field_str, dp);
   }
   // Check for the output of a previous call to this tool
   else if(get_gen_vx_mask_data(mtddf_ptr, dp)) {
   }
   // Otherwise, fill the input data plane with zeros
   else {
      dp.set_size(grid.nx(), grid.ny());
      dp.set_constant(0.0);
   }

   mlog << Debug(2)
        << "Parsed Data Grid:\t" << grid.name()
        << " (" << grid.nx() << " x " << grid.ny() << ")\n";

   // Clean up
   if(mtddf_ptr) { delete mtddf_ptr; mtddf_ptr = (Met2dDataFile *) 0; }

   return;
}

////////////////////////////////////////////////////////////////////////

void process_mask_file(DataPlane &dp) {
   Met2dDataFileFactory mtddf_factory;
   Met2dDataFile *mtddf_ptr = (Met2dDataFile *) 0;
   GrdFileType ftype = FileType_None;

   // Process the mask file as a lat/lon polyline file
   if(mask_type == MaskType_Poly ||
      mask_type == MaskType_Circle ||
      mask_type == MaskType_Track) {

      poly_mask.clear();
      poly_mask.load(mask_filename);

      mlog << Debug(2)
           << "Parsed Mask Polyline:\t" << poly_mask.name()
           << " containing " << poly_mask.n_points() << " points\n";
   }
   // Otherwise, process the mask file as a gridded data file
   else {

      // Get the gridded file type from the mask config string, if present
      if(mask_field_str.length() > 0) {
         config.read_string(mask_field_str);
         ftype = parse_conf_file_type(&config);
      }

      mtddf_ptr = mtddf_factory.new_met_2d_data_file(mask_filename, ftype);
      if(!mtddf_ptr) {
         mlog << Error << "\nprocess_mask_file() -> "
              << "can't open gridded mask data file \"" << mask_filename << "\"\n\n";
         exit(1);
      }

      // Extract the grid
      grid_mask = mtddf_ptr->grid();

      mlog << Debug(2)
           << "Parsed Mask Grid:\t" << grid_mask.name()
           << " (" << grid_mask.nx() << " x " << grid_mask.ny() << ")\n";

      // Check for matching grids
      if(mask_type == MaskType_Data && grid != grid_mask) {
         mlog << Error << "\nprocess_mask_file() -> "
              << "The data grid and mask grid must be identical for \"data\" masking.\n"
              << "Data Grid -> " << grid.serialize() << "\n"
              << "Mask Grid -> " << grid_mask.serialize() << "\n\n";
         exit(1);
      }
   }

   // Read the mask data plane, if requested
   if(mask_type == MaskType_Data) {
      if(mask_field_str.length() == 0) {
         mlog << Error << "\nprocess_mask_file() -> "
              << "use \"-mask_field\" to specify the field for \"data\" masking.\n\n";
         exit(1);
      }
      get_data_plane(mtddf_ptr, mask_field_str, dp);
   }
   // Otherwise, initialize the masking data
   else {
      dp.set_size(grid.nx(), grid.ny());
   }

   // Construct the mask
   switch(mask_type) {
      case MaskType_Poly:
         apply_poly_mask(dp);
         break;
      case MaskType_Circle:
         apply_circle_mask(dp);
         break;
      case MaskType_Track:
         apply_track_mask(dp);
         break;
      case MaskType_Grid:
         apply_grid_mask(dp);
         break;
      case MaskType_Data:
         apply_data_mask(dp);
         break;
      default:
         mlog << Error << "\nprocess_mask_file() -> "
              << "Unxpected MaskType value (" << mask_type << ")\n\n";
         break;
   }

   // Clean up
   if(mtddf_ptr) { delete mtddf_ptr; mtddf_ptr = (Met2dDataFile *) 0; }

   return;
}

////////////////////////////////////////////////////////////////////////

static void get_data_plane(Met2dDataFile *mtddf_ptr,
                           const char *config_str, DataPlane &dp) {
   VarInfoFactory vi_factory;
   VarInfo *vi_ptr = (VarInfo *) 0;
   double dmin, dmax;

   // Parse the config string
   config.read_string(config_str);

   // Allocate new VarInfo object
   vi_ptr = vi_factory.new_var_info(mtddf_ptr->file_type());
   if(!vi_ptr) {
      mlog << Error << "\nget_data_plane() -> "
           << "can't allocate new VarInfo pointer.\n\n";
      exit(1);
   }

   // Read config into the VarInfo object
   vi_ptr->set_dict(config);

   // Get data plane from the file for this VarInfo object
   if(!mtddf_ptr->data_plane(*vi_ptr, dp)) {
      mlog << Error << "\nget_data_plane() -> "
           << "trouble reading field \"" << config_str
           << "\" from file \"" << mtddf_ptr->filename() << "\"\n\n";
      exit(1);
   }

   // Dump the range of data values read
   dp.data_range(dmin, dmax);
   mlog << Debug(3)
        << "Read field \"" << vi_ptr->magic_str() << "\" from \""
        << mtddf_ptr->filename() << "\" with data ranging from "
        << dmin << " to " << dmax << ".\n";

   // Clean up
   if(vi_ptr) { delete vi_ptr; vi_ptr = (VarInfo *) 0; }

   return;
}

////////////////////////////////////////////////////////////////////////

bool get_gen_vx_mask_data(Met2dDataFile *mtddf_ptr, DataPlane &dp) {
   bool status = false;
   ConcatString tool, config_str;
   int i;

   // Must be MET NetCDF format
   if(mtddf_ptr->file_type() != FileType_NcMet) return(status);

   // Cast pointer of correct type
   MetNcMetDataFile *mnmdf_ptr = (MetNcMetDataFile *) mtddf_ptr;

   // Check for the MET_tool global attribute
   if(!get_global_att(mnmdf_ptr->MetNc->Nc, "MET_tool", tool)) return(status);

   // Check for gen_vx_mask output
   if(tool != program_name) return(status);

   // Loop through the NetCDF variables
   for(i=0; i<mnmdf_ptr->MetNc->Nvars; i++) {

      // Skip the lat/lon variables
      if(mnmdf_ptr->MetNc->Var[i].name == "lat" ||
         mnmdf_ptr->MetNc->Var[i].name == "lon") continue;

      // Read the first non-lat/lon variable
      config_str << "'name=\"" << mnmdf_ptr->MetNc->Var[i].name << "\"; level=\"(*,*)\";'";
      get_data_plane(mtddf_ptr, config_str, dp);
      status = true;
      break;
   }

   return(status);
}

////////////////////////////////////////////////////////////////////////

static void apply_poly_mask(DataPlane &dp) {
   int x, y, n_in;
   bool inside;
   double lat, lon;

   // Check each grid point being inside the polyline
   for(x=0,n_in=0; x<grid.nx(); x++) {
      for(y=0; y<grid.ny(); y++) {

         // Lat/Lon value for the current grid point
         grid.xy_to_latlon(x, y, lat, lon);
         lon -= 360.0*floor((lon + 180.0)/360.0);

         // Check current grid point inside polyline
         // converting from degrees_east to degrees_west
         inside = poly_mask.latlon_is_inside(lat, -1.0*lon);

         // Check the complement
         if(complement) inside = !inside;

         // Increment count
         n_in += inside;

         // Store the current mask value
         dp.set(inside, x, y);
      } // end for y
   } // end for x

   if(complement) {
      mlog << Debug(3)
           << "Applying complement of polyline mask.\n";
   }

   // List number of points inside the mask
   mlog << Debug(3)
        << "Polyline Masking:\t" << n_in << " of " << grid.nx() * grid.ny()
        << " points inside\n";

   return;
}

////////////////////////////////////////////////////////////////////////

static void apply_circle_mask(DataPlane &dp) {
   int x, y, i, n_in;
   double lat, lon, dist, v;
   bool check;

   // Check for no threshold
   if(thresh.get_type() == thresh_na) {
      mlog << Warning
           << "\napply_circle_mask() -> since \"-thresh\" was not used "
           << "to specify a threshold for circle masking, the minimum "
           << "distance to the points will be written.\n\n";
   }

   // For each grid point, compute mimumum distance to polyline points
   for(x=0,n_in=0; x<grid.nx(); x++) {
      for(y=0; y<grid.ny(); y++) {

         // Lat/Lon value for the current grid point
         grid.xy_to_latlon(x, y, lat, lon);
         lon -= 360.0*floor((lon + 180.0)/360.0);

         // Find the minimum distance to a polyline point
         for(i=0, dist=1.0E10; i<poly_mask.n_points(); i++) {
            dist = min(dist, gc_dist(lat, lon, poly_mask.lat(i), poly_mask.lon(i)));
         }

         // Apply threshold, if specified
         if(thresh.get_type() != thresh_na) {
            check = thresh.check(dist);

            // Check the complement
            if(complement) check = !check;

            // Increment count
            n_in += check;

            v = (check ? 1.0 : 0.0);
         }
         else {
            v = dist;
         }

         // Store the result
         dp.set(v, x, y);
      } // end for y
   } // end for x

   if(thresh.get_type() != thresh_na && complement) {
      mlog << Debug(3)
           << "Applying complement of circle mask.\n";
   }

   // List the number of points inside the mask
   if(thresh.get_type() != thresh_na) {
      mlog << Debug(3)
           << "Circle Masking:\t" << n_in << " of " << grid.nx() * grid.ny()
           << " points inside\n";
   }
   // Otherwise, list the min/max distances computed
   else {
      double dmin, dmax;
      dp.data_range(dmin, dmax);
      mlog << Debug(3)
           << "Circle Masking:\tDistances ranging from "
           << dmin << " km to " << dmax << " km\n";
   }

   return;
}

////////////////////////////////////////////////////////////////////////

static void apply_track_mask(DataPlane &dp) {
   int x, y, i, n_in;
   double lat, lon, dist, v;
   bool check;

   // Check for no threshold
   if(thresh.get_type() == thresh_na) {
      mlog << Warning
           << "\napply_track_mask() -> since \"-thresh\" was not used "
           << "to specify a threshold for track masking, the minimum "
           << "distance to the track will be written.\n\n";
   }

   // For each grid point, compute mimumum distance to track
   for(x=0,n_in=0; x<grid.nx(); x++) {
      for(y=0; y<grid.ny(); y++) {

         // Lat/Lon value for the current grid point
         grid.xy_to_latlon(x, y, lat, lon);
         lon -= 360.0*floor((lon + 180.0)/360.0);

         // Find the minimum distance to the track
         for(i=1, dist=1.0E10; i<poly_mask.n_points(); i++) {
            dist = min(dist,
                       gc_dist_to_line(poly_mask.lat(i-1), poly_mask.lon(i-1),
                                       poly_mask.lat(i),   poly_mask.lon(i),
                                       lat, lon));
         }

         // Apply threshold, if specified
         if(thresh.get_type() != thresh_na) {
            check = thresh.check(dist);

            // Check the complement
            if(complement) check = !check;

            // Increment count
            n_in += check;

            v = (check ? 1.0 : 0.0);
         }
         else {
            v = dist;
         }

         // Store the result
         dp.set(v, x, y);
      } // end for y
   } // end for x

   if(thresh.get_type() != thresh_na && complement) {
      mlog << Debug(3)
           << "Applying complement of track mask.\n";
   }

   // List the number of points inside the mask
   if(thresh.get_type() != thresh_na) {
      mlog << Debug(3)
           << "Track Masking:\t\t" << n_in << " of " << grid.nx() * grid.ny()
           << " points inside\n";
   }
   // Otherwise, list the min/max distances computed
   else {
      double dmin, dmax;
      dp.data_range(dmin, dmax);
      mlog << Debug(3)
           << "Track Masking:\t\tDistances ranging from "
           << dmin << " km to " << dmax << " km\n";
   }

   return;
}

////////////////////////////////////////////////////////////////////////

static void apply_grid_mask(DataPlane &dp) {
   int x, y, n_in;
   bool inside;
   double lat, lon, mask_x, mask_y;

   // Check each grid point being inside the polyline
   for(x=0,n_in=0; x<grid.nx(); x++) {
      for(y=0; y<grid.ny(); y++) {

         // Lat/Lon value for the current grid point
         grid.xy_to_latlon(x, y, lat, lon);
         lon -= 360.0*floor((lon + 180.0)/360.0);

         // Convert Lat/Lon to masking grid x/y
         grid_mask.latlon_to_xy(lat, lon, mask_x, mask_y);

         // Check for point falling within the masking grid
         inside = (mask_x >= 0 && mask_x < grid_mask.nx() &&
                   mask_y >= 0 && mask_y < grid_mask.ny());

         // Apply the complement
         if(complement) inside = !inside;

         // Increment count
         n_in += inside;

         // Store the current mask value
         dp.set(inside, x, y);
      } // end for y
   } // end for x

   if(complement) {
      mlog << Debug(3)
           << "Applying complement of grid mask.\n";
   }

   mlog << Debug(3)
        << "Grid Masking:\t\t" << n_in << " of " << grid.nx() * grid.ny()
        << " points inside\n";

   return;
}


////////////////////////////////////////////////////////////////////////

static void apply_data_mask(DataPlane &dp) {
   int x, y, n_in;
   bool check;

   // Nothing to do without a threshold
   if(thresh.get_type() == thresh_na) {
      double dmin, dmax;
      dp.data_range(dmin, dmax);
      mlog << Debug(3)
           << "Data Masking:\t\tValues ranging from "
           << dmin << " km to " << dmax << " km\n";
      mlog << Warning
           << "\napply_data_mask() -> since \"-thresh\" was not used "
           << "to specify a threshold for data masking, the raw data "
           << "values will be written.\n\n";
      return;
   }

   // For each grid point, apply the data threshold
   for(x=0,n_in=0; x<grid.nx(); x++) {
      for(y=0; y<grid.ny(); y++) {

         // Apply the threshold
         check = thresh.check(dp(x,y));

         // Check the complement
         if(complement) check = !check;

         // Increment count
         n_in += check;

         // Store the result
         dp.set(check ? 1.0 : 0.0, x, y);
      } // end for y
   } // end for x

   // List the number of points inside the mask
   mlog << Debug(3)
        << "Data Masking:\t\t" << n_in << " of " << grid.nx() * grid.ny()
        << " points inside\n";

   return;
}

////////////////////////////////////////////////////////////////////////

DataPlane combine(const DataPlane &dp_data, const DataPlane &dp_mask,
                  SetLogic logic) {
   int x, y, n_in;
   bool v_data, v_mask;
   double v;
   DataPlane dp;

   // Check that the input data planes have the same dimension
   if(dp_data.nx() != dp_mask.nx() || dp_data.ny() != dp_mask.ny()) {
      mlog << Error << "\ncombine() -> "
           << "dimensions of input data planes do not match: ("
           << dp_data.nx() << ", " << dp_data.ny() << ") != ("
           << dp_mask.nx() << ", " << dp_mask.ny() << ")\n\n";
      exit(1);
   }

   // Set the output data plane size
   dp.set_size(grid.nx(), grid.ny());

   // Process each point
   for(x=0,n_in=0; x<grid.nx(); x++) {
      for(y=0; y<grid.ny(); y++) {

         // Get the two input data values
         v_data = !is_eq(dp_data(x, y), 0.0);
         v_mask = !is_eq(dp_mask(x, y), 0.0);

         switch(logic) {

            case SetLogic_Union:
               if(v_data || v_mask) v = mask_val;
               else                 v = 0.0;
               break;

            case SetLogic_Intersection:
               if(v_data && v_mask) v = mask_val;
               else                 v = 0.0;
               break;

            case SetLogic_SymDiff:
               if((v_data && !v_mask) || (!v_data && v_mask)) v = mask_val;
               else                                           v = 0.0;
               break;

            // Default behavior is to apply the mask value or pass through
            // the data value
            default:
               if(v_mask) v = mask_val;
               else       v = dp_data(x, y);
               break;
         }

         // Increment count
         n_in += !is_eq(v, 0.0);

         // Store the result
         dp.set(v, x, y);

      } // end for y
   } // end for x

   // List the number of points inside the mask
   if(logic != SetLogic_None) {
      mlog << Debug(3)
           << "Mask " << setlogic_to_string(logic) << ": " << n_in
           << " of " << grid.nx() * grid.ny() << " points inside\n";
   }

   return(dp);
}

////////////////////////////////////////////////////////////////////////

void write_netcdf(const DataPlane &dp) {
   int n, x, y;
   ConcatString cs;

   float *mask_data = (float *)  0;
   NcFile *f_out    = (NcFile *) 0;
   NcDim *lat_dim   = (NcDim *)  0;
   NcDim *lon_dim   = (NcDim *)  0;
   NcVar *mask_var  = (NcVar *)  0;

   // Create a new NetCDF file and open it.
   f_out = new NcFile(out_filename, NcFile::Replace);

   if(!f_out->is_valid()) {
      mlog << Error << "\nwrite_netcdf() -> "
           << "trouble opening output file " << out_filename
           << "\n\n";
      f_out->close();
      delete f_out;
      f_out = (NcFile *) 0;
      exit(1);
   }

   // Add global attributes
   write_netcdf_global(f_out, out_filename, program_name);

   // Add the projection information
   write_netcdf_proj(f_out, grid);

   // Define Dimensions
   lat_dim = f_out->add_dim("lat", (long) grid.ny());
   lon_dim = f_out->add_dim("lon", (long) grid.nx());

   // Add the lat/lon variables
   write_netcdf_latlon(f_out, lat_dim, lon_dim, grid);

   // Set the mask_name, if not already set
   if(mask_name.length() == 0) {
      if(mask_type == MaskType_Poly   ||
         mask_type == MaskType_Circle ||
         mask_type == MaskType_Track) {
         mask_name = poly_mask.name();
      }
      else {
         mask_name << masktype_to_string(mask_type) << "_mask";
      }
   }

   // Define Variables
   mask_var = f_out->add_var(mask_name, ncFloat, lat_dim, lon_dim);
   cs << cs_erase << mask_name << " masking region";
   mask_var->add_att("long_name", cs);
   mask_var->add_att("_FillValue", bad_data_float);
   cs << cs_erase << masktype_to_string(mask_type);
   if(thresh.get_type() != thresh_na) cs << thresh.get_str();
   mask_var->add_att("mask_type", cs);

   // Allocate memory to store the mask values for each grid point
   mask_data = new float [grid.nx()*grid.ny()];

   // Loop through each grid point
   for(x=0; x<grid.nx(); x++) {
      for(y=0; y<grid.ny(); y++) {
         n = DefaultTO.two_to_one(grid.nx(), grid.ny(), x, y);
         mask_data[n] = dp(x, y);
      } // end for y
   } // end for x

   if(!mask_var->put(&mask_data[0], grid.ny(), grid.nx())) {
      mlog << Error << "\nwrite_netcdf() -> "
           << "error with mask_var->put\n\n";
      exit(1);
   }

   // Delete allocated memory
   if(mask_data) { delete mask_data; mask_data = (float *) 0; }

   f_out->close();
   delete f_out;
   f_out = (NcFile *) 0;

   mlog << Debug(1)
        << "Output File:\t\t" << out_filename << "\n";

   return;
}

////////////////////////////////////////////////////////////////////////

MaskType string_to_masktype(const char *s) {
   MaskType t;

        if(strcasecmp(s, "poly")   == 0) t = MaskType_Poly;
   else if(strcasecmp(s, "circle") == 0) t = MaskType_Circle;
   else if(strcasecmp(s, "track")  == 0) t = MaskType_Track;
   else if(strcasecmp(s, "grid")   == 0) t = MaskType_Grid;
   else if(strcasecmp(s, "data")   == 0) t = MaskType_Data;
   else {
      mlog << Error << "\nstring_to_masktype() -> "
           << "unsupported masking type \"" << s << "\"\n\n";
      exit(1);
   }

   return(t);
}

////////////////////////////////////////////////////////////////////////

const char * masktype_to_string(const MaskType t) {
   const char *s = (const char *) 0;

   switch(t) {
      case MaskType_Poly:   s = "poly";           break;
      case MaskType_Circle: s = "circle";         break;
      case MaskType_Track:  s = "track";          break;
      case MaskType_Grid:   s = "grid";           break;
      case MaskType_Data:   s = "data";           break;
      case MaskType_None:   s = na_str;           break;
      default:              s = (const char *) 0; break;
   }

   return(s);
}

////////////////////////////////////////////////////////////////////////

void usage() {

   cout << "\n*** Model Evaluation Tools (MET" << met_version
        << ") ***\n\n"

        << "Usage: " << program_name << "\n"
        << "\tinput_file\n"
        << "\tmask_file\n"
        << "\tout_file\n"
        << "\t[-type string]\n"
        << "\t[-input_field string]\n"
        << "\t[-mask_field string]\n"
        << "\t[-complement]\n"
        << "\t[-union|-intersection|-symdiff]\n"
        << "\t[-thresh string]\n"
        << "\t[-value n]\n"
        << "\t[-name string]\n"
        << "\t[-log file]\n"
        << "\t[-v level]\n\n"

        << "\twhere\t\"input_file\" is the input gridded data file "
        << "(required).\n"
        << "\t\t   If " << program_name << " output, automatically read mask data.\n"

        << "\t\t\"mask_file\" defines the masking region (required).\n"
        << "\t\t   ASCII Lat/Lon file for \"poly\", \"circle\", and \"track\" masking.\n"
        << "\t\t   Gridded data file for \"grid\" and \"data\" masking.\n"

        << "\t\t\"out_file\" is the output NetCDF mask file to be "
        << "written (required).\n"

        << "\t\t\"-type string\" overrides the default masking type ("
        << masktype_to_string(default_mask_type) << ") (optional).\n"
        << "\t\t   May be set to \"poly\", \"circle\", \"track\", \"grid\", or \"data\".\n"

        << "\t\t\"-input_field string\" to read existing mask data from \"input_file\" (optional).\n"

        << "\t\t\"-mask_field string\" to define the field from \"mask_file\" to be used for \"data\" masking (optional).\n"

        << "\t\t\"-complement\" to compute the complement of the area defined in \"mask_file\" (optional).\n"

        << "\t\t\"-union | -intersection | -symdiff\" to specify how to combine the "
        << "masks from \"input_file\" and \"mask_file\" (optional).\n"

        << "\t\t\"-thresh string\" defines the threshold to be applied (optional).\n"
        << "\t\t   Distance (km) for \"circle\" and \"track\" masking.\n"
        << "\t\t   Raw input values for \"data\" masking.\n"

        << "\t\t\"-value n\" overrides the default output mask data value ("
        << default_mask_val << ") (optional).\n"

        << "\t\t\"-name string\" specifies the output variable name for the mask"
        << " (optional).\n"

        << "\t\t\"-log file\" outputs log messages to the specified "
        << "file (optional).\n"

        << "\t\t\"-v level\" overrides the default level of logging ("
        << mlog.verbosity_level() << ") (optional).\n\n"

        << flush;

   exit (1);
}

////////////////////////////////////////////////////////////////////////

void set_type(const StringArray & a) {
   mask_type = string_to_masktype(a[0]);
}

////////////////////////////////////////////////////////////////////////

void set_input_field(const StringArray & a) {
   input_field_str = a[0];
}

////////////////////////////////////////////////////////////////////////

void set_mask_field(const StringArray & a) {
   mask_field_str = a[0];
}

////////////////////////////////////////////////////////////////////////

void set_complement(const StringArray & a) {
   complement = true;
}

////////////////////////////////////////////////////////////////////////

void set_union(const StringArray & a) {
   set_logic = SetLogic_Union;
}

////////////////////////////////////////////////////////////////////////

void set_intersection(const StringArray & a) {
   set_logic = SetLogic_Intersection;
}

////////////////////////////////////////////////////////////////////////

void set_symdiff(const StringArray & a) {
   set_logic = SetLogic_SymDiff;
}

////////////////////////////////////////////////////////////////////////

void set_thresh(const StringArray & a) {
   thresh.set(a[0]);
}

////////////////////////////////////////////////////////////////////////

void set_value(const StringArray & a) {
   mask_val = atof(a[0]);
}

////////////////////////////////////////////////////////////////////////

void set_name(const StringArray & a) {
   mask_name = a[0];
}

////////////////////////////////////////////////////////////////////////

void set_logfile(const StringArray & a) {
   ConcatString filename;

   filename = a[0];

   mlog.open_log_file(filename);
}

////////////////////////////////////////////////////////////////////////

void set_verbosity(const StringArray & a) {
   mlog.set_verbosity_level(atoi(a[0]));
}

////////////////////////////////////////////////////////////////////////
