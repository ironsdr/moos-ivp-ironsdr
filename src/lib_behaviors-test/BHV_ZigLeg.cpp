/************************************************************/
/*    NAME: David Irons                                     */
/*    ORGN: MIT                                             */
/*    FILE: BHV_ZigLeg.cpp                                  */
/*    DATE: 13 APR 2026                                     */
/************************************************************/

#include "BHV_ZigLeg.h"
#include "BuildUtils.h"
#include "MBUtils.h"
#include "ZAIC_PEAK.h"
#include <cmath>
#include <cstdlib>

using namespace std;

//---------------------------------------------------------------
// Constructor

BHV_ZigLeg::BHV_ZigLeg(IvPDomain domain) : IvPBehavior(domain) {
  IvPBehavior::setParam("name", "defaultname");

  m_domain = subDomain(m_domain, "course,speed");

  addInfoVars("NAV_X, NAV_Y, NAV_HEADING");
  addInfoVars("WPT_INDEX", "no_warning");

  m_zig_angle = 45;
  m_zig_duration = 10;

  m_osx = 0;
  m_osy = 0;
  m_curr_heading = 0;
  m_current_time = 0;
  m_wpt_index = 0;
  m_wpt_change_time = -1;
  m_zig_pending = false;
  m_zig_active = false;
  m_zig_start_time = 0;
  m_zig_heading = 0;
}

//---------------------------------------------------------------
// Procedure: setParam()

bool BHV_ZigLeg::setParam(string param, string val) {
  param = tolower(param);
  double double_val = atof(val.c_str());

  if ((param == "zig_angle") && isNumber(val)) {
    m_zig_angle = double_val;
    return (true);
  } else if ((param == "zig_duration") && isNumber(val)) {
    m_zig_duration = double_val;
    return (true);
  }

  return (false);
}

//---------------------------------------------------------------
void BHV_ZigLeg::onSetParamComplete() {}
void BHV_ZigLeg::onHelmStart() {}
void BHV_ZigLeg::onIdleState() {}
void BHV_ZigLeg::onCompleteState() {}
void BHV_ZigLeg::postConfigStatus() {}
void BHV_ZigLeg::onIdleToRunState() {}
void BHV_ZigLeg::onRunToIdleState() {}

//---------------------------------------------------------------
// Procedure: onRunState()

IvPFunction *BHV_ZigLeg::onRunState() {
  bool ok1, ok2, ok3, ok4;
  m_osx = getBufferDoubleVal("NAV_X", ok1);
  m_osy = getBufferDoubleVal("NAV_Y", ok2);
  m_curr_heading = getBufferDoubleVal("NAV_HEADING", ok3);
  double wpt = getBufferDoubleVal("WPT_INDEX", ok4);
  m_current_time = getBufferCurrTime();

  if (!ok1 || !ok2)
    postWMessage("No ownship X/Y info in info_buffer.");
  if (!ok3)
    postWMessage("No ownship heading info in info_buffer.");
  if (ok4) {
    int new_index = (int)wpt;
    if (new_index != m_wpt_index) {
      m_wpt_index = new_index;
      m_wpt_change_time = m_current_time;
      m_zig_pending = true;
      m_zig_active = false;
    }
  }

  // 5 seconds after waypoint hit: start the zig
  if (m_zig_pending && (m_current_time - m_wpt_change_time >= 15)) {
    m_zig_pending = false;
    m_zig_active = true;
    m_zig_start_time = m_current_time;
    m_zig_heading = m_curr_heading + m_zig_angle;
    if (m_zig_heading >= 360)
      m_zig_heading -= 360;
    if (m_zig_heading < 0)
      m_zig_heading += 360;
  }

  // Stop the zig after zig_duration seconds
  if (m_zig_active && (m_current_time - m_zig_start_time >= m_zig_duration))
    m_zig_active = false;

  IvPFunction *ipf = 0;

  // While zig is active, produce heading objective function
  if (m_zig_active) {
    ZAIC_PEAK crs_zaic(m_domain, "course");
    crs_zaic.setSummit(m_zig_heading);
    crs_zaic.setPeakWidth(0);
    crs_zaic.setBaseWidth(180);
    crs_zaic.setSummitDelta(0);

    if (crs_zaic.stateOK() == false) {
      postWMessage("ZAIC problem in BHV_ZigLeg");
    } else {
      ipf = crs_zaic.extractIvPFunction();
    }
  }

  if (ipf)
    ipf->setPWT(m_priority_wt);

  return (ipf);
}
