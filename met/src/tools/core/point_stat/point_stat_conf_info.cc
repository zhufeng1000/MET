// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2017
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

////////////////////////////////////////////////////////////////////////

using namespace std;

#include <dirent.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <cmath>

#include "point_stat_conf_info.h"
#include "vx_data2d_factory.h"
#include "vx_data2d.h"
#include "vx_log.h"

extern bool use_var_id;

////////////////////////////////////////////////////////////////////////
//
//  Code for class PointStatConfInfo
//
////////////////////////////////////////////////////////////////////////

PointStatConfInfo::PointStatConfInfo() {

   init_from_scratch();
}

////////////////////////////////////////////////////////////////////////

PointStatConfInfo::~PointStatConfInfo() {

   clear();
}

////////////////////////////////////////////////////////////////////////

void PointStatConfInfo::init_from_scratch() {

   // Initialize pointers
   vx_opt = (PointStatVxOpt *) 0;

   clear();

   return;
}

////////////////////////////////////////////////////////////////////////

void PointStatConfInfo::clear() {
   int i;

   // Initialize values
   model.clear();
   msg_typ_group_map.clear();
   msg_typ_sfc.clear();
   mask_dp_map.clear();
   mask_sid_map.clear();
   tmp_dir.clear();
   output_prefix.clear();
   version.clear();

   // Deallocate memory
   if(vx_opt) { delete [] vx_opt; vx_opt = (PointStatVxOpt *) 0; }

   // Set count to zero
   n_vx = 0;

   return;
}

////////////////////////////////////////////////////////////////////////

void PointStatConfInfo::read_config(const char *default_file_name,
                                    const char *user_file_name) {

   // Read the config file constants
   conf.read(replace_path(config_const_filename));

   // Read the default config file
   conf.read(default_file_name);

   // Read the user-specified config file
   conf.read(user_file_name);

   return;
}

////////////////////////////////////////////////////////////////////////

void PointStatConfInfo::process_config(GrdFileType ftype,
        bool use_var_id) {
   int i, j, n_fvx, n_ovx;
   Dictionary *fdict = (Dictionary *) 0;
   Dictionary *odict = (Dictionary *) 0;
   Dictionary i_fdict, i_odict;

   // Dump the contents of the config file
   if(mlog.verbosity_level() >= 5) conf.dump(cout);

   // Initialize
   clear();

   // Conf: version
   version = parse_conf_version(&conf);

   // Conf: model
   model = parse_conf_string(&conf, conf_key_model);

   // Conf: tmp_dir
   tmp_dir = parse_conf_tmp_dir(&conf);

   // Conf: output_prefix
   output_prefix = conf.lookup_string(conf_key_output_prefix);

   // Conf: message_type_group_map
   msg_typ_group_map = parse_conf_message_type_group_map(&conf);

   // Conf: message_type_group_map(SURFACE)
   if(msg_typ_group_map.count(surface_msg_typ_group_str) == 0) {
      mlog << Error << "\nPointStatConfInfo::process_config() -> "
           << "\"" << conf_key_message_type_group_map
           << "\" must contain an entry for \""
           << surface_msg_typ_group_str << "\".\n\n";
      exit(1);
   }
   else {
      msg_typ_sfc = msg_typ_group_map[surface_msg_typ_group_str];
   }

   // Conf: fcst.field and obs.field
   fdict = conf.lookup_array(conf_key_fcst_field);
   odict = conf.lookup_array(conf_key_obs_field);

   // Determine the number of fields (name/level) to be verified
   n_fvx = parse_conf_n_vx(fdict);
   n_ovx = parse_conf_n_vx(odict);

   // Check for a valid number of verification tasks
   if(n_fvx == 0 || n_fvx != n_ovx) {
      mlog << Error << "\nPointStatConfInfo::process_config() -> "
           << "The number of verification tasks in \""
           << conf_key_obs_field << "\" (" << n_ovx
           << ") must be non-zero and match the number in \""
           << conf_key_fcst_field << "\" (" << n_fvx << ").\n\n";
      exit(1);
   }

   // Allocate memory for the verification task options
   n_vx   = n_fvx;
   vx_opt = new PointStatVxOpt [n_vx];

   // Check for consistent number of climatology fields
   check_climo_n_vx(&conf, n_vx);

   // Parse settings for each verification task
   for(i=0; i<n_vx; i++) {

      // Get the current dictionaries
      i_fdict = parse_conf_i_vx_dict(fdict, i);
      i_odict = parse_conf_i_vx_dict(odict, i);

      // Process the options for this verification task
      vx_opt[i].process_config(ftype, i_fdict, i_odict, use_var_id);
   }

   // Summarize output flags across all verification tasks
   process_flags();

   // If VL1L2 or VAL1L2 is requested, set the uv_index.
   // When processing vectors, need to make sure the message types,
   // masking regions, and interpolation methods are consistent.
   if(output_flag[i_vl1l2]  != STATOutputType_None ||
      output_flag[i_val1l2] != STATOutputType_None) {

      for(i=0; i<n_vx; i++) {

         // Process u-wind
         if(vx_opt[i].vx_pd.fcst_info->is_u_wind() &&
            vx_opt[i].vx_pd.obs_info->is_u_wind()) {

            // Search for corresponding v-wind
            for(j=0; j<n_vx; j++) {
               if(vx_opt[j].vx_pd.fcst_info->is_v_wind()      &&
                  vx_opt[j].vx_pd.obs_info->is_v_wind()       &&
                  vx_opt[i].vx_pd.fcst_info->req_level_name() ==
                  vx_opt[j].vx_pd.fcst_info->req_level_name() &&
                  vx_opt[i].vx_pd.obs_info->req_level_name()  ==
                  vx_opt[j].vx_pd.obs_info->req_level_name()) {

                  vx_opt[i].vx_pd.fcst_info->set_uv_index(j);
                  vx_opt[i].vx_pd.obs_info->set_uv_index(j);
               }
            }
         }
         // Process v-wind
         else if(vx_opt[i].vx_pd.fcst_info->is_v_wind() &&
                 vx_opt[i].vx_pd.obs_info->is_v_wind()) {

            // Search for corresponding u-wind
            for(j=0; j<n_vx; j++) {
               if(vx_opt[j].vx_pd.fcst_info->is_u_wind()      &&
                  vx_opt[j].vx_pd.obs_info->is_u_wind()       &&
                  vx_opt[i].vx_pd.fcst_info->req_level_name() ==
                  vx_opt[j].vx_pd.fcst_info->req_level_name() &&
                  vx_opt[i].vx_pd.obs_info->req_level_name()  ==
                  vx_opt[j].vx_pd.obs_info->req_level_name()) {

                  vx_opt[i].vx_pd.fcst_info->set_uv_index(j);
                  vx_opt[i].vx_pd.obs_info->set_uv_index(j);
               }
            }
         }
      } // end for i
   } // end if

   return;
}

////////////////////////////////////////////////////////////////////////

void PointStatConfInfo::process_flags() {
   int i, j;
   bool output_ascii_flag = false;

   // Initialize
   for(i=0; i<n_txt; i++) output_flag[i] = STATOutputType_None;

   // Loop over the verification tasks
   for(i=0; i<n_vx; i++) {

      // Summary of output_flag settings
      for(j=0; j<n_txt; j++) {
         if(vx_opt[i].output_flag[j] == STATOutputType_Both) {
            output_flag[j] = STATOutputType_Both;
            output_ascii_flag = true;
         }
         else if(vx_opt[i].output_flag[j] == STATOutputType_Stat &&
                           output_flag[j] == STATOutputType_None) {
            output_flag[j] = STATOutputType_Stat;
            output_ascii_flag = true;
         }
      }
   }

   // Check for at least one output line type
   if(!output_ascii_flag) {
      mlog << Error << "\nPointStatVxOpt::process_config() -> "
           << "At least one output STAT type must be requested in \""
           << conf_key_output_flag << "\".\n\n";
      exit(1);
   }

   return;
}

////////////////////////////////////////////////////////////////////////

void PointStatConfInfo::process_masks(const Grid &grid) {
   int i, j;
   DataPlane dp;
   StringArray sid;
   ConcatString name;

   mlog << Debug(2)
        << "Processing masking regions.\n";

   // Mapping of grid definition strings to mask names
   map<ConcatString,ConcatString> grid_map;
   map<ConcatString,ConcatString> poly_map;
   map<ConcatString,ConcatString> sid_map;

   // Initiailize
   mask_dp_map.clear();
   mask_sid_map.clear();

   // Process the masks for each vx task
   for(i=0; i<n_vx; i++) {

      // Initialize
      vx_opt[i].mask_name.clear();

      // Parse the masking grids
      for(j=0; j<vx_opt[i].mask_grid.n_elements(); j++) {

         // Process new grid masks
         if(grid_map.count(vx_opt[i].mask_grid[j]) == 0) {
            mlog << Debug(3)
                 << "Processing grid mask: "
                 << vx_opt[i].mask_grid[j] << "\n";
            parse_grid_mask(vx_opt[i].mask_grid[j], grid, dp, name);
            grid_map[vx_opt[i].mask_grid[j]] = name;
            mask_dp_map[name] = dp;
         }

         // Store the name for the current grid mask
         vx_opt[i].mask_name.add(grid_map[vx_opt[i].mask_grid[j]]);

      } // end for j

      // Parse the masking polylines
      for(j=0; j<vx_opt[i].mask_poly.n_elements(); j++) {

         // Process new poly mask
         if(poly_map.count(vx_opt[i].mask_poly[j]) == 0) {
            mlog << Debug(3)
                 << "Processing poly mask: "
                 << vx_opt[i].mask_poly[j] << "\n";
            parse_poly_mask(vx_opt[i].mask_poly[j], grid, dp, name);
            poly_map[vx_opt[i].mask_poly[j]] = name;
            mask_dp_map[name] = dp;
         }

         // Store the name for the current poly mask
         vx_opt[i].mask_name.add(poly_map[vx_opt[i].mask_poly[j]]);

      } // end for j

      // Parse the masking station ID's
      for(j=0; j<vx_opt[i].mask_sid.n_elements(); j++) {

         // Process new station ID mask
         if(sid_map.count(vx_opt[i].mask_sid[j]) == 0) {
            mlog << Debug(3)
                 << "Processing station ID mask: "
                 << vx_opt[i].mask_sid[j] << "\n";
            parse_sid_mask(vx_opt[i].mask_sid[j], sid, name);
            sid_map[vx_opt[i].mask_sid[j]] = name;
            mask_sid_map[name] = sid;
         }

         // Store the name for the current station ID mask
         vx_opt[i].mask_name.add(sid_map[vx_opt[i].mask_sid[j]]);

      } // end for j

   } // end for i

   return;
}

////////////////////////////////////////////////////////////////////////

void PointStatConfInfo::set_vx_pd() {

   // This should be called after process_masks()
   for(int i=0; i<n_vx; i++) vx_opt[i].set_vx_pd(this);
}

////////////////////////////////////////////////////////////////////////

int PointStatConfInfo::n_txt_row(int i_txt_row) const {
   int i, n;

   // Loop over the tasks and sum the line counts for this line type
   for(i=0, n=0; i<n_vx; i++) n += vx_opt[i].n_txt_row(i_txt_row);

   return(n);
}

////////////////////////////////////////////////////////////////////////

int PointStatConfInfo::n_stat_row() const {
   int i, n;

   // Loop over the line types and sum the line counts
   for(i=0, n=0; i<n_txt; i++) n += n_txt_row(i);

   return(n);
}

////////////////////////////////////////////////////////////////////////

int PointStatConfInfo::get_max_n_cat_thresh() const {
   int i, n;

   for(i=0,n=0; i<n_vx; i++) n = max(n, vx_opt[i].get_n_cat_thresh());

   return(n);
}

////////////////////////////////////////////////////////////////////////

int PointStatConfInfo::get_max_n_cnt_thresh() const {
   int i, n;

   for(i=0,n=0; i<n_vx; i++) n = max(n, vx_opt[i].get_n_cnt_thresh());

   return(n);
}

////////////////////////////////////////////////////////////////////////

int PointStatConfInfo::get_max_n_wind_thresh() const {
   int i, n;

   for(i=0,n=0; i<n_vx; i++) n = max(n, vx_opt[i].get_n_wind_thresh());

   return(n);
}

////////////////////////////////////////////////////////////////////////

int PointStatConfInfo::get_max_n_fprob_thresh() const {
   int i, n;

   for(i=0,n=0; i<n_vx; i++) n = max(n, vx_opt[i].get_n_fprob_thresh());

   return(n);
}

////////////////////////////////////////////////////////////////////////

int PointStatConfInfo::get_max_n_oprob_thresh() const {
   int i, n;

   for(i=0,n=0; i<n_vx; i++) n = max(n, vx_opt[i].get_n_oprob_thresh());

   return(n);
}

////////////////////////////////////////////////////////////////////////

int PointStatConfInfo::get_max_n_eclv_points() const {
   int i, n;

   for(i=0,n=0; i<n_vx; i++) n = max(n, vx_opt[i].get_n_eclv_points());

   return(n);
}

////////////////////////////////////////////////////////////////////////

bool PointStatConfInfo::get_vflag() const {
   int i;
   bool vflag = false;

   // Vector output must be requested
   if(output_flag[i_vl1l2]  == STATOutputType_None &&
      output_flag[i_val1l2] == STATOutputType_None) {
      return(false);
   }

   // Vector components must be requested
   for(i=0; i<n_vx; i++) {

      if(!vx_opt[i].vx_pd.fcst_info || !vx_opt[i].vx_pd.obs_info) continue;

      if((vx_opt[i].vx_pd.fcst_info->is_u_wind() &&
          vx_opt[i].vx_pd.obs_info->is_u_wind()) ||
         (vx_opt[i].vx_pd.fcst_info->is_v_wind() &&
          vx_opt[i].vx_pd.obs_info->is_v_wind())) {
         vflag = true;
         break;
      }
   }

   return(vflag);
}

////////////////////////////////////////////////////////////////////////
//
//  Code for class PointStatVxOpt
//
////////////////////////////////////////////////////////////////////////

PointStatVxOpt::PointStatVxOpt() {

   init_from_scratch();
}

////////////////////////////////////////////////////////////////////////

PointStatVxOpt::~PointStatVxOpt() {

   clear();
}

////////////////////////////////////////////////////////////////////////

void PointStatVxOpt::init_from_scratch() {

   clear();

   return;
}

////////////////////////////////////////////////////////////////////////

void PointStatVxOpt::clear() {
   int i;

   // Initialize values
   vx_pd.clear();

   beg_ds = end_ds = bad_data_int;

   fcat_ta.clear();
   ocat_ta.clear();

   fcnt_ta.clear();
   ocnt_ta.clear();
   cnt_logic = SetLogic_None;

   fwind_ta.clear();
   owind_ta.clear();
   wind_logic = SetLogic_None;

   mask_grid.clear();
   mask_poly.clear();
   mask_sid.clear();

   mask_name.clear();

   eclv_points.clear();

   climo_cdf_ta.clear();

   ci_alpha.clear();

   boot_info.clear();
   interp_info.clear();
   hira_info.clear();

   rank_corr_flag = false;

   msg_typ.clear();

   duplicate_flag = DuplicateType_None;
   obs_summary = ObsSummary_None;
   obs_perc = bad_data_int;

   for(i=0; i<n_txt; i++) output_flag[i] = STATOutputType_None;

   return;
}

////////////////////////////////////////////////////////////////////////

void PointStatVxOpt::process_config(GrdFileType ftype,
        Dictionary &fdict, Dictionary &odict, bool use_var_id) {
   int i, n;
   VarInfoFactory info_factory;
   map<STATLineType,STATOutputType>output_map;
   InterpMthd mthd;
   Dictionary *dict;

   // Initialize
   clear();

   // Allocate new VarInfo objects
   vx_pd.fcst_info = info_factory.new_var_info(ftype);
   vx_pd.obs_info  = new VarInfoGrib;

   // Set the VarInfo objects
   vx_pd.fcst_info->set_dict(fdict);
   vx_pd.obs_info->set_dict(odict);

   // Set the GRIB code for point observations
   if(!use_var_id) vx_pd.obs_info->add_grib_code(odict);

   // Dump the contents of the current VarInfo
   if(mlog.verbosity_level() >= 5) {
      mlog << Debug(5)
           << "Parsed forecast field:\n";
      vx_pd.fcst_info->dump(cout);
      mlog << Debug(5)
           << "Parsed observation field:\n";
      vx_pd.obs_info->dump(cout);
   }

   // Check the levels for the forecast and observation fields.  If the
   // forecast field is a range of pressure levels, check to see if the
   // range of observation field pressure levels is wholly contained in the
   // fcst levels.  If not, print a warning message.
   if(vx_pd.fcst_info->level().type() == LevelType_Pres &&
      !is_eq(vx_pd.fcst_info->level().lower(), vx_pd.fcst_info->level().upper()) &&
      (vx_pd.obs_info->level().lower() < vx_pd.fcst_info->level().lower() ||
       vx_pd.obs_info->level().upper() > vx_pd.fcst_info->level().upper())) {

      mlog << Warning
           << "\nPointStatVxOpt::process_config() -> "
           << "The range of requested observation pressure levels "
           << "is not contained within the range of requested "
           << "forecast pressure levels.  No vertical interpolation "
           << "will be performed for observations falling outside "
           << "the range of forecast levels.  Instead, they will be "
           << "matched to the single nearest forecast level.\n\n";
   }

   // No support for wind direction
   if(vx_pd.fcst_info->is_wind_direction() ||
      vx_pd.obs_info->is_wind_direction()) {
      mlog << Error << "\nPointStatVxOpt::process_config() -> "
           << "wind direction may not be verified using point_stat.\n\n";
      exit(1);
   }

   // Check that the observation field does not contain probabilities
   if(vx_pd.obs_info->is_prob()) {
      mlog << Error << "\nPointStatVxOpt::process_config() -> "
           << "the observation field cannot contain probabilities.\n\n";
      exit(1);
   }

   // Conf: output_flag
   output_map = parse_conf_output_flag(&odict, txt_file_type, n_txt);

   // Populate the output_flag array with map values
   for(i=0; i<n_txt; i++) output_flag[i] = output_map[txt_file_type[i]];

   // Conf: beg_ds and end_ds
   dict = odict.lookup_dictionary(conf_key_obs_window);
   parse_conf_range_int(dict, beg_ds, end_ds);

   // Conf: cat_thresh
   fcat_ta = fdict.lookup_thresh_array(conf_key_cat_thresh);
   ocat_ta = odict.lookup_thresh_array(conf_key_cat_thresh);

   // Conf: cnt_thresh
   fcnt_ta = fdict.lookup_thresh_array(conf_key_cnt_thresh);
   ocnt_ta = odict.lookup_thresh_array(conf_key_cnt_thresh);

   // Conf: cnt_logic
   cnt_logic = check_setlogic(
      int_to_setlogic(fdict.lookup_int(conf_key_cnt_logic)),
      int_to_setlogic(odict.lookup_int(conf_key_cnt_logic)));

   // Conf: wind_thresh
   fwind_ta = fdict.lookup_thresh_array(conf_key_wind_thresh);
   owind_ta = odict.lookup_thresh_array(conf_key_wind_thresh);

   // Conf: wind_logic
   wind_logic = check_setlogic(
      int_to_setlogic(fdict.lookup_int(conf_key_wind_logic)),
      int_to_setlogic(odict.lookup_int(conf_key_wind_logic)));

   // Dump the contents of the current thresholds
   if(mlog.verbosity_level() >= 5) {
      mlog << Debug(5)
           << "Parsed thresholds:\n"
           << "Forecast categorical thresholds: " << fcat_ta.get_str() << "\n"
           << "Observed categorical thresholds: " << ocat_ta.get_str() << "\n"
           << "Forecast continuous thresholds:  " << fcnt_ta.get_str() << "\n"
           << "Observed continuous thresholds:  " << ocnt_ta.get_str() << "\n"
           << "Continuous threshold logic:      " << setlogic_to_string(cnt_logic) << "\n"
           << "Forecast wind speed thresholds:  " << fwind_ta.get_str() << "\n"
           << "Observed wind speed thresholds:  " << owind_ta.get_str() << "\n"
           << "Wind speed threshold logic:      " << setlogic_to_string(wind_logic) << "\n";
   }

   // Verifying a probability field
   if(vx_pd.fcst_info->is_prob()) {
      fcat_ta = string_to_prob_thresh(fcat_ta.get_str());
   }

   // Check for equal threshold length for non-probability fields
   if(!vx_pd.fcst_info->is_prob() &&
      fcat_ta.n_elements() != ocat_ta.n_elements()) {

      mlog << Error << "\nPointStatVxOpt::process_config() -> "
           << "The number of thresholds for each field in \"fcst."
           << conf_key_cat_thresh
           << "\" must match the number of thresholds for each "
           << "field in \"obs." << conf_key_cat_thresh << "\".\n\n";
      exit(1);
   }

   // Add default continuous thresholds until the counts match
   n = max(fcnt_ta.n_elements(), ocnt_ta.n_elements());
   while(fcnt_ta.n_elements() < n) fcnt_ta.add(na_str);
   while(ocnt_ta.n_elements() < n) ocnt_ta.add(na_str);

   // Add default wind speed thresholds until the counts match
   n = max(fwind_ta.n_elements(), owind_ta.n_elements());
   while(fwind_ta.n_elements() < n) fwind_ta.add(na_str);
   while(owind_ta.n_elements() < n) owind_ta.add(na_str);

   // Verifying with multi-category contingency tables
   if(!vx_pd.fcst_info->is_prob() &&
      (output_flag[i_mctc] != STATOutputType_None ||
       output_flag[i_mcts] != STATOutputType_None)) {
      check_mctc_thresh(fcat_ta);
      check_mctc_thresh(ocat_ta);
   }

   // Conf: mask_grid
   mask_grid = odict.lookup_string_array(conf_key_mask_grid);

   // Conf: mask_poly
   mask_poly = odict.lookup_string_array(conf_key_mask_poly);

   // Conf: mask_sid
   mask_sid = odict.lookup_string_array(conf_key_mask_sid);

   // Conf: eclv_points
   eclv_points = parse_conf_eclv_points(&odict);

   // Conf: climo_cdf_bins
   climo_cdf_ta = parse_conf_climo_cdf_bins(&odict);

   // Conf: ci_alpha
   ci_alpha = parse_conf_ci_alpha(&odict);

   // Conf: boot
   boot_info = parse_conf_boot(&odict);

   // Conf: interp
   interp_info = parse_conf_interp(&odict);

   // Conf: hira
   hira_info = parse_conf_hira(&odict);

   // Conf: rank_corr_flag
   rank_corr_flag = odict.lookup_bool(conf_key_rank_corr_flag);

   // Conf: message_type
   msg_typ = parse_conf_message_type(&odict);

   // Conf: duplicate_flag
   duplicate_flag = parse_conf_duplicate_flag(&odict);

   // Conf: obs_summary
   obs_summary = parse_conf_obs_summary(&odict);

   // Conf: obs_perc_value
   obs_perc = parse_conf_percentile(&odict);

   // Conf: desc
   vx_pd.set_desc(parse_conf_string(&odict, conf_key_desc));

   // Conf: sid_exc
   vx_pd.set_sid_exc_filt(parse_conf_sid_exc(&odict));

   // Conf: obs_qty
   vx_pd.set_obs_qty_filt(parse_conf_obs_qty(&odict));

   return;
}

////////////////////////////////////////////////////////////////////////

void PointStatVxOpt::set_vx_pd(PointStatConfInfo *conf_info) {
   int i, n;
   int n_msg_typ = msg_typ.n_elements();
   int n_mask    = mask_name.n_elements();
   int n_interp  = interp_info.n_interp;
   StringArray sa;

   // Setup the VxPairDataPoint object with these dimensions:
   // [n_msg_typ][n_mask][n_interp]

   // Check for at least one message type
   if(n_msg_typ == 0) {
      mlog << Error << "\nPointStatVxOpt::set_vx_pd() -> "
           << "At least one output message type must be requested in \""
           << conf_key_message_type << "\".\n\n";
      exit(1);
   }

   // Check for at least one masking region
   if(n_mask == 0) {
      mlog << Error << "\nPointStatVxOpt::set_vx_pd() -> "
           << "At least one output masking region must be requested in \""
           << conf_key_mask_grid << "\", \""
           << conf_key_mask_poly << "\", or \""
           << conf_key_mask_sid << "\".\n\n";
      exit(1);
   }

   // Check for at least one interpolation method
   if(n_interp == 0) {
      mlog << Error << "\nPointStatVxOpt::set_vx_pd() -> "
           << "At least one interpolation method must be requested in \""
           << conf_key_interp << "\".\n\n";
      exit(1);
   }

   // Define the dimensions
   vx_pd.set_pd_size(n_msg_typ, n_mask, n_interp);

   // Store the list of surface message types
   vx_pd.set_msg_typ_sfc(conf_info->msg_typ_sfc);

   // Define the verifying message type name and values
   for(i=0; i<n_msg_typ; i++) {
      vx_pd.set_msg_typ(i, msg_typ[i]);
      sa = conf_info->msg_typ_group_map[msg_typ[i]];
      if(sa.n_elements() == 0) sa.add(msg_typ[i]);
      vx_pd.set_msg_typ_vals(i, sa);
   }

   // Define the masking information: grid, poly, sid

   // Define the grid masks
   for(i=0; i<mask_grid.n_elements(); i++) {
      n = i;
      vx_pd.set_mask_dp(n, mask_name[n],
                        &(conf_info->mask_dp_map[mask_name[n]]));
   }

   // Define the poly masks
   for(i=0; i<mask_poly.n_elements(); i++) {
      n = i + mask_grid.n_elements();
      vx_pd.set_mask_dp(n, mask_name[n],
                        &(conf_info->mask_dp_map[mask_name[n]]));
   }

   // Define the station ID masks
   for(i=0; i<mask_sid.n_elements(); i++) {
      n = i + mask_grid.n_elements() + mask_poly.n_elements();
      vx_pd.set_mask_sid(n, mask_name[n],
                         &(conf_info->mask_sid_map[mask_name[n]]));
   }

   // Define the interpolation methods
   for(i=0; i<n_interp; i++) {
      vx_pd.set_interp(i, interp_info.method[i], interp_info.width[i],
                       interp_info.shape);
   }

   // After sizing VxPairDataPoint, add settings for each array element
   vx_pd.set_duplicate_flag(duplicate_flag);
   vx_pd.set_obs_summary(obs_summary);
   vx_pd.set_obs_perc_value(obs_perc);

   return;
}

////////////////////////////////////////////////////////////////////////

int PointStatVxOpt::n_txt_row(int i_txt_row) const {
   int n;

   // Range check
   if(i_txt_row < 0 || i_txt_row >= n_txt) {
      mlog << Error << "\nPointStatVxOpt::n_txt_row(int) -> "
           << "range check error for " << i_txt_row << "\n\n";
      exit(1);
   }

   // Check if this output line type is requested
   if(output_flag[i_txt_row] == STATOutputType_None) return(0);

   bool prob_flag = vx_pd.fcst_info->is_prob();
   bool vect_flag = (vx_pd.fcst_info->is_u_wind() &&
                     vx_pd.obs_info->is_u_wind());

   int n_pd = get_n_msg_typ() * get_n_mask() * get_n_interp();

   // Switch on the index of the line type
   switch(i_txt_row) {

      case(i_fho):
      case(i_ctc):
         // Number of FHO or CTC lines =
         //    Message Types * Masks * Interpolations *
         //    Thresholds
         n = (prob_flag ? 0 : n_pd *
              get_n_cat_thresh());
         break;

      case(i_cts):
         // Number of CTS lines =
         //    Message Types * Masks * Interpolations *
         //    Thresholds * Alphas
         n = (prob_flag ? 0 : n_pd *
              get_n_cat_thresh() * get_n_ci_alpha());
         break;

      case(i_mctc):
         // Number of MCTC lines =
         //    Message Types * Masks * Interpolations
         n = (prob_flag ? 0 : n_pd);
         break;

      case(i_mcts):
         // Number of MCTS lines =
         //    Message Types * Masks * Interpolations *
         //    Alphas
         n = (prob_flag ? 0 : n_pd *
              get_n_ci_alpha());
         break;

      case(i_cnt):
         // Number of CNT lines =
         //    Message Types * Masks * Interpolations *
         //    Thresholds * Alphas
         n = (prob_flag ? 0 : n_pd *
              get_n_cnt_thresh() * get_n_ci_alpha());
         break;

      case(i_sl1l2):
      case(i_sal1l2):
         // Number of SL1L2 and SAL1L2 lines =
         //    Message Types * Masks * Interpolations *
         //    Thresholds
         n = (prob_flag ? 0 : n_pd *
              get_n_cnt_thresh());
         break;

      case(i_vl1l2):
      case(i_val1l2):
         // Number of VL1L2 or VAL1L2 lines =
         //    Message Types * Masks * Interpolations *
         //    Thresholds
         n = (!vect_flag ? 0 : n_pd *
              get_n_wind_thresh());
         break;

      case(i_pct):
      case(i_pjc):
      case(i_prc):
         // Number of PCT, PJC, or PRC lines possible =
         //    Message Types * Masks * Interpolations *
         //    Thresholds * Climo Bins
         n = (!prob_flag ? 0 : n_pd *
              get_n_oprob_thresh() * get_n_cdf_bin());

         // Number of HiRA PCT, PJC, or PRC lines =
         //    Message Types * Masks * Interpolations *
         //    Thresholds * HiRA widths
         if(hira_info.flag) {
            n += (prob_flag ? 0 : n_pd *
                  get_n_cat_thresh() * hira_info.width.n_elements());
         }

         break;

      case(i_pstd):
         // Number of PSTD lines =
         //    Message Types * Masks * Interpolations *
         //    Thresholds * Alphas * Climo Bins
         n = (!prob_flag ? 0 : n_pd *
              get_n_oprob_thresh() * get_n_ci_alpha() * get_n_cdf_bin());

         // Number of HiRA PSTD lines =
         //    Message Types * Masks * Interpolations *
         //    Thresholds * HiRA widths * Alphas
         if(hira_info.flag) {
            n += (prob_flag ? 0 : n_pd *
                  get_n_cat_thresh() * hira_info.width.n_elements() *
                  get_n_ci_alpha());
         }

         break;

      case(i_eclv):
         // Number of CTC -> ECLV lines =
         //    Message Types * Masks * Interpolations *
         //    Thresholds
         n = (prob_flag ? 0 : n_pd *
              get_n_cat_thresh());

         // Number of PCT -> ECLV lines =
         //    Message Types * Masks * Interpolations *
         //    Observation Probability Thresholds *
         //    Forecast Probability Thresholds * Climo Bins
         n += (!prob_flag ? 0 : n_pd *
               get_n_oprob_thresh() * get_n_fprob_thresh() *
               get_n_cdf_bin());

         break;

      case(i_mpr):
         // Compute the number of matched pairs to be written
         n = vx_pd.get_n_pair();

         // Maximum number of HiRA MPR lines possible =
         //    Number of pairs * Max Scalar Categorical Thresholds *
         //    HiRA widths
         if(hira_info.flag) {
            n += (prob_flag ? 0 :
                  vx_pd.get_n_pair() * get_n_cat_thresh() *
                  hira_info.width.n_elements());
         }

         break;

      default:
         mlog << Error << "\nPointStatVxOpt::n_txt_row(int) -> "
              << "unexpected output type index value: " << i_txt_row
              << "\n\n";
         exit(1);
         break;
   }

   return(n);
}

////////////////////////////////////////////////////////////////////////

int PointStatVxOpt::get_n_fprob_thresh() const {
   return((!vx_pd.fcst_info || !vx_pd.fcst_info->is_prob()) ?
          0 : fcat_ta.n_elements());
}

////////////////////////////////////////////////////////////////////////

int PointStatVxOpt::get_n_oprob_thresh() const {
   return((!vx_pd.fcst_info || !vx_pd.fcst_info->is_prob()) ?
          0 : ocat_ta.n_elements());
}

////////////////////////////////////////////////////////////////////////
