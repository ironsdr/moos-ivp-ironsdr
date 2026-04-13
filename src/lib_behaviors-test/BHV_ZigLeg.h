/************************************************************/
/*    NAME: David Irons                                     */
/*    ORGN: MIT                                             */
/*    FILE: BHV_ZigLeg.h                                    */
/*    DATE: 13 APR 2026                                     */
/************************************************************/

#ifndef ZigLeg_HEADER
#define ZigLeg_HEADER

#include <string>
#include "IvPBehavior.h"

class BHV_ZigLeg : public IvPBehavior {
public:
  BHV_ZigLeg(IvPDomain);
  ~BHV_ZigLeg() {};

  bool         setParam(std::string, std::string);
  void         onSetParamComplete();
  void         onCompleteState();
  void         onIdleState();
  void         onHelmStart();
  void         postConfigStatus();
  void         onRunToIdleState();
  void         onIdleToRunState();
  IvPFunction* onRunState();

protected: // Local Utility functions

protected: // Configuration parameters
  double m_zig_angle;
  double m_zig_duration;

protected: // State variables
  double m_osx;
  double m_osy;
  double m_curr_heading;
  double m_current_time;
  int    m_wpt_index;
  double m_wpt_change_time;
  bool   m_zig_pending;
  bool   m_zig_active;
  double m_zig_start_time;
  double m_zig_heading;
};

#define IVP_EXPORT_FUNCTION

extern "C" {
  IVP_EXPORT_FUNCTION IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_ZigLeg(domain);}
}
#endif
