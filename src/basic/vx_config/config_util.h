////////////////////////////////////////////////////////////////////////
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2017
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

////////////////////////////////////////////////////////////////////////

#ifndef __CONFIG_UTIL_H__
#define __CONFIG_UTIL_H__

////////////////////////////////////////////////////////////////////////

#include "string"
#include "map"

#include "config_constants.h"
#include "config_file.h"
#include "data_file_type.h"

////////////////////////////////////////////////////////////////////////

extern ConcatString    parse_conf_version(Dictionary *dict);
extern ConcatString    parse_conf_string(Dictionary *dict, const char *);
extern GrdFileType     parse_conf_file_type(Dictionary *dict);
extern map<STATLineType,STATOutputType>
                       parse_conf_output_flag(Dictionary *dict);
extern map<STATLineType,StringArray>
                       parse_conf_output_stats(Dictionary *dict);
extern int             parse_conf_n_vx(Dictionary *dict);
extern Dictionary      parse_conf_i_vx_dict(Dictionary *dict, int index);
extern StringArray     parse_conf_message_type(Dictionary *dict, bool error_out = default_dictionary_error_out);
extern StringArray     parse_conf_sid_exc(Dictionary *dict);
extern void            parse_sid_mask(const ConcatString &, StringArray &, ConcatString &);
extern StringArray     parse_conf_obs_qty(Dictionary *dict);
extern NumArray        parse_conf_ci_alpha(Dictionary *dict);
extern NumArray        parse_conf_eclv_points(Dictionary *dict);
extern ThreshArray     parse_conf_climo_cdf_bins(Dictionary *dict);
extern TimeSummaryInfo parse_conf_time_summary(Dictionary *dict);
extern map<ConcatString,ConcatString>
                       parse_conf_message_type_map(Dictionary *dict);
extern map<ConcatString,ConcatString>
                       parse_conf_obs_var_map(Dictionary *dict);
extern BootInfo        parse_conf_boot(Dictionary *dict);
extern RegridInfo      parse_conf_regrid(Dictionary *dict);
extern InterpInfo      parse_conf_interp(Dictionary *dict);
extern NbrhdInfo       parse_conf_nbrhd(Dictionary *dict);
extern HiRAInfo        parse_conf_hira(Dictionary *dict);
extern GridWeightType  parse_conf_grid_weight_flag(Dictionary *dict);
extern DuplicateType   parse_conf_duplicate_flag(Dictionary *dict);
extern ObsSummary      parse_conf_obs_summary(Dictionary *dict);
extern ConcatString    parse_conf_tmp_dir(Dictionary *dict);
extern GridDecompType  parse_conf_grid_decomp_flag(Dictionary *dict);
extern WaveletType     parse_conf_wavelet_type(Dictionary *dict);
extern PlotInfo        parse_conf_plot_info(Dictionary *dict);
extern void            parse_conf_range_int(Dictionary *dict, int &beg, int &end);
extern void            parse_conf_range_double(Dictionary *dict, double &beg, double &end);

extern void         check_climo_n_vx(Dictionary *dict, const int);

extern InterpMthd   int_to_interpmthd(int);
extern void         check_mctc_thresh(const ThreshArray &);

extern bool         check_fo_thresh(const double, const SingleThresh &,
                                    const double, const SingleThresh &,
                                    const SetLogic);

extern const char * statlinetype_to_string(const STATLineType);
extern void         statlinetype_to_string(const STATLineType, char *);
extern STATLineType string_to_statlinetype(const char *);

extern FieldType    int_to_fieldtype(int);
extern ConcatString fieldtype_to_string(FieldType);

extern GridTemplateFactory::GridTemplates int_to_gridtemplate(int);

extern SetLogic     int_to_setlogic(int);
extern SetLogic     string_to_setlogic(const char *);
extern ConcatString setlogic_to_string(SetLogic);
extern ConcatString setlogic_to_abbr(SetLogic);
extern ConcatString setlogic_to_symbol(SetLogic);

extern SetLogic     check_setlogic(SetLogic, SetLogic);

extern TrackType    int_to_tracktype(int);
extern TrackType    string_to_tracktype(const char *);
extern ConcatString tracktype_to_string(TrackType);

extern Interp12Type int_to_interp12type(int);
extern Interp12Type string_to_interp12type(const char *);
extern ConcatString interp12type_to_string(Interp12Type);

extern MergeType    int_to_mergetype(int);
extern ConcatString mergetype_to_string(MergeType);

extern ConcatString obssummary_to_string(ObsSummary, int);

extern MatchType    int_to_matchtype(int);
extern ConcatString matchtype_to_string(MatchType);

extern ConcatString griddecomptype_to_string(GridDecompType);
extern ConcatString wavelettype_to_string(WaveletType);

extern int parse_conf_percentile(Dictionary *dict);

////////////////////////////////////////////////////////////////////////

#endif   /*  __CONFIG_UTIL_H__  */

////////////////////////////////////////////////////////////////////////
