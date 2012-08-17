// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2012
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

////////////////////////////////////////////////////////////////////////

#ifndef  __VX_TRACK_POINT_H__
#define  __VX_TRACK_POINT_H__

////////////////////////////////////////////////////////////////////////

#include <iostream>

#include "vx_cal.h"
#include "vx_util.h"

#include "atcf_line.h"

////////////////////////////////////////////////////////////////////////

// Define the wind intensity levels to be handled
static const int WindIntensity[] = { 34, 50, 64 };
static const int NWinds = sizeof(WindIntensity)/sizeof(*WindIntensity);

////////////////////////////////////////////////////////////////////////

class QuadInfo {
   private:

      void init_from_scratch();

      void assign(const QuadInfo &);

      int    Intensity;
      double ALVal;
      double NEVal;
      double SEVal;
      double SWVal;
      double NWVal;

   public:

      QuadInfo();
     ~QuadInfo();
      QuadInfo(const QuadInfo &);
      QuadInfo & operator=(const QuadInfo &);
      QuadInfo & operator+=(const QuadInfo &);
      bool       operator==(const QuadInfo &) const;

      void clear();

      void         dump(ostream &, int = 0)  const;
      ConcatString serialize()               const;
      ConcatString serialize_r(int, int = 0) const;

         //
         //  set stuff
         //

      void set_wind(const ATCFLine &);
      void set_seas(const ATCFLine &);
      void set_quad_vals(QuadrantType, int, int, int, int);

      void set_intensity(int);
      void set_al_val(double);
      void set_ne_val(double);
      void set_se_val(double);
      void set_sw_val(double);
      void set_nw_val(double);

         //
         //  get stuff
         //

      int    intensity() const;
      double al_val()    const;
      double ne_val()    const;
      double se_val()    const;
      double sw_val()    const;
      double nw_val()    const;

         //
         //  do stuff
         //

      bool has_wind(const ATCFLine &)      const;
      bool has_seas(const ATCFLine &)      const;

      bool is_match_wind(const ATCFLine &) const;
      bool is_match_seas(const ATCFLine &) const;

};

////////////////////////////////////////////////////////////////////////

inline void QuadInfo::set_intensity(int i) { Intensity = i; }
inline void QuadInfo::set_al_val(double v) { ALVal = v;     }
inline void QuadInfo::set_ne_val(double v) { NEVal = v;     }
inline void QuadInfo::set_se_val(double v) { SEVal = v;     }
inline void QuadInfo::set_sw_val(double v) { SWVal = v;     }
inline void QuadInfo::set_nw_val(double v) { NWVal = v;     }

inline int    QuadInfo::intensity() const { return(Intensity); }
inline double QuadInfo::al_val()    const { return(ALVal);     }
inline double QuadInfo::ne_val()    const { return(NEVal);     }
inline double QuadInfo::se_val()    const { return(SEVal);     }
inline double QuadInfo::sw_val()    const { return(SWVal);     }
inline double QuadInfo::nw_val()    const { return(NWVal);     }

////////////////////////////////////////////////////////////////////////
//
// TrackPoint class stores the data for a single (lat,lon) track point.
//
////////////////////////////////////////////////////////////////////////

class TrackPoint {

   private:

      void init_from_scratch();
      void assign(const TrackPoint &);

      bool          IsSet;
      
      // Timing information
      unixtime      ValidTime;
      int           LeadTime;   //  seconds
 
      // Location
      double        Lat;        //  degrees, + north, - south
      double        Lon;        //  degrees, + west, - east

      // Intensity
      int           Vmax;       //  knots
      int           MSLP;       //  millibars
      CycloneLevel  Level;

      // Speed and direction
      double       Speed;
      double       Direction;

      // Watch/Warning status
      WatchWarnType WatchWarn; 

      // Wind Radii
      QuadInfo      Wind[NWinds];

   public:

      TrackPoint();
     ~TrackPoint();
      TrackPoint(const TrackPoint &);
      TrackPoint & operator=(const TrackPoint &);
      TrackPoint & operator+=(const TrackPoint &);      

      void clear();

      void         dump(ostream &, int = 0)  const;
      ConcatString serialize()               const;
      ConcatString serialize_r(int, int = 0) const;

         //
         //  set stuff
         //

      void initialize(const ATCFLine &);

      void set_valid(const unixtime);
      void set_lead(const int);
      void set_lat(const double);
      void set_lon(const double);
      void set_v_max(const int);
      void set_mslp(const int);
      void set_level(CycloneLevel);
      void set_speed(const double);
      void set_direction(const double);
      void set_watch_warn(WatchWarnType);
      void set_watch_warn(WatchWarnType, unixtime);

         //
         //  get stuff
         //
         
      const QuadInfo & operator[](int) const;
      unixtime         valid()         const;
      int              lead()          const;
      double           lat()           const;
      double           lon()           const;
      int              v_max()         const;
      int              mslp()          const;
      CycloneLevel     level()         const;
      double           speed()         const;
      double           direction()     const;
      WatchWarnType    watch_warn()    const;

         //
         //  do stuff
         //

      void set_wind(int, const QuadInfo &);
      bool set(const ATCFLine &);
      bool has(const ATCFLine &) const;
      bool is_match(const ATCFLine &) const;

};

////////////////////////////////////////////////////////////////////////

inline void TrackPoint::set_valid(const unixtime u)    { ValidTime = u; }
inline void TrackPoint::set_lead(const int s)          { LeadTime = s;  }
inline void TrackPoint::set_lat(const double l)        { Lat = l;       }
inline void TrackPoint::set_lon(const double l)        { Lon = l;       }
inline void TrackPoint::set_v_max(const int v)         { Vmax = v;      }
inline void TrackPoint::set_mslp(const int v)          { MSLP = v;      }
inline void TrackPoint::set_level(CycloneLevel l)      { Level = l;     }
inline void TrackPoint::set_speed(const double s)      { Speed = s;     }
inline void TrackPoint::set_direction(const double d)  { Direction = d; }
inline void TrackPoint::set_watch_warn(WatchWarnType t){ WatchWarn = t; }

inline unixtime      TrackPoint::valid()      const { return(ValidTime); }
inline int           TrackPoint::lead()       const { return(LeadTime);  }
inline double        TrackPoint::lat()        const { return(Lat);       }
inline double        TrackPoint::lon()        const { return(Lon);       }
inline int           TrackPoint::v_max()      const { return(Vmax);      }
inline int           TrackPoint::mslp()       const { return(MSLP);      }
inline CycloneLevel  TrackPoint::level()      const { return(Level);     }
inline double        TrackPoint::speed()      const { return(Speed);     }
inline double        TrackPoint::direction()  const { return(Direction); }
inline WatchWarnType TrackPoint::watch_warn() const { return(WatchWarn); }

////////////////////////////////////////////////////////////////////////

#endif   /*  __VX_TRACK_POINT_H__  */

////////////////////////////////////////////////////////////////////////
