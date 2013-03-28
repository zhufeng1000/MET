

   // *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
   // ** Copyright UCAR (c) 1992 - 2012
   // ** University Corporation for Atmospheric Research (UCAR)
   // ** National Center for Atmospheric Research (NCAR)
   // ** Research Applications Lab (RAL)
   // ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
   // *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*




////////////////////////////////////////////////////////////////////////


using namespace std;


#include <iostream>

#include "vx_log.h"
#include "vx_math.h"

#include "met_handler.h"


static const int   n_met_col     = 10;
static const int   n_met_col_qty = 11;

////////////////////////////////////////////////////////////////////////


   //
   //  Code for class MetHandler
   //


////////////////////////////////////////////////////////////////////////


MetHandler::MetHandler(const string &program_name) :
  FileHandler(program_name),
  _nFileColumns(0)
{
}


////////////////////////////////////////////////////////////////////////


MetHandler::~MetHandler()
{
}


////////////////////////////////////////////////////////////////////////


bool MetHandler::isFileType(LineDataFile &ascii_file) const
{
  //
  // Initialize the return value.
  //
  bool is_file_type = false;
  
  //
  // Read the first line from the file
  //
  DataLine dl;
  ascii_file >> dl;

  //
  // Check for expected number of MET columns
  //

  if (dl.n_items() == n_met_col || dl.n_items() == n_met_col_qty)
    is_file_type = true;
   
  return is_file_type;
}
  

////////////////////////////////////////////////////////////////////////

bool MetHandler::_readObservations(LineDataFile &ascii_file)
{
  // Process each line of the file

  DataLine data_line;

  while (ascii_file >> data_line)
  {
    ConcatString hdr_typ;
    ConcatString hdr_sid;
    ConcatString hdr_vld_str;
    double hdr_lat = 0.0;
    double hdr_lon = 0.0;
    double hdr_elv = 0.0;
    
    time_t hdr_vld = 0;
    
    // Check for the first line of the file or the header changing

    if (data_line.line_number()  == 1       ||
	hdr_typ           != data_line[0]   ||
	hdr_sid           != data_line[1]   ||
	hdr_vld_str       != data_line[2]   ||
	!is_eq(hdr_lat, atof(data_line[3])) ||
	!is_eq(hdr_lon, atof(data_line[4])) ||
	!is_eq(hdr_elv, atof(data_line[5])))
    {
      // Store the column format

      _nFileColumns = data_line.n_items();
      if (data_line.line_number() == 1 &&
          _nFileColumns == n_met_col)
      {
	mlog << Debug(1) << "Found deprecated 10 column input file format, "
	     << "consider adding quality flag values to file: "
	     << ascii_file.filename() << "\n";
      }

      // Store the header info

      hdr_typ =      data_line[0];
      hdr_sid =      data_line[1];
      hdr_vld_str =  data_line[2];
      hdr_lat = atof(data_line[3]);
      hdr_lon = atof(data_line[4]);
      hdr_elv = atof(data_line[5]);

      hdr_vld = _getValidTime(hdr_vld_str.text());
      
    }

    // Pressure level (hPa) or precip accumulation interval (sec)

    double obs_prs = (is_precip_grib_code(atoi(data_line[6])) ?
		      timestring_to_sec(data_line[7]) : atof(data_line[7]));

    // Observation height (meters above sea level)

    double obs_hgt = atof(data_line[8]);

    // Observation quality

    ConcatString obs_qty = (_nFileColumns == n_met_col ? "NA" : data_line[9]);

    // Observation value

    int obs_idx = (_nFileColumns == n_met_col ? 9 : 10);
    double obs_val = atof(data_line[obs_idx]);
             
    // Save the observation info

    _observations.push_back(Observation(hdr_typ.text(),
					hdr_sid.text(),
					hdr_vld,
					hdr_lat, hdr_lon, hdr_elv,
					obs_qty.text(),
					atoi(data_line[6]),
					obs_prs, obs_hgt, obs_val));
  } // end while

  return true;
}
  
