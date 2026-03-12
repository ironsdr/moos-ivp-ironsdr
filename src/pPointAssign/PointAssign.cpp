/************************************************************/
/*    NAME: David Irons                                      */
/*    ORGN: MIT, Cambridge MA                                */
/*    FILE: PointAssign.cpp                                  */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include <cstdlib>
#include "MBUtils.h"
#include "ACTable.h"
#include "PointAssign.h"
#include "XYPoint.h"

using namespace std;

PointAssign::PointAssign()
  : m_assign_by_region(false),
    m_got_lastpoint(false),
    m_assigned(false),
    m_vars_preregistered(false),
    m_drain_delay(0)
{
}

PointAssign::~PointAssign()
{
}
void PointAssign::postViewPoint(double x, double y, string label, string color)
 {
   XYPoint point(x, y);
   point.set_label(label);
   point.set_color("vertex", color);  // yellow is handy on dark screen 
   point.set_param("vertex_size", "4");

   string spec = point.get_spec();    // gets the string representation of a point
   Notify("VIEW_POINT", spec);
 }
//---------------------------------------------------------
bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p = NewMail.begin(); p != NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();

    if(key == "VISIT_POINT" && msg.IsString()) {
      string val = msg.GetString();
      if(val == "firstpoint" || val == "lastpoint") {
        if(val == "lastpoint")
          m_got_lastpoint = true;
      }
      else {
        m_visit_points.push_back(val);
      }
    }
    else if(key != "APPCAST_REQ")
      reportRunWarning("Unhandled Mail: " + key);
  }

  if(m_got_lastpoint && !m_assigned)
    assignPoints();

  return true;
}

//---------------------------------------------------------
bool PointAssign::Iterate()
{
  AppCastingMOOSApp::Iterate();

  // On first Iterate, pre-create output variables so pLogger
  // (via WildCardLogging) discovers them well before data flows.
  if(!m_vars_preregistered && !m_vnames.empty()) {
    for(size_t i = 0; i < m_vnames.size(); i++)
      m_Comms.Notify("VISIT_POINT_" + m_vnames[i], "");
    m_vars_preregistered = true;
  }

  // After assignPoints builds queues, wait before draining
  // so pLogger has time to register for the output variables.
  if(m_drain_delay > 0) {
    m_drain_delay--;
    AppCastingMOOSApp::PostReport();
    return true;
  }

  // Drain one message per vehicle per cycle
  map<string, deque<string> >::iterator it;
  for(it = m_outbox.begin(); it != m_outbox.end(); ++it) {
    if(!it->second.empty()) {
      m_Comms.Notify(it->first, it->second.front());
      it->second.pop_front();
    }
  }

  AppCastingMOOSApp::PostReport();
  return true;
}

//---------------------------------------------------------
bool PointAssign::OnConnectToServer()
{
  registerVariables();
  return true;
}

//---------------------------------------------------------
bool PointAssign::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p = sParams.begin(); p != sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;
    if(param == "vname") {
      handled = true;
      if(!value.empty())
        m_vnames.push_back(toupper(value));
    }
    else if(param == "assign_by_region") {
      handled = true;
      m_assign_by_region = (tolower(value) == "true");
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return true;
}

//---------------------------------------------------------
void PointAssign::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("VISIT_POINT", 0);
}

//---------------------------------------------------------
void PointAssign::assignPoints()
{
  if(m_vnames.empty())
    return;

  m_assigned = true;
  m_drain_delay = 20;

  size_t nveh = m_vnames.size();

  // Initialise queues with firstpoint
  for(size_t v = 0; v < nveh; v++) {
    string var = "VISIT_POINT_" + m_vnames[v];
    m_outbox[var].push_back("firstpoint");
  }

  string colors[] = {"yellow", "red"};

  if(m_assign_by_region && nveh == 2) {
    string var_west = "VISIT_POINT_" + m_vnames[0];
    string var_east = "VISIT_POINT_" + m_vnames[1];
    for(size_t k = 0; k < m_visit_points.size(); k++) {
      double xval = parseXFromPoint(m_visit_points[k]);
      double yval = parseYFromPoint(m_visit_points[k]);
      string id   = parseIdFromPoint(m_visit_points[k]);
      if(xval < 87.5) {
        m_outbox[var_west].push_back(m_visit_points[k]);
        postViewPoint(xval, yval, id, colors[0]);
      }
      else {
        m_outbox[var_east].push_back(m_visit_points[k]);
        postViewPoint(xval, yval, id, colors[1]);
      }
    }
  }
  else {
    size_t npts = m_visit_points.size();
    for(size_t v = 0; v < nveh; v++) {
      string var = "VISIT_POINT_" + m_vnames[v];
      size_t start = v * npts / nveh;
      size_t end   = (v + 1) * npts / nveh;
      for(size_t k = start; k < end; k++) {
        m_outbox[var].push_back(m_visit_points[k]);
        double xval = parseXFromPoint(m_visit_points[k]);
        double yval = parseYFromPoint(m_visit_points[k]);
        string id   = parseIdFromPoint(m_visit_points[k]);
        postViewPoint(xval, yval, id, colors[v % 2]);
      }
    }
  }

  // Close each queue with lastpoint
  for(size_t v = 0; v < nveh; v++) {
    string var = "VISIT_POINT_" + m_vnames[v];
    m_outbox[var].push_back("lastpoint");
  }
}

//---------------------------------------------------------
double PointAssign::parseXFromPoint(const string& pt) const
{
  string working = pt;
  while(!working.empty()) {
    string token = biteStringX(working, ',');
    string param = biteStringX(token, '=');
    while(!param.empty() && param[0] == ' ') param.erase(0,1);
    if(param == "x") {
      while(!token.empty() && token[0] == ' ') token.erase(0,1);
      return atof(token.c_str());
    }
  }
  return 0;
}

//---------------------------------------------------------
double PointAssign::parseYFromPoint(const string& pt) const
{
  string working = pt;
  while(!working.empty()) {
    string token = biteStringX(working, ',');
    string param = biteStringX(token, '=');
    while(!param.empty() && param[0] == ' ') param.erase(0,1);
    if(param == "y") {
      while(!token.empty() && token[0] == ' ') token.erase(0,1);
      return atof(token.c_str());
    }
  }
  return 0;
}

//---------------------------------------------------------
string PointAssign::parseIdFromPoint(const string& pt) const
{
  string working = pt;
  while(!working.empty()) {
    string token = biteStringX(working, ',');
    string param = biteStringX(token, '=');
    while(!param.empty() && param[0] == ' ') param.erase(0,1);
    if(param == "id") {
      while(!token.empty() && token[0] == ' ') token.erase(0,1);
      return token;
    }
  }
  return "0";
}

//---------------------------------------------------------
bool PointAssign::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "Vehicles: " << (int)m_vnames.size() << endl;
  m_msgs << "Assign by region: " << (m_assign_by_region ? "true" : "false") << endl;
  m_msgs << "Points collected: " << (int)m_visit_points.size() << endl;
  m_msgs << "Lastpoint received: " << (m_got_lastpoint ? "yes" : "no") << endl;
  m_msgs << "Assigned: " << (m_assigned ? "yes" : "no") << endl;
  m_msgs << "Drain delay: " << m_drain_delay << endl;
  m_msgs << "============================================" << endl;

  ACTable actab(3);
  actab << "Vehicle | Variable | Queue";
  actab.addHeaderLines();
  for(size_t i = 0; i < m_vnames.size(); i++) {
    string var = "VISIT_POINT_" + m_vnames[i];
    int remaining = 0;
    map<string, deque<string> >::iterator it = m_outbox.find(var);
    if(it != m_outbox.end())
      remaining = (int)it->second.size();
    actab << m_vnames[i] << var << remaining;
  }
  m_msgs << actab.getFormattedString();
  return true;
}
