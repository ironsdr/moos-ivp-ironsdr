/************************************************************/
/*    NAME: Berry                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: Odometry.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "Odometry.h"
#include <cmath>

using namespace std;

//---------------------------------------------------------
// Constructor()

Odometry::Odometry()
{
  m_first_reading = false;
  m_first_x = false;
  m_first_y = false;
  m_current_x = 0.0;
  m_current_y = 0.0;
  m_previous_x = 0.0;
  m_previous_y = 0.0;
  m_total_distance = 0.0;
}

//---------------------------------------------------------
// Destructor

Odometry::~Odometry()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool Odometry::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for (p = NewMail.begin(); p != NewMail.end(); p++)
  {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();
    if(key == "NAV_X") {
      m_previous_x = m_current_x;
      m_current_x = msg.GetDouble();
      if(!m_first_x) {
        m_previous_x = m_current_x;
        m_first_x = true;
      }
    }
    else if(key == "NAV_Y") {
      m_previous_y = m_current_y;
      m_current_y = msg.GetDouble();
      if(!m_first_y) {
        m_previous_y = m_current_y;
        m_first_y = true;
      }
    }

    if (key == "FOO")
      cout << "great!";

    else if (key != "APPCAST_REQ") // handled by AppCastingMOOSApp
      reportRunWarning("Unhandled Mail: " + key);
  }
  m_first_reading = m_first_x && m_first_y;

  return (true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool Odometry::OnConnectToServer()
{
  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool Odometry::Iterate()
{
  AppCastingMOOSApp::Iterate();
  // Do your thing here!
  // Calculate distance since last position
  if(m_first_reading) { 
    double dx = m_current_x - m_previous_x;
    double dy = m_current_y - m_previous_y;
    double distance = sqrt(dx*dx + dy*dy);
    
    m_total_distance += distance;
    Notify("ODOMETRY_DIST", m_total_distance);
    
    m_previous_x = m_current_x;
    m_previous_y = m_current_y;
  }
  AppCastingMOOSApp::PostReport();
  return (true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool Odometry::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if (!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for (p = sParams.begin(); p != sParams.end(); p++)
  {
    string orig = *p;
    string line = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;
    if (param == "foo")
    {
      handled = true;
    }
    else if (param == "bar")
    {
      handled = true;
    }

    if (!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void Odometry::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
}

//------------------------------------------------------------
// Procedure: buildReport()

bool Odometry::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  ACTable actab(2);
  actab << "Parameter | Value";
  actab.addHeaderLines();
  actab << "Current X" << m_current_x;
  actab << "Current Y" << m_current_y;
  actab << "Total Distance" << m_total_distance;
  m_msgs << actab.getFormattedString();

  return (true);
}
