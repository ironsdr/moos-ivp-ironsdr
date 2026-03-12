/************************************************************/
/*    NAME: David Irons                                     */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenPath.h                                       */
/************************************************************/

#ifndef GenPath_HEADER
#define GenPath_HEADER

#include <string>
#include <vector>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

struct VisitPoint {
  double x;
  double y;
  std::string id;
};

class GenPath : public AppCastingMOOSApp
{
 public:
   GenPath();
   ~GenPath();

 protected:
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();
   bool buildReport();
   void registerVariables();

 private:
   void computeAndPublishTour();

   std::vector<VisitPoint> m_points;
   bool   m_got_firstpoint;
   bool   m_got_lastpoint;
   bool   m_tour_published;
   bool   m_tour_complete;
   bool   m_tsp_active;
   std::string m_tour_update_str;
   double m_nav_x;
   double m_nav_y;
   bool   m_nav_received;
};

#endif
