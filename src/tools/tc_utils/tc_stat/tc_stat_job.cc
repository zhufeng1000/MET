// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2014
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
#include <map>
#include <utility>

#include "tc_stat_job.h"

#include "met_stats.h"
#include "vx_tc_util.h"
#include "vx_log.h"
#include "vx_util.h"
#include "vx_math.h"
#include "vx_config.h"
#include "vx_stat_out.h"

////////////////////////////////////////////////////////////////////////

static const char *TCStatJobType_FilterStr  = "filter";
static const char *TCStatJobType_SummaryStr = "summary";
static const char *TCStatJobType_RIRWStr    = "rirw";

////////////////////////////////////////////////////////////////////////

// Functions for parsing command line options
extern void parse_thresh_option(const char *, const char *, map<ConcatString,ThreshArray> &);
extern void parse_string_option(const char *, const char *, map<ConcatString,StringArray> &);
extern void setup_table        (AsciiTable &, int);

////////////////////////////////////////////////////////////////////////
//
// Code for class TCStatJobFactory
//
////////////////////////////////////////////////////////////////////////

TCStatJob *TCStatJobFactory::new_tc_stat_job_type(const char *type_str) {
   TCStatJob *job = (TCStatJob *) 0;
   TCStatJobType type = NoTCStatJobType;

   // Determine the TCStatJobType
   type = string_to_tcstatjobtype(type_str);

   // Switch on job type and instantiate the appropriate class.
   // The TCStatJob object is allocated and needs to be deleted by caller.
   switch(type) {

      case TCStatJobType_Filter:
         job = new TCStatJobFilter;
         break;

      case TCStatJobType_Summary:
         job = new TCStatJobSummary;
         break;

      case TCStatJobType_RIRW:
         job = new TCStatJobRIRW;
         break;

      case NoTCStatJobType:
      default:
         mlog << Error << "\nTCStatJobFactory::new_tc_stat_job_type() -> "
              << "unsupported job type \"" << type_str << "\"\n\n";
         exit(1);
         break;
   } // end switch

   return(job);
}

///////////////////////////////////////////////////////////////////////////////

TCStatJob *TCStatJobFactory::new_tc_stat_job(const char *jobstring) {
   TCStatJob *job = (TCStatJob *) 0;
   StringArray a;
   ConcatString type_str = na_str;
   ConcatString err_str;
   int i;

   // Parse the jobstring into an array
   a.parse_wsss(jobstring);

   // Check if the job type is specified
   if(a.has("-job", i)) type_str = a[i+1];

   // Allocate a new job of the requested type
   job = new_tc_stat_job_type(type_str);

   // Parse the jobstring into the new job
   a = job->parse_job_command(jobstring);

   // Check for unused arguments
   if(a.n_elements() > 0) {

      // Build list of unknown args
      for(i=0; i<a.n_elements()-1; i++) err_str << a[i] << " ";
      err_str << a[a.n_elements()-1];

      mlog << Error
           << "\nTCStatJob *TCStatJobFactory::new_tc_stat_job(const char *jobstring) -> "
           << "unsupported job command options \""
           << err_str << "\".\n\n";
      exit(1);
   }

   return(job);
}

////////////////////////////////////////////////////////////////////////
//
// Code for class TCStatJob
//
////////////////////////////////////////////////////////////////////////

TCStatJob::TCStatJob() {

   init_from_scratch();
}

////////////////////////////////////////////////////////////////////////

TCStatJob::~TCStatJob() {

   clear();
}

////////////////////////////////////////////////////////////////////////

TCStatJob::TCStatJob(const TCStatJob &j) {

   init_from_scratch();

   assign(j);
}

////////////////////////////////////////////////////////////////////////

TCStatJob & TCStatJob::operator=(const TCStatJob &j) {

   if(this == &j) return(*this);

   assign(j);

   return(*this);
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::init_from_scratch() {

   DumpOut = (ofstream *) 0;
   JobOut  = (ofstream *) 0;

   // Ignore case when performing comparisons
   AModel.set_ignore_case(1);
   BModel.set_ignore_case(1);
   StormId.set_ignore_case(1);
   Basin.set_ignore_case(1);
   Cyclone.set_ignore_case(1);
   StormName.set_ignore_case(1);
   InitMask.set_ignore_case(1);
   ValidMask.set_ignore_case(1);
   LineType.set_ignore_case(1);
   TrackWatchWarn.set_ignore_case(1);

   clear();

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::clear() {

   JobType = NoTCStatJobType;

   AModel.clear();
   BModel.clear();
   StormId.clear();
   Basin.clear();
   Cyclone.clear();
   StormName.clear();
   InitBeg = InitEnd = (unixtime) 0;
   InitInc.clear();
   InitExc.clear();
   InitHour.clear();
   Lead.clear();
   ValidBeg = ValidEnd = (unixtime) 0;
   ValidInc.clear();
   ValidExc.clear();
   ValidHour.clear();
   InitMask.clear();
   ValidMask.clear();
   LineType.clear();
   TrackWatchWarn.clear();
   ColumnThreshMap.clear();
   ColumnStrMap.clear();
   InitThreshMap.clear();
   InitStrMap.clear();
   EventEqualLead.clear();
   EventEqualCases.clear();

   DumpFile.clear();
   close_dump_file();
   JobOut = (ofstream *) 0;

   // Set to default values
   WaterOnly             = default_water_only;
   RapidIntenTrack       = default_rapid_inten_track;
   RapidIntenTimeADeck   = default_rapid_inten_time;
   RapidIntenTimeBDeck   = default_rapid_inten_time;
   RapidIntenExactADeck  = default_rapid_inten_exact;
   RapidIntenExactBDeck  = default_rapid_inten_exact;
   RapidIntenThreshADeck = default_rapid_inten_thresh;
   RapidIntenThreshBDeck = default_rapid_inten_thresh;
   RapidIntenWindow      = default_rapid_inten_window;
   Landfall              = default_landfall;
   LandfallBeg           = default_landfall_beg;
   LandfallEnd           = default_landfall_end;
   MatchPoints           = default_match_points;
   EventEqual            = default_event_equal;
   EventEqualSet         = false;

   OutInitMask.clear();
   OutValidMask.clear();

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::assign(const TCStatJob & j) {

   clear();

   JobType = j.JobType;

   AModel = j.AModel;
   BModel = j.BModel;
   StormId = j.StormId;
   Basin = j.Basin;
   Cyclone = j.Cyclone;
   StormName = j.StormName;
   InitBeg = j.InitBeg;
   InitEnd = j.InitEnd;
   InitInc = j.InitInc;
   InitExc = j.InitExc;
   InitHour = j.InitHour;
   Lead = j.Lead;
   ValidBeg = j.ValidBeg;
   ValidEnd = j.ValidEnd;
   ValidInc = j.ValidInc;
   ValidExc = j.ValidExc;
   ValidHour = j.ValidHour;
   InitMask = j.InitMask;
   ValidMask = j.ValidMask;
   LineType = j.LineType;
   TrackWatchWarn = j.TrackWatchWarn;
   ColumnThreshMap = j.ColumnThreshMap;
   ColumnStrMap = j.ColumnStrMap;
   InitThreshMap = j.InitThreshMap;
   InitStrMap = j.InitStrMap;

   DumpFile = j.DumpFile;
   open_dump_file();

   WaterOnly = j.WaterOnly;

   RapidIntenTrack = j.RapidIntenTrack;
   RapidIntenTimeADeck = j.RapidIntenTimeADeck;
   RapidIntenTimeBDeck = j.RapidIntenTimeBDeck;
   RapidIntenExactADeck = j.RapidIntenExactADeck;
   RapidIntenExactBDeck = j.RapidIntenExactBDeck;
   RapidIntenThreshADeck = j.RapidIntenThreshADeck;
   RapidIntenThreshBDeck = j.RapidIntenThreshBDeck;
   RapidIntenWindow = j.RapidIntenWindow;

   Landfall = j.Landfall;
   LandfallBeg = j.LandfallBeg;
   LandfallEnd = j.LandfallEnd;

   MatchPoints = j.MatchPoints;
   EventEqual = j.EventEqual;
   EventEqualSet = j.EventEqualSet;
   EventEqualLead = j.EventEqualLead;
   EventEqualCases = j.EventEqualCases;

   OutInitMask = j.OutInitMask;
   OutValidMask = j.OutValidMask;

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::dump(ostream & out, int depth) const {
   Indent prefix(depth);
   map<ConcatString,ThreshArray>::const_iterator thr_it;
   map<ConcatString,StringArray>::const_iterator str_it;

   out << prefix << "JobType = " << tcstatjobtype_to_string(JobType)
       << "\n";

   out << prefix << "AModel ...\n";
   AModel.dump(out, depth + 1);

   out << prefix << "BModel ...\n";
   BModel.dump(out, depth + 1);

   out << prefix << "StormId ...\n";
   StormId.dump(out, depth + 1);

   out << prefix << "Basin ...\n";
   Basin.dump(out, depth + 1);

   out << prefix << "Cyclone ...\n";
   Cyclone.dump(out, depth + 1);

   out << prefix << "StormName ...\n";
   StormName.dump(out, depth + 1);

   out << prefix << "InitBeg = " << unix_to_yyyymmdd_hhmmss(InitBeg) << "\n";
   out << prefix << "InitEnd = " << unix_to_yyyymmdd_hhmmss(InitEnd) << "\n";

   out << prefix << "InitInc ...\n";
   InitInc.dump(out, depth + 1);

   out << prefix << "InitExc ...\n";
   InitExc.dump(out, depth + 1);

   out << prefix << "InitHour ...\n";
   InitHour.dump(out, depth + 1);

   out << prefix << "Lead ...\n";
   Lead.dump(out, depth + 1);

   out << prefix << "ValidBeg = " << unix_to_yyyymmdd_hhmmss(ValidBeg) << "\n";
   out << prefix << "ValidEnd = " << unix_to_yyyymmdd_hhmmss(ValidEnd) << "\n";

   out << prefix << "ValidInc ...\n";
   ValidInc.dump(out, depth + 1);

   out << prefix << "ValidExc ...\n";
   ValidExc.dump(out, depth + 1);

   out << prefix << "ValidHour ...\n";
   ValidHour.dump(out, depth + 1);

   out << prefix << "InitMask ...\n";
   InitMask.dump(out, depth + 1);

   out << prefix << "ValidMask ...\n";
   ValidMask.dump(out, depth + 1);

   out << prefix << "LineType ...\n";
   LineType.dump(out, depth + 1);

   out << prefix << "TrackWatchWarn ...\n";
   TrackWatchWarn.dump(out, depth + 1);

   out << prefix << "ColumnThreshMap ...\n";
   for(thr_it = ColumnThreshMap.begin(); thr_it != ColumnThreshMap.end(); thr_it++) {
      out << prefix << thr_it->first << ": \n";
      thr_it->second.dump(out, depth + 1);
   }

   out << prefix << "ColumnStrMap ...\n";
   for(str_it = ColumnStrMap.begin(); str_it != ColumnStrMap.end(); str_it++) {
      out << prefix << str_it->first << ": \n";
      str_it->second.dump(out, depth + 1);
   }

   out << prefix << "InitThreshMap ...\n";
   for(thr_it = InitThreshMap.begin(); thr_it != InitThreshMap.end(); thr_it++) {
      out << prefix << thr_it->first << ": \n";
      thr_it->second.dump(out, depth + 1);
   }

   out << prefix << "InitStrMap ...\n";
   for(str_it = InitStrMap.begin(); str_it != InitStrMap.end(); str_it++) {
      out << prefix << str_it->first << ": \n";
      str_it->second.dump(out, depth + 1);
   }

   out << prefix << "WaterOnly = " << bool_to_string(WaterOnly) << "\n";

   out << prefix << "RapidIntenTrack = " << tracktype_to_string(RapidIntenTrack) << "\n";

   out << prefix << "RapidIntenTimeADeck = " << sec_to_hhmmss(RapidIntenTimeADeck) << "\n";

   out << prefix << "RapidIntenTimeBDeck = " << sec_to_hhmmss(RapidIntenTimeBDeck) << "\n";

   out << prefix << "RapidIntenExactADeck = " << bool_to_string(RapidIntenExactADeck) << "\n";

   out << prefix << "RapidIntenExactBDeck = " << bool_to_string(RapidIntenExactBDeck) << "\n";

   out << prefix << "RapidIntenThreshADeck = " << RapidIntenThreshADeck.get_str() << "\n";

   out << prefix << "RapidIntenThreshBDeck = " << RapidIntenThreshBDeck.get_str() << "\n";

   out << prefix << "RapidIntenWindow = " << sec_to_hhmmss(RapidIntenWindow) << "\n";

   out << prefix << "Landfall = " << bool_to_string(Landfall) << "\n";

   out << prefix << "LandfallBeg = " << LandfallBeg << "\n";

   out << prefix << "LandfallEnd = " << LandfallEnd << "\n";

   out << prefix << "MatchPoints = " << bool_to_string(MatchPoints) << "\n";

   out << prefix << "EventEqual = " << bool_to_string(EventEqual) << "\n";

   out << prefix << "EventEqualLead ...\n";
   EventEqualLead.dump(out, depth + 1);

   out << prefix << "OutInitMask = " << (OutInitMask.name() ? OutInitMask.name() : na_str) << "\n";

   out << prefix << "OutValidMask = " << (OutValidMask.name() ? OutValidMask.name() : na_str) << "\n";

   out << prefix << "DumpFile = " << (DumpFile ? DumpFile.text() : na_str) << "\n";

   out.flush();

   return;
}

////////////////////////////////////////////////////////////////////////

bool TCStatJob::is_keeper_track(const TrackPairInfo &tpi,
                                TCLineCounts &n) const {
   bool keep = true;
   int i, i_init, offset;
   double v_dbl;
   ConcatString v_str;
   map<ConcatString,ThreshArray>::const_iterator thr_it;
   map<ConcatString,StringArray>::const_iterator str_it;

   // Check TrackWatchWarn for each TrackPoint
   if(TrackWatchWarn.n_elements() > 0) {

      // Assume track will not be kept
      keep = false;

      // Loop through the points to see if any are to be kept
      for(i=0; i<tpi.n_points(); i++) {

         // Get the current watch/warning type
         WatchWarnType ww_type = tpi.watch_warn(i);
         
         // Check for the current watch/warn status in the list and the
         // special string ALL:
         //    HUWARN, TSWARN, HUWATCH, TSWATCH
         if(TrackWatchWarn.has(watchwarntype_to_string(ww_type)) ||
            (TrackWatchWarn.has("ALL") &&
             (ww_type == HurricaneWarn     ||
              ww_type == TropicalStormWarn ||
              ww_type == HurricaneWatch    ||
              ww_type == TropicalStormWatch))) {
            keep = true;
            break;
         }
      } // end for i
   } // end if

   // Update counts
   if(!keep) n.RejTrackWatchWarn += tpi.n_points();

   // Get the index of the track initialization point
   i_init = tpi.i_init();

   // Check for bad track initialization point
   if(is_bad_data(i_init)) {
      if(InitThreshMap.size() > 0) {
         keep = false;
         n.RejInitThresh += tpi.n_points();
      }
      else if(InitStrMap.size() > 0) {
         keep = false;
         n.RejInitStr += tpi.n_points();
      }
      else if(OutInitMask.n_points() > 0) {
         keep = false;
         n.RejOutInitMask += tpi.n_points();
      }
   }

   // Check InitThreshMap
   if(keep == true) {

      // Loop through the numeric column thresholds
      for(thr_it = InitThreshMap.begin(); thr_it != InitThreshMap.end(); thr_it++) {
         
         // Get the numeric init column value
         v_dbl = get_column_double(*tpi.line(i_init), thr_it->first);

         // Check the column threshold
         if(is_bad_data(v_dbl) || !thr_it->second.check_dbl(v_dbl)) {
           keep = false;
           n.RejInitThresh++;
           break;
         }
      }
   } 

   // Check InitStr
   if(keep == true) {

      for(str_it = InitStrMap.begin(); str_it != InitStrMap.end(); str_it++) {

         // Determine the column offset and retrieve the value
         offset = determine_column_offset(tpi.line(i_init)->type(), str_it->first);
         v_str  = tpi.line(i_init)->get_item(offset);

         // Check the string value
         if(!str_it->second.has(v_str)) {
            keep = false;
            n.RejInitStr += tpi.n_points();
            break;
         }
      }
   }

   // Check OutInitMask
   if(keep == true) {

      // A mask has been specified
      if(OutInitMask.n_points() > 0) {

         // Check if ADECK init location falls inside the mask
         if(is_bad_data(tpi.adeck()[i_init].lat()) ||
            is_bad_data(tpi.adeck()[i_init].lon()) ||
            !OutInitMask.latlon_is_inside_dege(
               tpi.adeck()[i_init].lat(),
               tpi.adeck()[i_init].lon())) {
            keep = false;
            n.RejOutInitMask++;
         }
      }
   }

   // Update counts
   if(!keep) n.NKeep -= tpi.n_points();

   return(keep);
}

////////////////////////////////////////////////////////////////////////

bool TCStatJob::is_keeper_line(const TCStatLine &line,
                               TCLineCounts &n) const {
   bool keep = true;
   int offset;
   double v_dbl, alat, alon, blat, blon;
   ConcatString v_str;
   StringArray sa;
   map<ConcatString,ThreshArray>::const_iterator thr_it;
   map<ConcatString,StringArray>::const_iterator str_it;

   // Check TC-STAT header columns
   if(AModel.n_elements() > 0 &&
     !AModel.has(line.amodel()))        { keep = false; n.RejAModel++;    }
   else if(BModel.n_elements() > 0 &&
     !BModel.has(line.bmodel()))        { keep = false; n.RejBModel++;    }
   else if(StormId.n_elements() > 0 &&
     !has_storm_id(StormId, line.basin(), line.cyclone(), line.init()))
                                        { keep = false; n.RejStormId++;   }
   else if(Basin.n_elements() > 0 &&
     !Basin.has(line.basin()))          { keep = false; n.RejBasin++;     }
   else if(Cyclone.n_elements() > 0 &&
     !Cyclone.has(line.cyclone()))      { keep = false; n.RejCyclone++;   }
   else if(StormName.n_elements() > 0 &&
     !StormName.has(line.storm_name())) { keep = false; n.RejStormName++; }
   else if(InitBeg > 0 &&
      line.init() < InitBeg)            { keep = false; n.RejInit++;      }
   else if(InitEnd > 0 &&
      line.init() > InitEnd)            { keep = false; n.RejInit++;      }
   else if(InitInc.n_elements() > 0 &&
     !InitInc.has(line.init()))         { keep = false; n.RejInit++;      }
   else if(InitExc.n_elements() > 0 &&
     InitExc.has(line.init()))          { keep = false; n.RejInit++;      }
   else if(InitHour.n_elements() > 0 &&
     !InitHour.has(line.init_hour()))   { keep = false; n.RejInitHour++;  }
   else if(Lead.n_elements() > 0 &&
     !Lead.has(line.lead()))            { keep = false; n.RejLead++;      }
   else if(ValidBeg > 0 &&
      line.valid() < ValidBeg)          { keep = false; n.RejValid++;     }
   else if(ValidEnd > 0 &&
      line.valid() > ValidEnd)          { keep = false; n.RejValid++;     }
   else if(ValidInc.n_elements() > 0 &&
     !ValidInc.has(line.valid()))       { keep = false; n.RejValid++;     }
   else if(ValidExc.n_elements() > 0 &&
     ValidExc.has(line.valid()))        { keep = false; n.RejValid++;     }
   else if(ValidHour.n_elements() > 0 &&
     !ValidHour.has(line.valid_hour())) { keep = false; n.RejValidHour++; }
   else if(InitMask.n_elements() > 0 &&
     !InitMask.has(line.init_mask()))   { keep = false; n.RejInitMask++;  }
   else if(ValidMask.n_elements() > 0 &&
     !ValidMask.has(line.valid_mask())) { keep = false; n.RejValidMask++; }
   else if(LineType.n_elements() > 0 &&
     !LineType.has(line.line_type()))   { keep = false; n.RejLineType++;  }

   // Check ColumnThreshMap
   if(keep == true) {

      // Loop through the numeric column thresholds
      for(thr_it = ColumnThreshMap.begin(); thr_it != ColumnThreshMap.end(); thr_it++) {
         
         // Get the numeric column value
         v_dbl = get_column_double(line, thr_it->first);

         // Check the column threshold
         if(is_bad_data(v_dbl) || !thr_it->second.check_dbl(v_dbl)) {
           keep = false;
           n.RejColumnThresh++;
           break;
         }
      }
   }
   
   // Check ColumnStrMap
   if(keep == true) {

      // Loop through the column string matching
      for(str_it = ColumnStrMap.begin(); str_it != ColumnStrMap.end(); str_it++) {
         
         // Determine the column offset and retrieve the value
         offset = determine_column_offset(line.type(), str_it->first);
         v_str  = line.get_item(offset);

         // Check the string value
         if(!str_it->second.has(v_str)) {
           keep = false;
           n.RejColumnStr++;
           break;
         }
      }
   }

   // Retrieve the ADECK and BDECK lat/lon values
   alat = atof(line.get_item("ALAT"));
   alon = atof(line.get_item("ALON"));
   blat = atof(line.get_item("BLAT"));
   blon = atof(line.get_item("BLON"));

   // Check MatchPoints
   if(keep == true && MatchPoints == true) {

      // Check that the ADECK and BDECK locations are both defined
      if(is_bad_data(alat) || is_bad_data(alon) ||
         is_bad_data(blat) || is_bad_data(blon)) {
         keep = false;
         n.RejMatchPoints++;
      }
   }

   // Check the event equalization cases
   if(keep == true && EventEqualSet == true &&
     !EventEqualCases.has(line.header())) {
      keep = false;
      n.RejEventEqual++;
   }

   // Check OutValidMask
   if(keep == true && OutValidMask.n_points() > 0) {

      // Check ADECK locations
      if(is_bad_data(alat) ||
         is_bad_data(alon) ||
         !OutValidMask.latlon_is_inside_dege(alat, alon)) {
         keep = false;
         n.RejOutValidMask++;
      }
   }

   // Update counts
   if(!keep) n.NKeep -= 1;

   return(keep);
}

////////////////////////////////////////////////////////////////////////

double TCStatJob::get_column_double(const TCStatLine &line,
                                    const ConcatString &column) const {
   StringArray sa;
   ConcatString in;
   double v, v_cur;
   bool abs_flag = false;
   int i;

   // Check for absolute value
   if(strncasecmp(column, "ABS", 3) == 0) {
      abs_flag = true;
      sa = column.split("()");
      in = sa[1];
   }
   else {
      in = column;
   }

   // Split the input column name on hyphens for differences
   sa = in.split("-");

   // Get the first value
   v = atof(line.get_item(sa[0]));

   // If multiple columns, compute the requested difference
   if(sa.n_elements() > 1) {

      // Loop through the column
      for(i=1; i<sa.n_elements(); i++) {

         // Get the current column value
         v_cur = atof(line.get_item(sa[i]));

         // Compute the difference, checking for bad data
         if(is_bad_data(v) || is_bad_data(v_cur)) v  = bad_data_double;
         else                                     v -= v_cur;
      }
   }

   // Apply absolute value, if requested
   if(abs_flag && !is_bad_data(v)) v = fabs(v);

   return(v);
}

////////////////////////////////////////////////////////////////////////

StringArray TCStatJob::parse_job_command(const char *jobstring) {
   StringArray a, b;
   const char * c = (const char *) 0;
   int i;

   // Parse the jobstring into a StringArray
   a.parse_wsss(jobstring);

   // Loop over the StringArray elements
   for(i=0; i<a.n_elements(); i++) {

      c = a[i];

      // Check for a job command option
      if(c[0] != '-') {
         b.add(a[i]);
         continue;
      }

      // Check job command options
           if(strcasecmp(c, "-job"                     ) == 0) { JobType = string_to_tcstatjobtype(a[i+1]);       a.shift_down(i, 1); }
      else if(strcasecmp(c, "-amodel"                  ) == 0) { AModel.add_css(a[i+1]);                          a.shift_down(i, 1); }
      else if(strcasecmp(c, "-bmodel"                  ) == 0) { BModel.add_css(a[i+1]);                          a.shift_down(i, 1); }
      else if(strcasecmp(c, "-storm_id"                ) == 0) { StormId.add_css(a[i+1]);                         a.shift_down(i, 1); }
      else if(strcasecmp(c, "-basin"                   ) == 0) { Basin.add_css(a[i+1]);                           a.shift_down(i, 1); }
      else if(strcasecmp(c, "-cyclone"                 ) == 0) { Cyclone.add_css(a[i+1]);                         a.shift_down(i, 1); }
      else if(strcasecmp(c, "-storm_name"              ) == 0) { StormName.add_css(a[i+1]);                       a.shift_down(i, 1); }
      else if(strcasecmp(c, "-init_beg"                ) == 0) { InitBeg = timestring_to_unix(a[i+1]);            a.shift_down(i, 1); }
      else if(strcasecmp(c, "-init_end"                ) == 0) { InitEnd = timestring_to_unix(a[i+1]);            a.shift_down(i, 1); }
      else if(strcasecmp(c, "-init_inc"                ) == 0) { InitInc.add_css(a[i+1]);                         a.shift_down(i, 1); }
      else if(strcasecmp(c, "-init_exc"                ) == 0) { InitExc.add_css(a[i+1]);                         a.shift_down(i, 1); }
      else if(strcasecmp(c, "-init_hour"               ) == 0) { InitHour.add_css_sec(a[i+1]);                    a.shift_down(i, 1); }
      else if(strcasecmp(c, "-lead"                    ) == 0) { Lead.add_css_sec(a[i+1]);                        a.shift_down(i, 1); }
      else if(strcasecmp(c, "-valid_beg"               ) == 0) { ValidBeg = timestring_to_unix(a[i+1]);           a.shift_down(i, 1); }
      else if(strcasecmp(c, "-valid_end"               ) == 0) { ValidEnd = timestring_to_unix(a[i+1]);           a.shift_down(i, 1); }
      else if(strcasecmp(c, "-valid_inc"               ) == 0) { ValidInc.add_css(a[i+1]);                        a.shift_down(i, 1); }
      else if(strcasecmp(c, "-valid_exc"               ) == 0) { ValidExc.add_css(a[i+1]);                        a.shift_down(i, 1); }
      else if(strcasecmp(c, "-valid_hour"              ) == 0) { ValidHour.add_css_sec(a[i+1]);                   a.shift_down(i, 1); }
      else if(strcasecmp(c, "-init_mask"               ) == 0) { InitMask.add_css(a[i+1]);                        a.shift_down(i, 1); }
      else if(strcasecmp(c, "-valid_mask"              ) == 0) { ValidMask.add_css(a[i+1]);                       a.shift_down(i, 1); }
      else if(strcasecmp(c, "-line_type"               ) == 0) { LineType.add_css(a[i+1]);                        a.shift_down(i, 1); }
      else if(strcasecmp(c, "-track_watch_warn"        ) == 0) { TrackWatchWarn.add_css(a[i+1]);                  a.shift_down(i, 1); }
      else if(strcasecmp(c, "-column_thresh"           ) == 0) { parse_thresh_option(a[i+1], a[i+2], ColumnThreshMap);
                                                                                                                  a.shift_down(i, 2); }
      else if(strcasecmp(c, "-column_str"              ) == 0) { parse_string_option(a[i+1], a[i+2], ColumnStrMap);
                                                                                                                  a.shift_down(i, 2); }
      else if(strcasecmp(c, "-init_thresh"             ) == 0) { parse_thresh_option(a[i+1], a[i+2], InitThreshMap);
                                                                                                                  a.shift_down(i, 2); }
      else if(strcasecmp(c, "-init_str"                ) == 0) { parse_string_option(a[i+1], a[i+2], InitStrMap);
                                                                                                                  a.shift_down(i, 2); }
      else if(strcasecmp(c, "-water_only"              ) == 0) { WaterOnly = string_to_bool(a[i+1]);              a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_track"       ) == 0) { RapidIntenTrack = string_to_tracktype(a[i+1]);   a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_time"        ) == 0) { RapidIntenTimeADeck = timestring_to_sec(a[i+1]);
                                                                 RapidIntenTimeBDeck = timestring_to_sec(a[i+1]); a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_time_adeck"  ) == 0) { RapidIntenTimeADeck = timestring_to_sec(a[i+1]); a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_time_bdeck"  ) == 0) { RapidIntenTimeBDeck = timestring_to_sec(a[i+1]); a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_exact"       ) == 0) { RapidIntenExactADeck = string_to_bool(a[i+1]);
                                                                 RapidIntenExactBDeck = string_to_bool(a[i+1]);   a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_exact_adeck" ) == 0) { RapidIntenExactADeck = string_to_bool(a[i+1]);   a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_exact_bdeck" ) == 0) { RapidIntenExactBDeck = string_to_bool(a[i+1]);   a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_thresh"      ) == 0) { RapidIntenThreshADeck.set(a[i+1]);
                                                                 RapidIntenThreshBDeck.set(a[i+1]);               a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_thresh_adeck") == 0) { RapidIntenThreshADeck.set(a[i+1]);               a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_thresh_bdeck") == 0) { RapidIntenThreshBDeck.set(a[i+1]);               a.shift_down(i, 1); }
      else if(strcasecmp(c, "-rapid_inten_window"      ) == 0) { RapidIntenWindow = timestring_to_sec(a[i+1]);    a.shift_down(i, 1); }
      else if(strcasecmp(c, "-landfall"                ) == 0) { Landfall = string_to_bool(a[i+1]);               a.shift_down(i, 1); }
      else if(strcasecmp(c, "-landfall_beg"            ) == 0) { LandfallBeg = atoi(a[i+1]);                      a.shift_down(i, 1); }
      else if(strcasecmp(c, "-landfall_end"            ) == 0) { LandfallEnd = atoi(a[i+1]);                      a.shift_down(i, 1); }
      else if(strcasecmp(c, "-match_points"            ) == 0) { MatchPoints = string_to_bool(a[i+1]);            a.shift_down(i, 1); }
      else if(strcasecmp(c, "-event_equal"             ) == 0) { EventEqual = string_to_bool(a[i+1]);             a.shift_down(i, 1); }
      else if(strcasecmp(c, "-event_equal_lead"        ) == 0) { EventEqualLead.add_css_sec(a[i+1]);              a.shift_down(i, 1); }
      else if(strcasecmp(c, "-out_init_mask"           ) == 0) { set_mask(OutInitMask, a[i+1]);                   a.shift_down(i, 1); }
      else if(strcasecmp(c, "-out_valid_mask"          ) == 0) { set_mask(OutValidMask, a[i+1]);                  a.shift_down(i, 1); }
      else if(strcasecmp(c, "-dump_row"                ) == 0) { DumpFile = a[i+1]; open_dump_file();             a.shift_down(i, 1); }
      else                                                     {                                                  b.add(a[i]);        }
   }

   return(b);
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::set_mask(MaskPoly &p, const char *file) {

   ConcatString poly_file = replace_path(file);
   p.clear();
   p.load(poly_file);

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::open_dump_file() {

   close_dump_file();

   if(DumpFile.empty()) return;

   DumpOut = new ofstream;
   DumpOut->open(DumpFile);

   if(!DumpOut) {
      mlog << Error << "\nTCStatJob::open_dump_file()-> "
           << "can't open the output file \"" << DumpFile
           << "\" for writing!\n\n";
      exit(1);
   }

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::close_dump_file() {

   if(DumpOut) {

      mlog << Debug(1)
           << "Creating output dump file: " << DumpFile << "\n";

      DumpOut->close();
      delete DumpOut;
      DumpOut = (ofstream *) 0;
   }

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::dump_track_pair(const TrackPairInfo &tpi, ofstream *out) {

   if(!out || tpi.n_points() == 0) return;

   TcHdrColumns tchc;
   AsciiTable out_at;
   int i_row, hdr_row;

   // Determine if we need to write a header row
   if((int) out->tellp() == 0) hdr_row = 1;
   else                        hdr_row = 0;

   // Initialize the output AsciiTable
   out_at.set_size(tpi.n_points() + hdr_row,
                   n_tc_header_cols + n_tc_mpr_cols);

   // Write the TCMPR header row
   write_tc_mpr_header_row(1, out_at, 0, 0);

   // Setup the output AsciiTable
   justify_tc_stat_cols(out_at);
   out_at.set_precision(default_precision);
   out_at.set_bad_data_value(bad_data_double);
   out_at.set_bad_data_str(na_str);
   out_at.set_delete_trailing_blank_rows(1);

   // Setup header columns
   tchc.clear();
   tchc.set_adeck_model(tpi.adeck().technique());
   tchc.set_bdeck_model(tpi.bdeck().technique());
   tchc.set_storm_id(tpi.adeck().storm_id());
   tchc.set_basin(tpi.bdeck().basin());
   tchc.set_cyclone(tpi.bdeck().cyclone());
   tchc.set_storm_name(tpi.bdeck().storm_name());

   if(OutInitMask.n_points() > 0)  tchc.set_init_mask(OutInitMask.name());
   else                            tchc.set_init_mask(na_str);
   if(OutValidMask.n_points() > 0) tchc.set_valid_mask(OutValidMask.name());
   else                            tchc.set_valid_mask(na_str);

   // Write the TrackPairInfo object
   i_row = hdr_row;
   write_tc_mpr_row(tchc, tpi, out_at, i_row);

   // Write the AsciiTable to the file
   *out << out_at;

   return;
}

////////////////////////////////////////////////////////////////////////

ConcatString TCStatJob::serialize() const {
   ConcatString s;
   int i;
   map<ConcatString,ThreshArray>::const_iterator thr_it;
   map<ConcatString,StringArray>::const_iterator str_it;

   // Initialize the jobstring
   s.clear();
   s.set_precision(default_precision);

   if(JobType != NoTCStatJobType)
      s << "-job " << tcstatjobtype_to_string(JobType) << " ";
   for(i=0; i<AModel.n_elements(); i++)
      s << "-amodel " << AModel[i] << " ";
   for(i=0; i<BModel.n_elements(); i++)
      s << "-bmodel " << BModel[i] << " ";
   for(i=0; i<StormId.n_elements(); i++)
      s << "-storm_id " << StormId[i] << " ";
   for(i=0; i<Basin.n_elements(); i++)
      s << "-basin " << Basin[i] << " ";
   for(i=0; i<Cyclone.n_elements(); i++)
      s << "-cyclone " << Cyclone[i] << " ";
   for(i=0; i<StormName.n_elements(); i++)
      s << "-storm_name " << StormName[i] << " ";
   if(InitBeg > 0)
      s << "-init_beg " << unix_to_yyyymmdd_hhmmss(InitBeg) << " ";
   if(InitEnd > 0)
      s << "-init_end " << unix_to_yyyymmdd_hhmmss(InitEnd) << " ";
   for(i=0; i<InitInc.n_elements(); i++)
      s << "-init_inc " << unix_to_yyyymmdd_hhmmss(InitInc[i]) << " ";
   for(i=0; i<InitExc.n_elements(); i++)
      s << "-init_exc " << unix_to_yyyymmdd_hhmmss(InitExc[i]) << " ";
   for(i=0; i<InitHour.n_elements(); i++)
      s << "-init_hour " << sec_to_hhmmss(nint(InitHour[i])) << " ";
   for(i=0; i<Lead.n_elements(); i++)
      s << "-lead " << sec_to_hhmmss(nint(Lead[i])) << " ";
   if(ValidBeg > 0)
      s << "-valid_beg " << unix_to_yyyymmdd_hhmmss(ValidBeg) << " ";
   if(ValidEnd > 0)
      s << "-valid_end " << unix_to_yyyymmdd_hhmmss(ValidEnd) << " ";
   for(i=0; i<ValidInc.n_elements(); i++)
      s << "-valid_inc " << unix_to_yyyymmdd_hhmmss(ValidInc[i]) << " ";
   for(i=0; i<ValidExc.n_elements(); i++)
      s << "-valid_exc " << unix_to_yyyymmdd_hhmmss(ValidExc[i]) << " ";
   for(i=0; i<ValidHour.n_elements(); i++)
      s << "-valid_hour " << sec_to_hhmmss(nint(ValidHour[i])) << " ";
   for(i=0; i<InitMask.n_elements(); i++)
      s << "-init_mask " << InitMask[i] << " ";
   for(i=0; i<ValidMask.n_elements(); i++)
      s << "-valid_mask " << ValidMask[i] << " ";
   for(i=0; i<LineType.n_elements(); i++)
      s << "-line_type " << LineType[i] << " ";
   for(i=0; i<TrackWatchWarn.n_elements(); i++)
      s << "-track_watch_warn " << TrackWatchWarn[i] << " ";
   for(thr_it = ColumnThreshMap.begin(); thr_it != ColumnThreshMap.end(); thr_it++) {
      for(i=0; i<thr_it->second.n_elements(); i++) {
         s << "-column_thresh " << thr_it->first << " "
           << thr_it->second[i].get_str() << " ";
      }
   }
   for(str_it = ColumnStrMap.begin(); str_it != ColumnStrMap.end(); str_it++) {
      for(i=0; i<str_it->second.n_elements(); i++) {
         s << "-column_str " << str_it->first << " "
           << str_it->second[i] << " ";
      }
   }
   for(thr_it = InitThreshMap.begin(); thr_it != InitThreshMap.end(); thr_it++) {
      for(i=0; i<thr_it->second.n_elements(); i++) {
         s << "-init_thresh " << thr_it->first << " "
           << thr_it->second[i].get_str() << " ";
      }
   }
   for(str_it = InitStrMap.begin(); str_it != InitStrMap.end(); str_it++) {
      for(i=0; i<str_it->second.n_elements(); i++) {
         s << "-init_str " << str_it->first << " "
           << str_it->second[i] << " ";
      }
   }
   if(WaterOnly != default_water_only)
      s << "-water_only " << bool_to_string(WaterOnly) << " ";
   if(RapidIntenTrack != default_rapid_inten_track) {
      s << "-rapid_inten_track " << tracktype_to_string(RapidIntenTrack) << " ";
      if(RapidIntenTimeADeck == RapidIntenTimeBDeck) {
         s << "-rapid_inten_time " << sec_to_hhmmss(RapidIntenTimeADeck) << " ";
      }
      else {
         s << "-rapid_inten_time_adeck " << sec_to_hhmmss(RapidIntenTimeADeck) << " "
           << "-rapid_inten_time_bdeck " << sec_to_hhmmss(RapidIntenTimeBDeck) << " ";
      }
      if(RapidIntenExactADeck == RapidIntenExactBDeck) {
         s << "-rapid_inten_exact " << bool_to_string(RapidIntenExactADeck) << " ";
      }
      else {
         s << "-rapid_inten_exact_adeck " << bool_to_string(RapidIntenExactADeck) << " "
           << "-rapid_inten_exact_bdeck " << bool_to_string(RapidIntenExactBDeck) << " ";
      }
      if(RapidIntenThreshADeck == RapidIntenThreshBDeck) {
         s << "-rapid_inten_thresh " << RapidIntenThreshADeck.get_str() << " ";
      }
      else {
         s << "-rapid_inten_thresh_adeck " << RapidIntenThreshADeck.get_str() << " "
           << "-rapid_inten_thresh_bdeck " << RapidIntenThreshBDeck.get_str() << " ";
      }
      s << "-rapid_inten_window " << sec_to_hhmmss(RapidIntenWindow) << " ";
   }
   if(Landfall != default_landfall) {
      s << "-landfall " << bool_to_string(Landfall) << " "
        << "-landfall_beg " << LandfallBeg << " "
        << "-landfall_end " << LandfallEnd << " ";
   }
   if(MatchPoints != default_match_points)
      s << "-match_points " << bool_to_string(MatchPoints) << " ";
   if(EventEqual != default_event_equal)
      s << "-event_equal " << bool_to_string(EventEqual) << " ";
   for(i=0; i<EventEqualLead.n_elements(); i++)
      s << "-event_equal_lead " << sec_to_hhmmss(nint(EventEqualLead[i])) << " ";
   if(OutInitMask.n_points() > 0)
      s << "-out_init_mask " << OutInitMask.file_name() << " ";
   if(OutValidMask.n_points() > 0)
      s << "-out_valid_mask " << OutValidMask.file_name() << " ";
   if(DumpFile.length() > 0)
      s << "-dump_row " << DumpFile << " ";

   return(s);
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::do_job(const StringArray &file_list, TCLineCounts &n) {

   // Add the input file list
   TCSTFiles.add_files(file_list);

   // Apply the event equalization logic to build a list of common cases
   if(EventEqual == true) process_event_equal();

   return;
}

////////////////////////////////////////////////////////////////////////
//
// Apply event equalization logic to build list of common cases.
//
////////////////////////////////////////////////////////////////////////

void TCStatJob::process_event_equal() {
   TrackPairInfo tpi;
   TCLineCounts n;
   ConcatString key, val;
   StringArray case_list;
   ConcatString models;
   bool skip;
   int i;

   // Event equalization case map
   map<ConcatString,StringArray,cs_cmp> case_map;
   map<ConcatString,StringArray,cs_cmp>::iterator it;

   mlog << Debug(3)
        << "Applying event equalization logic.\n";

   // Rewind to the beginning of the track pair input
   TCSTFiles.rewind();

   // Process each of the track pairs
   while(TCSTFiles >> tpi) {

      // Subset the track pair down to points to be used
      subset_track_pair(tpi, n);

      // Skip tracks missing any of the required lead times
      for(i=0, skip=false; i<EventEqualLead.n_elements(); i++) {
         if(tpi.adeck().lead_index(EventEqualLead[i]) < 0) {
            skip = true;
            break;
         }
      }
      if(skip) continue;

      // Add event equalization information for each track point
      for(i=0; i<tpi.n_lines(); i++) {

         // Store the current map key and value
         key = tpi.adeck().technique(); // ADECK model name
         val = tpi.line(i)->header();   // Track line header

         // Add a new map entry, if necessary
         if(case_map.count(key) == 0) {
            case_list.clear();
            case_map[key] = case_list;
         }

         // Add the current header to the case map
         if(!case_map[key].has(val)) case_map[key].add(val);

      } // end for i
   } // end while

   // Initialize to the first map entry
   EventEqualCases.clear();
   if(case_map.size() > 0) EventEqualCases = case_map.begin()->second;

   // Loop over the map entries and build a list of common cases
   for(it = case_map.begin(); it != case_map.end(); it++) {
      EventEqualCases = intersection(EventEqualCases, it->second);
      models << it->first << " ";
   } // end for it

   mlog << Debug(3)
        << "For event equalization, identified "
        << EventEqualCases.n_elements() << " common cases for "
        << (int) case_map.size() << " models: " << models << "\n";

   for(i=0; i<EventEqualCases.n_elements(); i++) {
      mlog << Debug(4)
           << "[Case " << i+1 << " of " << EventEqualCases.n_elements()
           << "] " << EventEqualCases[i] << "\n";
   }

   // Set flag to indicate that event equalization has been applied.
   EventEqualSet = true;

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJob::subset_track_pair(TrackPairInfo &tpi, TCLineCounts &n) {
   int i, n_rej;

   // Check for no points
   if(tpi.n_points() == 0) return;

   // Increment the read and keep counts
   n.NRead += tpi.n_points();
   n.NKeep += tpi.n_points();

   // Determine if the whole track can be discarded
   if(!is_keeper_track(tpi, n)) {
      tpi.clear();
      return;
   }

   // Check WaterOnly
   if(WaterOnly == true) {

      // Determine the rapid intensification points
      n_rej = tpi.check_water_only();

      // Update counts
      n.RejWaterOnly += n_rej;
      n.NKeep        -= n_rej;
   }

   // Check RapidInten
   if(RapidIntenTrack != TrackType_None) {

      // Determine the rapid intensification points
      n_rej = tpi.check_rapid_inten(RapidIntenTrack,
                                    RapidIntenTimeADeck, RapidIntenTimeBDeck, 
                                    RapidIntenExactADeck, RapidIntenExactBDeck,
                                    RapidIntenThreshADeck, RapidIntenThreshBDeck);

      // Update counts
      n.RejRapidInten += n_rej;
      n.NKeep         -= n_rej;
   }

   // Check Landfall
   if(Landfall == true) {

      // Determine the landfall points
      n_rej = tpi.check_landfall(LandfallBeg, LandfallEnd);

      // Update counts
      n.RejLandfall += n_rej;
      n.NKeep       -= n_rej;
   }

   // Loop over the track points to check line by line
   for(i=0; i<tpi.n_lines(); i++) {

      // Skip marked lines
      if(!tpi.keep(i)) continue;

      // Check if this line should be discarded
      if(!is_keeper_line(*tpi.line(i), n)) tpi.set_keep(i, 0);
   }

   // Subset the points marked for retention
   tpi = tpi.keep_subset();
   
   return;
}

////////////////////////////////////////////////////////////////////////
//
// Code for class TCStatJobFilter
//
////////////////////////////////////////////////////////////////////////

TCStatJobFilter::TCStatJobFilter() {

   init_from_scratch();
}

////////////////////////////////////////////////////////////////////////

TCStatJobFilter::~TCStatJobFilter() {

   clear();
}

////////////////////////////////////////////////////////////////////////

TCStatJobFilter::TCStatJobFilter(const TCStatJobFilter &j) {

   init_from_scratch();

   assign(j);
}

////////////////////////////////////////////////////////////////////////

TCStatJobFilter & TCStatJobFilter::operator=(const TCStatJobFilter &j) {

   if(this == &j) return(*this);

   assign(j);

   return(*this);
}

////////////////////////////////////////////////////////////////////////

void TCStatJobFilter::init_from_scratch() {

   TCStatJob::init_from_scratch();

   clear();

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobFilter::clear() {

   TCStatJob::clear();

   JobType = TCStatJobType_Filter;

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobFilter::assign(const TCStatJobFilter & j) {

   TCStatJob::assign(j);

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobFilter::do_job(const StringArray &file_list,
                             TCLineCounts &n) {
   TrackPairInfo tpi;

   // Check that the -dump_row option has been supplied
   if(!DumpOut) {
      mlog << Error << "\nTCStatJobFilter::do_job() -> "
           << "this function may only be called when using the "
           << "-dump_row option in the job command line: "
           << serialize() << "\n\n";
      exit(1);
   }

   // Call the parent's do_job() to do event equalization
   TCStatJob::do_job(file_list, n);

   // Check for no common cases
   if(EventEqualSet == true && EventEqualCases.n_elements() == 0) {
      mlog << Debug(1)
           << "Event equalization found no common cases.\n";
   }
   // Otherwise, process the track data
   else {

      // Rewind to the beginning of the track pair input
      TCSTFiles.rewind();

      // Process each of the track pairs
      while(TCSTFiles >> tpi) {

         // Process the track pair down to points to be used
         subset_track_pair(tpi, n);

         // Write out the retained lines
         if(tpi.n_points() > 0) {

            mlog << Debug(4)
                 << "Processing track pair: " << tpi.case_info() << "\n";

            if(DumpOut) dump_track_pair(tpi, DumpOut);
         }
      } // end while
   } // end else

   // Close the dump file
   if(DumpOut) close_dump_file();

   // Process the filter output
   if(JobOut) do_output(*JobOut);
   else       do_output(cout);

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobFilter::do_output(ostream &out) {
   ConcatString line;

   // Build a simple output line
   line << "FILTER: " << serialize() << "\n";
   out  << line << "\n";

   return;
}

////////////////////////////////////////////////////////////////////////
//
// Code for class TCStatJobSummary
//
////////////////////////////////////////////////////////////////////////

TCStatJobSummary::TCStatJobSummary() {

   init_from_scratch();
}

////////////////////////////////////////////////////////////////////////

TCStatJobSummary::~TCStatJobSummary() {

   clear();
}

////////////////////////////////////////////////////////////////////////

TCStatJobSummary::TCStatJobSummary(const TCStatJobSummary &j) {

   init_from_scratch();

   assign(j);
}

////////////////////////////////////////////////////////////////////////

TCStatJobSummary & TCStatJobSummary::operator=(const TCStatJobSummary &j) {

   if(this == &j) return(*this);

   assign(j);

   return(*this);
}

////////////////////////////////////////////////////////////////////////

void TCStatJobSummary::init_from_scratch() {

   TCStatJob::init_from_scratch();

   // Ignore case when performing comparisons
   ReqColumn.set_ignore_case(1);
   Column.set_ignore_case(1);
   CaseColumn.set_ignore_case(1);

   clear();

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobSummary::clear() {

   TCStatJob::clear();

   JobType = TCStatJobType_Summary;

   ReqColumn.clear();
   Column.clear();
   CaseColumn.clear();
   SummaryMap.clear();

   // Set to default value
   OutAlpha  = default_tc_alpha;
   FSPThresh = default_fsp_thresh;

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobSummary::assign(const TCStatJobSummary & j) {

   TCStatJob::assign(j);

   ReqColumn = j.ReqColumn;
   Column = j.Column;
   CaseColumn = j.CaseColumn;
   SummaryMap = j.SummaryMap;
   OutAlpha = j.OutAlpha;
   FSPThresh = j.FSPThresh;

   return;
}

////////////////////////////////////////////////////////////////////////

StringArray TCStatJobSummary::parse_job_command(const char *jobstring) {
   StringArray a, b;
   const char * c = (const char *) 0;
   int i;

   // Call the parent and store any unused options
   a = TCStatJob::parse_job_command(jobstring);

   // Loop over the StringArray elements
   for(i=0; i<a.n_elements(); i++) {

      // Point at the current entry
      c = a[i];

      // Check for a job command option
      if(c[0] != '-') {
         b.add(a[i]);
         continue;
      }

      // Check job command options
           if(strcasecmp(c, "-column"    ) == 0) { ReqColumn.add_css(to_upper(a[i+1]));
                                                   add_column(a[i+1]);                   a.shift_down(i, 1); }
      else if(strcasecmp(c, "-by"        ) == 0) { CaseColumn.add_css(to_upper(a[i+1])); a.shift_down(i, 1); }
      else if(strcasecmp(c, "-out_alpha" ) == 0) { OutAlpha = atof(a[i+1]);              a.shift_down(i, 1); }
      else if(strcasecmp(c, "-fsp_thresh") == 0) { FSPThresh.set(a[i+1]);                a.shift_down(i, 1); }
      else                                       {                                       b.add(a[i]);        }
   }

   return(b);
}

////////////////////////////////////////////////////////////////////////

void TCStatJobSummary::add_column(const char *c) {
   int i, j;
   StringArray sa;

   // Parse entries into a StringArray
   sa.add_css(c);

   // Loop over the entries, handling special column names
   for(i=0; i<sa.n_elements(); i++) {

      // Track errors
      if(strcasecmp(sa[i], "TRACK") == 0) {
         for(j=0; j<n_tc_cols_track; j++) Column.add(tc_cols_track[j]);
      }
      // Wind errors
      else if(strcasecmp(sa[i], "WIND") == 0) {
         for(j=0; j<n_tc_cols_wind; j++) Column.add(tc_cols_wind[j]);
      }
      // Track and Intensity (TI)
      else if(strcasecmp(sa[i], "TI") == 0) {
         for(j=0; j<n_tc_cols_ti; j++) Column.add(tc_cols_ti[j]);
      }
      // Along and Cross Track (AC)
      else if(strcasecmp(sa[i], "AC") == 0) {
         for(j=0; j<n_tc_cols_ac; j++) Column.add(tc_cols_ac[j]);
      }
      // Lat/Lon Difference (XY)
      else if(strcasecmp(sa[i], "XY") == 0) {
         for(j=0; j<n_tc_cols_xy; j++) Column.add(tc_cols_xy[j]);
      }
      // Otherwise, just add the column name
      else {
         Column.add(sa[i]);
      }
   } // end for i

   return;
}

////////////////////////////////////////////////////////////////////////

ConcatString TCStatJobSummary::serialize() const {
   ConcatString s;
   int i;

   // Call the parent
   s = TCStatJob::serialize();

   // Add summary job-specific options
   for(i=0; i<ReqColumn.n_elements(); i++)
      s << "-column " << ReqColumn[i] << " ";
   for(i=0; i<CaseColumn.n_elements(); i++)
      s << "-by " << CaseColumn[i] << " ";
   if(!(FSPThresh == default_fsp_thresh))
      s << "-fsp_thresh " << FSPThresh.get_str();

   // Always list the output alpha value used
   s << "-out_alpha " << OutAlpha << " ";

   return(s);
}

////////////////////////////////////////////////////////////////////////

void TCStatJobSummary::do_job(const StringArray &file_list,
                              TCLineCounts &n) {
   TrackPairInfo tpi;

   // Check that the -column option has been supplied
   if(Column.n_elements() == 0) {
      mlog << Error << "\nTCStatJobSummary::do_job() -> "
           << "this function may only be called when using the "
           << "-column option in the job command line: "
           << serialize() << "\n\n";
      exit(1);
   }

   // Call the parent's do_job() to do event equalization
   TCStatJob::do_job(file_list, n);

   // Check for no common cases
   if(EventEqualSet == true && EventEqualCases.n_elements() == 0) {
      mlog << Debug(1)
           << "Event equalization found no common cases.\n";
   }
   // Otherwise, process the track data
   else {

      // Rewind to the beginning of the track pair input
      TCSTFiles.rewind();

      // Process each of the track pairs
      while(TCSTFiles >> tpi) {

         // Process the track pair down to points to be used
         subset_track_pair(tpi, n);

         // Write out the retained lines
         if(tpi.n_points() > 0) {

            mlog << Debug(4)
                 << "Processing track pair: " << tpi.case_info() << "\n";

            // Process the track pair info for the summary job
            process_track_pair(tpi);

            if(DumpOut) dump_track_pair(tpi, DumpOut);
         }
      } // end while
   } // end else

   // Close the dump file
   if(DumpOut) close_dump_file();

   // Process the summary output
   if(JobOut) do_output(*JobOut);
   else       do_output(cout);

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobSummary::process_track_pair(TrackPairInfo &tpi) {
   int i, j, k, lead;
   map<ConcatString,SummaryMapData,cs_cmp> cur_map;
   ConcatString key, cur;
   SummaryMapData data;
   double val;

   // Initialize the map
   cur_map.clear();

   // Loop over TCStatLines and construct a summary map
   for(i=0; i<tpi.n_lines(); i++) {

      // Add summary info to the current map
      for(j=0; j<Column.n_elements(); j++) {

         // Build the key and get the current column value
         key = Column[j];
         val = get_column_double(*tpi.line(i), Column[j]);

         // Add case information to the key
         for(k=0; k<CaseColumn.n_elements(); k++) {

            cur = tpi.line(i)->get_item(CaseColumn[k]);

            // For bad data, use the NA string
            if(is_bad_data(atoi(cur))) cur = na_str;

            // Special handling for lead time:
            // Switch 2-digit hours to 3-digit hours so that the
            // summary job output is sorted nicely.
            if(strcasecmp(CaseColumn[k], "LEAD") == 0 &&
               cur != na_str &&
               abs(lead = hhmmss_to_sec(cur)) < 100*sec_per_hour) {

               // Handle positive and negative lead times
               key << (lead < 0 ? ":-0" : ":0")
                   << sec_to_hhmmss(abs(lead));
            }
            // Otherwise, just append the current case info
            else {
               key << ":" << cur;
            }

         } // end for k

         // Add map entry for this key, if necessary
         if(cur_map.count(key) == 0) cur_map[key] = data;

         // Add values for this key
         cur_map[key].Val.add(val);
         cur_map[key].Hdr.add(tpi.line(i)->header());
         cur_map[key].AModel.add(tpi.line(i)->amodel());
         cur_map[key].Init.add(tpi.line(i)->init());
         cur_map[key].Lead.add(tpi.line(i)->lead());
         cur_map[key].Valid.add(tpi.line(i)->valid());

      } // end for j
   } // end for i

   // Add the current map
   add_map(cur_map);

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobSummary::add_map(map<ConcatString,SummaryMapData,cs_cmp>&m) {
   map<ConcatString,SummaryMapData,cs_cmp>::iterator it;

   // Loop over the input map entries
   for(it = m.begin(); it != m.end(); it++) {

      // Current map key is not yet defined in SummaryMap
      if(SummaryMap.count(it->first) == 0) {

         mlog << Debug(5)
              << "Summary Map Insert (" << it->first << ") "
              << it->second.Val.n_elements() << " values: "
              << it->second.Val.serialize() << "\n";

         // Add the pair to the map
         SummaryMap[it->first] = it->second;
      }
      // Current map key is defined in SummaryMap
      else {

         mlog << Debug(5)
              << "Summary Map Add (" << it->first << ") "
              << it->second.Val.n_elements() << " values: "
              << it->second.Val.serialize() << "\n";

         // Add the value for the existing key
         SummaryMap[it->first].Val.add(it->second.Val);
         SummaryMap[it->first].Hdr.add(it->second.Hdr);
         SummaryMap[it->first].AModel.add(it->second.AModel);
         SummaryMap[it->first].Init.add(it->second.Init);
         SummaryMap[it->first].Lead.add(it->second.Lead);
         SummaryMap[it->first].Valid.add(it->second.Valid);
      }
   } // end for it

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobSummary::do_output(ostream &out) {
   map<ConcatString,SummaryMapData,cs_cmp>::iterator it;
   StringArray sa;
   ConcatString line;
   AsciiTable out_at;
   NumArray v, lead, index;
   TimeArray init, valid;
   CIInfo mean_ci, stdev_ci;
   int i, r, c, dsec, tind;

   double fsp;
   NumArray fsp_total, fsp_best, fsp_ties;

   // Setup the output table
   out_at.set_size((int) SummaryMap.size() + 1,
                   CaseColumn.n_elements() + 22);
   
   // Left-justify case info and right-justify summary output
   for(i=0; i<out_at.ncols(); i++) {
      if(i < CaseColumn.n_elements()) out_at.set_column_just(i, LeftJust);
      else                            out_at.set_column_just(i, RightJust);
   }
   out_at.set_precision(default_precision);
   out_at.set_bad_data_value(bad_data_double);
   out_at.set_bad_data_str(na_str);
   out_at.set_delete_trailing_blank_rows(1);

   // Set up CIInfo objects
   mean_ci.allocate_n_alpha(1);
   stdev_ci.allocate_n_alpha(1);

   // Initialize row and column indices
   r = c = 0;

   // Write the header row
   out_at.set_entry(r, c++, "COL_NAME:");
   out_at.set_entry(r, c++, "COLUMN");

   // Write case column names
   for(i=0; i<CaseColumn.n_elements(); i++) {
      out_at.set_entry(r, c++, CaseColumn[i]);
   }

   out_at.set_entry(r, c++, "TOTAL");
   out_at.set_entry(r, c++, "VALID");
   out_at.set_entry(r, c++, "MEAN");
   out_at.set_entry(r, c++, "MEAN_NCL");
   out_at.set_entry(r, c++, "MEAN_NCU");
   out_at.set_entry(r, c++, "STDEV");
   out_at.set_entry(r, c++, "MIN");
   out_at.set_entry(r, c++, "P10");
   out_at.set_entry(r, c++, "P25");
   out_at.set_entry(r, c++, "P50");
   out_at.set_entry(r, c++, "P75");
   out_at.set_entry(r, c++, "P90");
   out_at.set_entry(r, c++, "MAX");
   out_at.set_entry(r, c++, "SUM");
   out_at.set_entry(r, c++, "TS_INT");
   out_at.set_entry(r, c++, "TS_IND");
   out_at.set_entry(r, c++, "FSP_TOTAL");
   out_at.set_entry(r, c++, "FSP_BEST");
   out_at.set_entry(r, c++, "FSP_TIES");
   out_at.set_entry(r, c++, "FSP");

   // Compute the frequency of superior performance
   compute_fsp(fsp_total, fsp_best, fsp_ties);

   // Loop over the map entries and popluate the output table
   for(it=SummaryMap.begin(),r=1; it!=SummaryMap.end(); it++,r++) {

      // Initialize column index
      c = 0;

      // Split the current map key
      sa = it->first.split(":");

      // Get the valid subset of data
      v.clear();
      init.clear();
      lead.clear();
      valid.clear();
      for(i=0; i<it->second.Val.n_elements(); i++) {
         if(!is_bad_data(it->second.Val[i])) {
            v.add(it->second.Val[i]);
            init.add(it->second.Init[i]);
            lead.add(it->second.Lead[i]);
            valid.add(it->second.Valid[i]);
         }
      }

      // Build index array
      index.clear();
      for(i=0; i<v.n_elements(); i++) index.add(i);

      // Compute mean and standard deviation
      compute_mean_stdev(v, index, 1, OutAlpha, mean_ci, stdev_ci);

      // Compute the FSP value
      if(fsp_total[r-1] != 0) fsp = fsp_best[r-1]/fsp_total[r-1];
      else                    fsp = bad_data_double;

      // Compute time to independence for time-series data
      if(is_time_series(init, lead, valid, dsec)) {
         tind = compute_time_to_indep(v, dsec);
      }
      else {
         tind = dsec = bad_data_int;
      }

      // Write the table row
      out_at.set_entry(r, c++, "SUMMARY:");
      out_at.set_entry(r, c++, sa[0]);

      // Write case column values
      for(i=1; i<sa.n_elements(); i++)
         out_at.set_entry(r, c++, sa[i]);

      out_at.set_entry(r, c++, it->second.Val.n_elements());
      out_at.set_entry(r, c++, v.n_elements());
      out_at.set_entry(r, c++, mean_ci.v);
      out_at.set_entry(r, c++, mean_ci.v_ncl[0]);
      out_at.set_entry(r, c++, mean_ci.v_ncu[0]);
      out_at.set_entry(r, c++, stdev_ci.v);
      out_at.set_entry(r, c++, v.min());
      out_at.set_entry(r, c++, v.percentile_array(0.10));
      out_at.set_entry(r, c++, v.percentile_array(0.25));
      out_at.set_entry(r, c++, v.percentile_array(0.50));
      out_at.set_entry(r, c++, v.percentile_array(0.75));
      out_at.set_entry(r, c++, v.percentile_array(0.90));
      out_at.set_entry(r, c++, v.max());
      out_at.set_entry(r, c++, v.sum());
      if(is_bad_data(dsec)) out_at.set_entry(r, c++, dsec);
      else                  out_at.set_entry(r, c++, sec_to_hhmmss(dsec));
      if(is_bad_data(tind)) out_at.set_entry(r, c++, tind);
      else                  out_at.set_entry(r, c++, sec_to_hhmmss(tind));
      out_at.set_entry(r, c++, nint(fsp_total[r-1]));
      out_at.set_entry(r, c++, nint(fsp_best[r-1]));
      out_at.set_entry(r, c++, nint(fsp_ties[r-1]));
      out_at.set_entry(r, c++, fsp);
   }

   // Build a simple output line
   line << "JOB_LIST: " << serialize() << "\n";
   out  << line;

   // Write the table
   out << out_at << "\n" << flush;

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobSummary::compute_fsp(NumArray &total, NumArray &best,
                                   NumArray &ties) {
   map<ConcatString,SummaryMapData,cs_cmp>::iterator it;
   StringArray case_list;
   double v, best_val;
   ConcatString best_mod, s;
   int i, j, k, n;

   // Initialize counts
   total.clear();
   best.clear();
   ties.clear();

   // Loop over the SummaryMap and initialize counts
   for(it=SummaryMap.begin(); it!=SummaryMap.end(); it++) {

      // Initialize counts to zero
      total.add(0);
      best.add(0);
      ties.add(0);
   } // end for it

   // Only compute FSP when AMODEL is in the case information
   if(!CaseColumn.has("AMODEL")) {
      mlog << Debug(4)
           << "Skipping frequency of superior performance since "
           << "the case information does not contain \"AMODEL\".\n";
      return;
   }

   // Compute FSP cases as the intersection of the cases for each entry
   for(it=SummaryMap.begin(); it!=SummaryMap.end(); it++) {

      // Initialize case list to the first entry
      if(it==SummaryMap.begin()) {
         case_list = it->second.Hdr;
      }
      // Take the intersection of the remaining entries
      else {
         case_list = intersection(case_list, it->second.Hdr);
      }
   } // end for it

   mlog << Debug(4)
        << "Computing frequency of superior performance for "
        << Column.n_elements() << " columns and "
        << case_list.n_elements() << " cases.\n";

   // Loop over the columns being summarized
   for(i=0; i<Column.n_elements(); i++) {

      // Check if FSP should be computed for this column
      s = to_upper(Column[i]);
      if(strstr(s, "-") == NULL && strstr(s, "ERR") == NULL) {
         mlog << Debug(4)
              << "Skipping frequency of superior performance for "
              << "column \"" << Column[i] << "\" since it is not an "
              << "error or difference.\n";
         continue;
      }

      // Loop over the cases
      for(j=0; j<case_list.n_elements(); j++) {

         // Initialize top performer
         best_mod = na_str;
         best_val = bad_data_double;

         // Loop over the SummaryMap
         for(it=SummaryMap.begin(); it!=SummaryMap.end(); it++) {

            // Loop over the values for this entry
            for(k=0; k<it->second.Hdr.n_elements(); k++) {

               // Check if entry matches the current case
               if(strncasecmp(Column[i], it->first,
                              strlen(Column[i])) != 0 ||
                  strcmp(case_list[j], it->second.Hdr[k]) != 0)
                  continue;

               // Store the value
               v = it->second.Val[k];

               // Check for bad data
               if(is_bad_data(v)) continue;

               // Check for an meaningful improvment
               if(is_bad_data(best_val) ||
                  FSPThresh.check(fabs(best_val) - fabs(v))) {
                  best_val = v;
                  best_mod = it->second.AModel[k];
               }
               // Check for ties
               else if(fabs(v) <= fabs(best_val)) {
                  best_val = v;
                  best_mod = "TIE";
               }

            } // end for k
         } // end for it

         // Check for no top performer found
         if(is_bad_data(best_val)) {
            mlog << Debug(4)
                 << "For case \"" << Column[i] << ":" << case_list[j]
                 << "\" no superior performance found.\n";
            continue;
         }

         mlog << Debug(4)
              << "For case \"" << Column[i] << ":" << case_list[j]
              << "\" superior performance of " << best_val
              << " by \"" << best_mod << "\".\n";

         // Loop over the SummaryMap to update counts
         for(it=SummaryMap.begin(),n=0; it!=SummaryMap.end(); it++,n++) {

            // Check if entry has the current case
            if(strncasecmp(Column[i], it->first,
                           strlen(Column[i])) != 0 ||
               !it->second.Hdr.has(case_list[j]))
               continue;

            // Update total
            total.set(n, total[n]+1);

            // See if this entry is the top performer
            if(strcmp(best_mod, it->second.AModel[0]) == 0)
               best.set(n, best[n]+1);

            // See if there was a tie
            if(strcmp(best_mod, "TIE") == 0)
               ties.set(n, ties[n]+1);

         } // end for it

      } // end for j
   } // end for i

   return;
}

////////////////////////////////////////////////////////////////////////
//
// Code for misc functions
//
////////////////////////////////////////////////////////////////////////

TCStatJobType string_to_tcstatjobtype(const char *s) {
   TCStatJobType t;

        if(strcasecmp(s, TCStatJobType_FilterStr)  == 0) t = TCStatJobType_Filter;
   else if(strcasecmp(s, TCStatJobType_SummaryStr) == 0) t = TCStatJobType_Summary;
   else if(strcasecmp(s, TCStatJobType_RIRWStr)    == 0) t = TCStatJobType_RIRW;
   else                                                  t = NoTCStatJobType;

   return(t);
}

////////////////////////////////////////////////////////////////////////

ConcatString tcstatjobtype_to_string(const TCStatJobType t) {
   const char *s = (const char *) 0;

   switch(t) {
      case TCStatJobType_Filter:  s = TCStatJobType_FilterStr;  break;
      case TCStatJobType_Summary: s = TCStatJobType_SummaryStr; break;
      case TCStatJobType_RIRW:    s = TCStatJobType_RIRWStr;    break;
      default:                    s = na_str;                   break;
   }

   return(ConcatString(s));
}

////////////////////////////////////////////////////////////////////////
//
// A time-series exists if either the init, valid, or lead time are
// fixed and the other two entries vary by a fixed amount.
//
////////////////////////////////////////////////////////////////////////

bool is_time_series(const TimeArray &init, const NumArray &lead,
                    const TimeArray &valid, int &dsec) {
   int i, dinit, dlead, dvalid;

   // Initialize
   dsec = bad_data_int;

   // The arrays should all be of the same length > 1
   if(init.n_elements() != lead.n_elements() ||
      init.n_elements() != valid.n_elements() ||
      init.n_elements() < 2) {
      mlog << Debug(4)
           << "Skipping time-series computations since the array "
           << "lengths differ.\n";
      return(false);
   }

   // Initialize time spacing
   dinit  = init[1] - init[0];
   dlead  = nint(lead[1] - lead[0]);
   dvalid = valid[1] - valid[0];

   // Loop over the entries to determine the time spacing
   for(i=0; i<init.n_elements()-1; i++) {

      // Make sure the time spacing remains fixed
      if(dinit != (init[i+1] - init[i])) {
         mlog << Debug(4)
              << "Skipping time-series computations since the "
              << "initialization time spacing changed: " << dinit
              << " != " << (init[i+1] - init[i]) << "\n";
         return(false);
      }
      else if(dlead != (lead[i+1] - lead[i])) {
         mlog << Debug(4)
              << "Skipping time-series computations since the "
              << "lead time spacing changed: " << dlead
              << " != " << (lead[i+1] - lead[i]) << "\n";
         return(false);
      }
      else if(dvalid != (valid[i+1] - valid[i])) {
         mlog << Debug(4)
              << "Skipping time-series computations since the "
              << "valid time spacing changed: " << dvalid
              << " != " << (valid[i+1] - valid[i]) << "\n";
         return(false);
      }
   }

   // Check for one fixed and the others varying by a non-zero amount
   if(dinit == 0 && dlead == dvalid && dlead > 0) {
      dsec = dlead;
      mlog << Debug(4)
           << "Computing time-series for initialization time \""
           << unix_to_yyyymmdd_hhmmss(init[0]) << "\" and spacing \""
           << sec_to_hhmmss(dsec) << "\".\n";
   }
   else if(dlead == 0 && dvalid == dinit && dvalid > 0) {
      dsec = dvalid;
      mlog << Debug(4)
           << "Computing time-series for valid time \""
           << unix_to_yyyymmdd_hhmmss(valid[0]) << "\" and spacing \""
           << sec_to_hhmmss(dsec) << "\".\n";
   }
   else if(dvalid == 0 && dinit == dlead && dinit > 0) {
      dsec = dinit;
      mlog << Debug(4)
           << "Computing time-series for lead time \""
           << sec_to_hhmmss(nint(lead[0])) << "\" and spacing \""
           << sec_to_hhmmss(dsec) << "\".\n";
   }

   return(!is_bad_data(dsec));
}

////////////////////////////////////////////////////////////////////////

int compute_time_to_indep(const NumArray &val, int ds) {
   double mean, exp_runs, eff_size, tind;
   int i, n_abv, n_run_abv, n_bel, n_run_bel;
   bool cur_abv, prv_abv;

   // Compute the mean value of the time-series
   mean = val.mean();

   // Initialize
   n_abv = n_run_abv = 0;
   n_bel = n_run_bel = 0;
   prv_abv = false;

   // Count the number of values above and belwo the mean
   for(i=0; i<val.n_elements(); i++) {

      // Store the current state
      cur_abv = (val[i] >= mean);

      // Increment counters
      if(cur_abv) n_abv++;
      else        n_bel++;

      // Count runs above and below
      if(i == 1) {
         if(cur_abv) n_run_abv++;
         else        n_run_bel++;
      }
      else {
              if(cur_abv  && !prv_abv) n_run_abv++;
         else if(!cur_abv &&  prv_abv) n_run_bel++;
      }

      // Store previous state
      prv_abv = cur_abv;

   } // end for i

   // Calculate expected number of runs
   exp_runs = 1.0 + 2.0*(n_abv * n_bel)/(n_abv + n_bel);

   // Calculate effective sample size, time to independence
   eff_size = val.n_elements()*(n_run_abv + n_run_bel)/exp_runs;
   tind     = ds*val.n_elements()/eff_size;

   return(nint(tind));
}

////////////////////////////////////////////////////////////////////////

StringArray intersection(const StringArray &s1, const StringArray &s2) {
   StringArray s;
   int i;

   // Add elements common to both list
   for(i=0; i<s1.n_elements(); i++) {
      if(s2.has(s1[i])) s.add(s1[i]);
   }

   return(s);
}

////////////////////////////////////////////////////////////////////////

void parse_thresh_option(const char *col_name, const char *col_val,
                         map<ConcatString,ThreshArray> &m) {
   ConcatString cs = to_upper(col_name);
   ThreshArray ta;
   ta.add_css(col_val);
   
   // Add data to the map
   if(m.count(cs) > 0) m[col_name].add(ta);
   else                m.insert(pair<ConcatString, ThreshArray>(cs, ta));

   return;
}

////////////////////////////////////////////////////////////////////////

void parse_string_option(const char *col_name, const char *col_val,
                         map<ConcatString,StringArray> &m) {
   ConcatString cs = to_upper(col_name);
   StringArray sa;
   sa.set_ignore_case(1);
   sa.add_css(col_val);
   
   // Add data to the map
   if(m.count(cs) > 0) m[col_name].add(sa);
   else                m.insert(pair<ConcatString, StringArray>(cs, sa));

   return;
}

////////////////////////////////////////////////////////////////////////
//
// Code for class TCStatJobRIRW
//
////////////////////////////////////////////////////////////////////////

TCStatJobRIRW::TCStatJobRIRW() {

   init_from_scratch();
}

////////////////////////////////////////////////////////////////////////

TCStatJobRIRW::~TCStatJobRIRW() {

   clear();
}

////////////////////////////////////////////////////////////////////////

TCStatJobRIRW::TCStatJobRIRW(const TCStatJobRIRW &j) {

   init_from_scratch();

   assign(j);
}

////////////////////////////////////////////////////////////////////////

TCStatJobRIRW & TCStatJobRIRW::operator=(const TCStatJobRIRW &j) {

   if(this == &j) return(*this);

   assign(j);

   return(*this);
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::init_from_scratch() {
   int i;
   
   for(i=0; i<4; i++) DumpOutCTC[i] = (ofstream *) 0;

   TCStatJob::init_from_scratch();

   // Ignore case when performing comparisons
   CaseColumn.set_ignore_case(1);

   clear();

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::clear() {
   int i;

   TCStatJob::clear();

   JobType = TCStatJobType_RIRW;
   
   // Disable rapid intensification/weakening filtering logic.
   RapidIntenTrack = TrackType_None;

   CaseColumn.clear();
   RIRWMap.clear();

   for(i=0; i<4; i++) DumpFileCTC[i].clear();
   close_dump_file();

   // Set to default value
   OutAlpha = default_tc_alpha;
   
   // Set to default values
   OutLineType.clear();
   OutLineType.add("CTC");
   OutLineType.add("CTS");

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::assign(const TCStatJobRIRW & j) {

   TCStatJob::assign(j);

   TCStatJob::assign(j);

   CaseColumn = j.CaseColumn;
   RIRWMap = j.RIRWMap;
   OutAlpha = j.OutAlpha;

   return;
}

////////////////////////////////////////////////////////////////////////

StringArray TCStatJobRIRW::parse_job_command(const char *jobstring) {
   StringArray a, b;
   const char * c = (const char *) 0;
   int i;

   // Call the parent and store any unused options
   a = TCStatJob::parse_job_command(jobstring);

   // Clear the OutLineType if the user has specified it
   if(a.has("-out_line_type")) OutLineType.clear();
   
   // Loop over the StringArray elements
   for(i=0; i<a.n_elements(); i++) {

      // Point at the current entry
      c = a[i];

      // Check for a job command option
      if(c[0] != '-') {
         b.add(a[i]);
         continue;
      }

      // Check job command options
           if(strcasecmp(c, "-by"            ) == 0) { CaseColumn.add_css(to_upper(a[i+1]));  a.shift_down(i, 1); }
      else if(strcasecmp(c, "-out_alpha"     ) == 0) { OutAlpha = atof(a[i+1]);               a.shift_down(i, 1); }
      else if(strcasecmp(c, "-out_line_type" ) == 0) { OutLineType.add_css(to_upper(a[i+1])); a.shift_down(i, 1); }
      else                                           {                                        b.add(a[i]);        }
   }

   return(b);
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::open_dump_file() {
   const char *category[] = { "FY_OY", "FY_ON", "FN_OY", "FN_ON" };
   int i;

   close_dump_file();

   if(DumpFile.empty()) return;

   // Use DumpFile entry to construct output file names for each category
   for(i=0; i<4; i++) {

      DumpFileCTC[i] << cs_erase << DumpFile << "_"  << category[i] << ".tcst";
      DumpOutCTC[i] = new ofstream;
      DumpOutCTC[i]->open(DumpFileCTC[i]);

      if(!DumpOutCTC[i]) {
         mlog << Error << "\nTCStatJob::open_dump_file()-> "
              << "can't open the output file \"" << DumpFileCTC[i]
              << "\" for writing!\n\n";
         exit(1);
      }
   }

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::close_dump_file() {
   int i;

   for(i=0; i<4; i++) {
      if(DumpOutCTC[i]) {
         mlog << Debug(1)
              << "Creating output dump file: " << DumpFileCTC[i] << "\n";

         DumpOutCTC[i]->close();
         delete DumpOutCTC[i];
         DumpOutCTC[i] = (ofstream *) 0;
      }
   }

   return;
}

////////////////////////////////////////////////////////////////////////

ConcatString TCStatJobRIRW::serialize() const {
   ConcatString s;
   int i;

   // Initialize the jobstring
   s.set_precision(default_precision);

   // Call the parent
   s = TCStatJob::serialize();

   // List the ADeck and BDeck rapid intensification logic
   if(RapidIntenTimeADeck == RapidIntenTimeBDeck) {
      s << "-rapid_inten_time " << sec_to_hhmmss(RapidIntenTimeADeck) << " ";
   }
   else {
      s << "-rapid_inten_time_adeck " << sec_to_hhmmss(RapidIntenTimeADeck) << " "
        << "-rapid_inten_time_bdeck " << sec_to_hhmmss(RapidIntenTimeBDeck) << " ";
   }
   if(RapidIntenExactADeck == RapidIntenExactBDeck) {
      s << "-rapid_inten_exact " << bool_to_string(RapidIntenExactADeck) << " ";
   }
   else {
      s << "-rapid_inten_exact_adeck " << bool_to_string(RapidIntenExactADeck) << " "
        << "-rapid_inten_exact_bdeck " << bool_to_string(RapidIntenExactBDeck) << " ";
   }
   if(RapidIntenThreshADeck == RapidIntenThreshBDeck) {
      s << "-rapid_inten_thresh " << RapidIntenThreshADeck.get_str() << " ";
   }
   else {
      s << "-rapid_inten_thresh_adeck " << RapidIntenThreshADeck.get_str() << " "
        << "-rapid_inten_thresh_bdeck " << RapidIntenThreshBDeck.get_str() << " ";
   }
   s << "-rapid_inten_window " << sec_to_hhmmss(RapidIntenWindow) << " ";

   // Add RIRW job-specific options
   for(i=0; i<CaseColumn.n_elements(); i++)
      s << "-by " << CaseColumn[i] << " ";
   
   for(i=0; i<OutLineType.n_elements(); i++)
      s << "-out_line_type " << OutLineType[i] << " ";

   // Always list the output alpha value used
   s << "-out_alpha " << OutAlpha << " ";

   return(s);
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::do_job(const StringArray &file_list,
                           TCLineCounts &n) {
   TrackPairInfo tpi;

   // Call the parent's do_job() to do event equalization
   TCStatJob::do_job(file_list, n);
   
   // Check for no common cases
   if(EventEqualSet == true && EventEqualCases.n_elements() == 0) {
      mlog << Debug(1)
           << "Event equalization found no common cases.\n";
   }
   // Otherwise, process the track data
   else {

      // Rewind to the beginning of the track pair input
      TCSTFiles.rewind();

      // Process each of the track pairs
      while(TCSTFiles >> tpi) {

         // Process the track pair down to points to be used
         subset_track_pair(tpi, n);

         if(tpi.n_points() > 0) {

            mlog << Debug(4)
                 << "Processing track pair: " << tpi.case_info() << "\n";

            // Process the track pair info for the RI/RW job
            process_track_pair(tpi);
         }   
      } // end while
   } // end else
   
   // Close the dump file
   close_dump_file();

   // Process the RI/RW job output
   if(JobOut) do_output(*JobOut);
   else       do_output(cout);

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::process_track_pair(TrackPairInfo &tpi) {
   int i, j, a, b, cur_dt, min_dt, i_min_dt, lead;
   map<ConcatString,RIRWMapData,cs_cmp> cur_map;
   ConcatString key, cur, cat;
   RIRWMapData data;
   TrackPairInfo tpi_pt;
   int acur, aprv, adlt;
   int bcur, bprv, bdlt;
   const char *sep = ":";

   // Initialize the map
   cur_map.clear();
   
   // Apply the rapid intensification/weakening logic
   tpi.check_rapid_inten(TrackType_Both,
                         RapidIntenTimeADeck, RapidIntenTimeBDeck, 
                         RapidIntenExactADeck, RapidIntenExactBDeck,
                         RapidIntenThreshADeck, RapidIntenThreshBDeck);

   // Loop over the track points and populate the contigency table
   for(i=0; i<tpi.n_points(); i++) {

      // Initialize the map key so it's never empty
      key = "RIRW";

      // Build case information for the map key
      for(j=0; j<CaseColumn.n_elements(); j++) {

         cur = tpi.line(i)->get_item(CaseColumn[j]);

         // For bad data, use the NA string
         if(is_bad_data(atoi(cur))) cur = na_str;

         // Special handling for lead time:
         // Switch 2-digit hours to 3-digit hours so that the
         // summary job output is sorted nicely.
         if(strcasecmp(CaseColumn[j], "LEAD") == 0 &&
            cur != na_str &&
            abs(lead = hhmmss_to_sec(cur)) < 100*sec_per_hour) {

            // Handle positive and negative lead times
            key << (lead < 0 ? ":-0" : ":0")
                << sec_to_hhmmss(abs(lead));
         }
         // Otherwise, just append the current case info
         else {
            key << ":" << cur;
         }
      } // end for j

      // Add map entry for this key, if necessary
      if(cur_map.count(key) == 0) cur_map[key] = data;

      // Get the ADeck and BDeck events
      a = nint(tpi.adeck_rirw(i));
      b = nint(tpi.bdeck_rirw(i));

      // Store current info
      lead = tpi.adeck()[i].lead();
      acur = tpi.adeck()[i].v_max();
      aprv = tpi.adeck_prv_int(i);
      adlt = (is_bad_data(acur) || is_bad_data(aprv) ? bad_data_int : acur - aprv);
      bcur = tpi.bdeck()[i].v_max();
      bprv = tpi.bdeck_prv_int(i);
      bdlt = (is_bad_data(bcur) || is_bad_data(bprv) ? bad_data_int : bcur - bprv);
      
      // Check for small enough timing error to consider a false alarm a hit
      if(a == 1 && b == 0) {

         // Loop through the BDECK track points
         for(j=0, min_dt=bad_data_int, i_min_dt=bad_data_int; j<tpi.n_points(); j++) {

            // Skip non-RI/RW events
            if(nint(tpi.bdeck_rirw(j)) != 1) continue;

            // Find closet RI/RW in time
            cur_dt = abs(tpi.valid(i) - tpi.valid(j));
            if(is_bad_data(min_dt)) {
               min_dt   = cur_dt;
               i_min_dt = j;
            }
            else if(cur_dt < min_dt) {
               min_dt   = cur_dt;
               i_min_dt = j;
            }
         } // end for j

         // Check if the BDeck RI/RW event is close enough in time
         if(!is_bad_data(min_dt) && min_dt <= RapidIntenWindow) {
            mlog << Debug(4)
                 << "Switching FY_ON to FY_OY since BDECK RI/RW ("
                 << tpi.bdeck_prv_int(i_min_dt) << " to "
                 << tpi.bdeck()[i_min_dt].v_max() << ") occurred at "
                 << unix_to_yyyymmdd_hhmmss(tpi.valid(i_min_dt)) << " ("
                 << sec_to_hhmmss(min_dt) << " offset <= "
                 << sec_to_hhmmss(RapidIntenWindow)
                 << " window) for " << tpi.case_info() << ", "
                 << "VALID = " << unix_to_yyyymmdd_hhmmss(tpi.valid(i)) << ", "
                 << "LEAD = " << (is_bad_data(lead) ? na_str : sec_to_hhmmss(lead)) << "\n";
            b = 1;
            bcur = tpi.bdeck()[i_min_dt].v_max();
            bprv = tpi.bdeck_prv_int(i_min_dt);
            bdlt = bcur - bprv;
         }
      }

      // Determine the contingency table category      
      if(is_bad_data(a) || is_bad_data(b)) {
         cat = na_str;
      }
      else {

         cat << cs_erase
             << "F"  << (a == 1 ? "Y" : "N")
             << "_O" << (b == 1 ? "Y" : "N");

              if(a == 1 && b == 1) cur_map[key].Info.cts.inc_fy_oy();
         else if(a == 1 && b == 0) cur_map[key].Info.cts.inc_fy_on();
         else if(a == 0 && b == 1) cur_map[key].Info.cts.inc_fn_oy();
         else if(a == 0 && b == 0) cur_map[key].Info.cts.inc_fn_on();

         // Write the current point to the correct dump file
         if(DumpOutCTC[0]) {
            
            // Create a track for the current point
            tpi_pt.clear();
            tpi_pt.add(*tpi.line(i));

                 if(a == 1 && b == 1) dump_track_pair(tpi_pt, DumpOutCTC[0]);
            else if(a == 1 && b == 0) dump_track_pair(tpi_pt, DumpOutCTC[1]);
            else if(a == 0 && b == 1) dump_track_pair(tpi_pt, DumpOutCTC[2]);
            else if(a == 0 && b == 0) dump_track_pair(tpi_pt, DumpOutCTC[3]);
         }
      }

      // Add values for this key:
      //    AMODEL        BMODEL        STORM_ID
      //    INIT          LEAD          VALID
      //    AMAX_WIND_PRV AMAX_WIND_CUR AMAX_WIND_DLT ARIRW
      //    BMAX_WIND_PRV BMAX_WIND_CUR BMAX_WIND_DLT BRIRW
      //    CATEGORY
      cur << cs_erase
          << tpi.line(i)->amodel() << sep
          << tpi.line(i)->bmodel() << sep
          << tpi.line(i)->storm_id() << sep
          << unix_to_yyyymmdd_hhmmss(tpi.line(i)->init()) << sep
          << (is_bad_data(lead) ? na_str : sec_to_hhmmss(lead)) << sep
          << unix_to_yyyymmdd_hhmmss(tpi.line(i)->valid()) << sep
          << aprv << sep << acur << sep << adlt << sep
          << (is_bad_data(a) ? na_str : bool_to_string(a)) << sep
          << bprv << sep << bcur << sep << bdlt << sep
          << (is_bad_data(b) ? na_str : bool_to_string(b)) << sep
          << cat;
      cur_map[key].Hdr.add(cur);
   } // end for i

   // Add the current map
   add_map(cur_map);

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::add_map(map<ConcatString,RIRWMapData,cs_cmp>&m) {
   map<ConcatString,RIRWMapData,cs_cmp>::iterator it;

   // Loop over the input map entries
   for(it = m.begin(); it != m.end(); it++) {

      // Current map key is not yet defined in RIRWMap
      if(RIRWMap.count(it->first) == 0) {

         mlog << Debug(5)
              << "RIRW Map Insert (" << it->first << ") "
              << it->second.Info.cts.total() << " contingency table entries: "
              << it->second.Info.cts.fy_oy() << " Hits, "
              << it->second.Info.cts.fy_on() << " False Alarms, "
              << it->second.Info.cts.fn_oy() << " Misses, "
              << it->second.Info.cts.fn_on() << " Correct Negatives.\n";

         // Add the pair to the map
         RIRWMap[it->first] = it->second;
      }
      // Current map key is defined in RIRWMap
      else {

         mlog << Debug(5)
              << "RIRW Map Add (" << it->first << ") "
              << it->second.Info.cts.total() << " contingency table entries: "
              << it->second.Info.cts.fy_oy() << " hits, "
              << it->second.Info.cts.fy_on() << " false alarms, "
              << it->second.Info.cts.fn_oy() << " misses, "
              << it->second.Info.cts.fn_on() << " correct negatives.\n";

         // Increment the counts for the existing key
         RIRWMap[it->first].Info.cts.set_fy_oy(
            RIRWMap[it->first].Info.cts.fy_oy() +
            it->second.Info.cts.fy_oy());
         RIRWMap[it->first].Info.cts.set_fy_on(
            RIRWMap[it->first].Info.cts.fy_on() +
            it->second.Info.cts.fy_on());
         RIRWMap[it->first].Info.cts.set_fn_oy(
            RIRWMap[it->first].Info.cts.fn_oy() +
            it->second.Info.cts.fn_oy());
         RIRWMap[it->first].Info.cts.set_fn_on(
            RIRWMap[it->first].Info.cts.fn_on() +
            it->second.Info.cts.fn_on());
         RIRWMap[it->first].Hdr.add(it->second.Hdr);
      }
   } // end for it

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::do_output(ostream &out) {
   ConcatString line;
 
   // Build a simple output line
   line << "JOB_LIST: " << serialize() << "\n";
   out  << line;
   
   if(OutLineType.has("CTC")) do_ctc_output(out);
   if(OutLineType.has("CTS")) do_cts_output(out);
   if(OutLineType.has("MPR")) do_mpr_output(out);
   
   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::do_ctc_output(ostream &out) {
   map<ConcatString,RIRWMapData,cs_cmp>::iterator it;
   StringArray sa;
   int i, r, c;
   AsciiTable out_at; 

   // Format the output table
   out_at.set_size((int) RIRWMap.size() + 1,
                   8 + CaseColumn.n_elements() + n_ctc_columns);   
   setup_table(out_at, 8 + CaseColumn.n_elements());
   
   // Initialize row and column indices
   r = c = 0;

   // Write the header row
   out_at.set_entry(r, c++, "COL_NAME:");
   
   // Write the RI/RW event definition headers
   out_at.set_entry(r, c++, "ATIME");
   out_at.set_entry(r, c++, "BTIME");
   out_at.set_entry(r, c++, "AEXACT");
   out_at.set_entry(r, c++, "BEXACT");
   out_at.set_entry(r, c++, "ATHRESH");
   out_at.set_entry(r, c++, "BTHRESH");
   out_at.set_entry(r, c++, "WINDOW");
  
   // Write case column names
   for(i=0; i<CaseColumn.n_elements(); i++) {
      out_at.set_entry(r, c++, CaseColumn[i]);
   }

   // Write the header columns
   write_header_row(ctc_columns, n_ctc_columns, 0, out_at, r, c);
   
   // Loop over the map entries and popluate the output table
   for(it=RIRWMap.begin(),r=1; it!=RIRWMap.end(); it++,r++) {

      // Split the current map key
      sa = it->first.split(":");

      // Initialize column counter
      c = 0;

      // Write the table row
      out_at.set_entry(r, c++, "RIRW_CTC:");

      // Write the RI/RW event definition
      out_at.set_entry(r, c++, sec_to_hhmmss(RapidIntenTimeADeck));
      out_at.set_entry(r, c++, sec_to_hhmmss(RapidIntenTimeBDeck));
      out_at.set_entry(r, c++, bool_to_string(RapidIntenExactADeck));
      out_at.set_entry(r, c++, bool_to_string(RapidIntenExactBDeck));
      out_at.set_entry(r, c++, RapidIntenThreshADeck.get_str());
      out_at.set_entry(r, c++, RapidIntenThreshBDeck.get_str());
      out_at.set_entry(r, c++, sec_to_hhmmss(RapidIntenWindow));
      
      // Write case column values
      for(i=1; i<sa.n_elements(); i++) {
         out_at.set_entry(r, c++, sa[i]);
      }

      // Write the contingency table counts
      write_ctc_cols(it->second.Info, out_at, r, c);
   }

   // Write the table
   out << out_at << "\n" << flush;

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::do_cts_output(ostream &out) {
   map<ConcatString,RIRWMapData,cs_cmp>::iterator it;
   StringArray sa;
   int i, r, c;
   AsciiTable out_at; 

   // Format the output table
   out_at.set_size((int) RIRWMap.size() + 1,
                   8 + CaseColumn.n_elements() + n_cts_columns);   
   setup_table(out_at, 8 + CaseColumn.n_elements());
   
   // Initialize row and column indices
   r = c = 0;

   // Write the header row
   out_at.set_entry(r, c++, "COL_NAME:");
   
   // Write the RI/RW event definition headers
   out_at.set_entry(r, c++, "ATIME");
   out_at.set_entry(r, c++, "BTIME");
   out_at.set_entry(r, c++, "AEXACT");
   out_at.set_entry(r, c++, "BEXACT");
   out_at.set_entry(r, c++, "ATHRESH");
   out_at.set_entry(r, c++, "BTHRESH");
   out_at.set_entry(r, c++, "WINDOW");
  
   // Write case column names
   for(i=0; i<CaseColumn.n_elements(); i++) {
      out_at.set_entry(r, c++, CaseColumn[i]);
   }

   // Write the header columns
   write_header_row(cts_columns, n_cts_columns, 0, out_at, r, c);
   
   // Loop over the map entries and popluate the output table
   for(it=RIRWMap.begin(),r=1; it!=RIRWMap.end(); it++,r++) {

      // Allocate a single alpha value and compute statistics
      it->second.Info.allocate_n_alpha(1);
      it->second.Info.alpha[0] = OutAlpha;

      // Compute the stats and confidence intervals
      it->second.Info.compute_stats();
      it->second.Info.compute_ci();

      // Split the current map key
      sa = it->first.split(":");

      // Initialize column counter
      c = 0;

      // Write the table row
      out_at.set_entry(r, c++, "RIRW_CTS:");

      // Write the RI/RW event definition
      out_at.set_entry(r, c++, sec_to_hhmmss(RapidIntenTimeADeck));
      out_at.set_entry(r, c++, sec_to_hhmmss(RapidIntenTimeBDeck));
      out_at.set_entry(r, c++, bool_to_string(RapidIntenExactADeck));
      out_at.set_entry(r, c++, bool_to_string(RapidIntenExactBDeck));
      out_at.set_entry(r, c++, RapidIntenThreshADeck.get_str());
      out_at.set_entry(r, c++, RapidIntenThreshBDeck.get_str());
      out_at.set_entry(r, c++, sec_to_hhmmss(RapidIntenWindow));
      
      // Write case column values
      for(i=1; i<sa.n_elements(); i++) {
         out_at.set_entry(r, c++, sa[i]);
      }

      // Write the contingency table statistics
      write_cts_cols(it->second.Info, 0, out_at, r, c);
   }

   // Write the table
   out << out_at << "\n" << flush;

   return;
}

////////////////////////////////////////////////////////////////////////

void TCStatJobRIRW::do_mpr_output(ostream &out) {
   map<ConcatString,RIRWMapData,cs_cmp>::iterator it;
   StringArray sa;
   int i, j, r, c;
   AsciiTable out_at; 
   ConcatString cs;

   // Determine the required number of rows
   for(it=RIRWMap.begin(),r=0; it!=RIRWMap.end(); it++) {
      r += it->second.Hdr.n_elements();
   }

   // Format the output table
   out_at.set_size(r + 1,
                   8 + CaseColumn.n_elements() + 15);
   setup_table(out_at, 8 + CaseColumn.n_elements());

   // Initialize row and column indices
   r = c = 0;

   // Write the header row
   out_at.set_entry(r, c++, "COL_NAME:");
   
   // Write the RI/RW event definition headers
   out_at.set_entry(r, c++, "ATIME");
   out_at.set_entry(r, c++, "BTIME");
   out_at.set_entry(r, c++, "AEXACT");
   out_at.set_entry(r, c++, "BEXACT");
   out_at.set_entry(r, c++, "ATHRESH");
   out_at.set_entry(r, c++, "BTHRESH");
   out_at.set_entry(r, c++, "WINDOW");
  
   // Write case column names
   for(i=0; i<CaseColumn.n_elements(); i++) {
      out_at.set_entry(r, c++, CaseColumn[i]);
   }

   // Write the header columns
   out_at.set_entry(r, c++, "AMODEL");
   out_at.set_entry(r, c++, "BMODEL");
   out_at.set_entry(r, c++, "STORM_ID");
   out_at.set_entry(r, c++, "INIT");
   out_at.set_entry(r, c++, "LEAD");
   out_at.set_entry(r, c++, "VALID");
   out_at.set_entry(r, c++, "AMAX_WIND_PRV");
   out_at.set_entry(r, c++, "AMAX_WIND_CUR");
   out_at.set_entry(r, c++, "AMAX_WIND_DLT");
   out_at.set_entry(r, c++, "ARIRW");
   out_at.set_entry(r, c++, "BMAX_WIND_PRV");
   out_at.set_entry(r, c++, "BMAX_WIND_CUR");
   out_at.set_entry(r, c++, "BMAX_WIND_DLT");
   out_at.set_entry(r, c++, "BRIRW");
   out_at.set_entry(r, c++, "CATEGORY");
   
   // Loop over the map entries and popluate the output table
   for(it=RIRWMap.begin(),r=1; it!=RIRWMap.end(); it++) {

      // Loop through the current points
      for(i=0; i<it->second.Hdr.n_elements(); i++,r++) {

         // Initialize column counter
         c = 0;

         // Write the table row
         out_at.set_entry(r, c++, "RIRW_MPR:");

         // Write the RI/RW event definition
         out_at.set_entry(r, c++, sec_to_hhmmss(RapidIntenTimeADeck));
         out_at.set_entry(r, c++, sec_to_hhmmss(RapidIntenTimeBDeck));
         out_at.set_entry(r, c++, bool_to_string(RapidIntenExactADeck));
         out_at.set_entry(r, c++, bool_to_string(RapidIntenExactBDeck));
         out_at.set_entry(r, c++, RapidIntenThreshADeck.get_str());
         out_at.set_entry(r, c++, RapidIntenThreshBDeck.get_str());
         out_at.set_entry(r, c++, sec_to_hhmmss(RapidIntenWindow));
         
         // Write case column values
         sa = it->first.split(":");
         for(j=1; j<sa.n_elements(); j++) {
            out_at.set_entry(r, c++, sa[j]);
         }

         // Write the header information
         cs = it->second.Hdr[i];
         sa = cs.split(":");
         for(j=0; j<sa.n_elements(); j++) {
            out_at.set_entry(r, c++, sa[j]);
         }
      }
   }

   // Write the table
   out << out_at << "\n" << flush;

   return;  
}

////////////////////////////////////////////////////////////////////////

void setup_table(AsciiTable &at, int n_hdr_cols) {
   int i;

   // Left-justify header columns and right-justify data columns
   for(i=0;          i<n_hdr_cols; i++) at.set_column_just(i, LeftJust);
   for(i=n_hdr_cols; i<at.ncols(); i++) at.set_column_just(i, RightJust);

   // Right-justify the first column
   at.set_column_just(0, RightJust);

   // Set the precision
   at.set_precision(default_precision);

   // Set the bad data value
   at.set_bad_data_value(bad_data_double);

   // Set the bad data string
   at.set_bad_data_str(na_str);

   // Don't write out trailing blank rows
   at.set_delete_trailing_blank_rows(1);

   return;
}

////////////////////////////////////////////////////////////////////////
