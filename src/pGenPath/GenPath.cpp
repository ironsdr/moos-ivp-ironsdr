/************************************************************/
/*    NAME: David Irons                                     */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenPath.cpp                                     */
/************************************************************/

#include <iterator>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include "MBUtils.h"
#include "ACTable.h"
#include "GenPath.h"
#include "XYSegList.h"

using namespace std;

GenPath::GenPath()
  : m_got_firstpoint(false),
    m_got_lastpoint(false),
    m_tour_published(false),
    m_tour_complete(false),
    m_tsp_active(false),
    m_nav_x(0),
    m_nav_y(0),
    m_nav_received(false)
{
}

GenPath::~GenPath()
{
}

bool GenPath::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p = NewMail.begin(); p != NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();

    if(key == "VISIT_POINT" && msg.IsString()) {
      string val = msg.GetString();
      if(val == "firstpoint") {
        m_got_firstpoint = true;
      }
      else if(val == "lastpoint") {
        m_got_lastpoint = true;
      }
      else if(!m_got_lastpoint) {
        VisitPoint vp;
        vp.x = 0;
        vp.y = 0;
        vp.id = "0";

        string working = val;
        while(!working.empty()) {
          string token = biteStringX(working, ',');
          string param = biteStringX(token, '=');
          while(!param.empty() && param[0] == ' ') param.erase(0, 1);
          while(!token.empty() && token[0] == ' ') token.erase(0, 1);
          if(param == "x")
            vp.x = atof(token.c_str());
          else if(param == "y")
            vp.y = atof(token.c_str());
          else if(param == "id")
            vp.id = token;
        }
        m_points.push_back(vp);
      }
    }
    else if(key == "NAV_X" && msg.IsDouble()) {
      m_nav_x = msg.GetDouble();
      m_nav_received = true;
    }
    else if(key == "NAV_Y" && msg.IsDouble()) {
      m_nav_y = msg.GetDouble();
      m_nav_received = true;
    }
    else if(key == "TSP" && msg.IsString()) {
      string val = msg.GetString();
      if(val == "true" && !m_tsp_active) {
        m_tsp_active = true;
        m_tour_complete = false;
        if(m_tour_published && !m_tour_update_str.empty())
          Notify("TSP_UPDATES", m_tour_update_str);
      }
      else if(val == "false") {
        m_tsp_active = false;
      }
    }
    else if(key == "TOUR_COMPLETE") {
      if(!m_tour_complete) {
        m_tour_complete = true;
        Notify("TSP", "false");
        Notify("RETURN", "true");
      }
    }
    else if(key != "APPCAST_REQ")
      reportRunWarning("Unhandled Mail: " + key);
  }

  return true;
}

bool GenPath::OnConnectToServer()
{
  registerVariables();
  return true;
}

bool GenPath::Iterate()
{
  AppCastingMOOSApp::Iterate();

  if(m_got_lastpoint && m_nav_received && !m_tour_published)
    computeAndPublishTour();

  AppCastingMOOSApp::PostReport();
  return true;
}

bool GenPath::OnStartUp()
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
    (void)handled;
    (void)value;

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return true;
}

void GenPath::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("VISIT_POINT", 0);
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  Register("TSP", 0);
  Register("TOUR_COMPLETE", 0);
}

void GenPath::computeAndPublishTour()
{
  m_tour_published = true;

  if(m_points.empty())
    return;

  vector<bool> used(m_points.size(), false);
  vector<int>  order;
  double cx = m_nav_x;
  double cy = m_nav_y;

  for(size_t round = 0; round < m_points.size(); round++) {
    int    best_idx  = -1;
    double best_dist = -1;
    for(int i = 0; i < (int)m_points.size(); i++) {
      if(used[i])
        continue;
      double dx = m_points[i].x - cx;
      double dy = m_points[i].y - cy;
      double dist = sqrt(dx * dx + dy * dy);
      if(best_idx < 0 || dist < best_dist) {
        best_dist = dist;
        best_idx  = i;
      }
    }
    if(best_idx >= 0) {
      order.push_back(best_idx);
      used[best_idx] = true;
      cx = m_points[best_idx].x;
      cy = m_points[best_idx].y;
    }
  }

  XYSegList seglist;
  for(size_t i = 0; i < order.size(); i++)
    seglist.add_vertex(m_points[order[i]].x, m_points[order[i]].y);

  m_tour_update_str = "points = ";
  m_tour_update_str += seglist.get_spec();
  Notify("TSP_UPDATES", m_tour_update_str);
}

bool GenPath::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "Total points:   " << (int)m_points.size() << endl;
  m_msgs << "Got firstpoint: " << (m_got_firstpoint ? "yes" : "no") << endl;
  m_msgs << "Got lastpoint:  " << (m_got_lastpoint ? "yes" : "no") << endl;
  m_msgs << "Nav received:   " << (m_nav_received ? "yes" : "no") << endl;
  m_msgs << "Tour published: " << (m_tour_published ? "yes" : "no") << endl;
  m_msgs << "Tour complete:  " << (m_tour_complete ? "yes" : "no") << endl;
  m_msgs << "============================================" << endl;

  return true;
}
