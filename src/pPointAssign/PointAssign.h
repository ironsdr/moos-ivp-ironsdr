/************************************************************/
/*    NAME: David Irons                                      */
/*    ORGN: MIT, Cambridge MA                                */
/*    FILE: PointAssign.h                                    */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef PointAssign_HEADER
#define PointAssign_HEADER

#include <string>
#include <vector>
#include <deque>
#include <map>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class PointAssign : public AppCastingMOOSApp
{
 public:
   PointAssign();
   ~PointAssign();

 protected:
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();
   bool buildReport();
   void registerVariables();

 private:
   std::vector<std::string> m_vnames;
   bool m_assign_by_region;

   std::vector<std::string> m_visit_points;
   bool m_got_lastpoint;
   bool m_assigned;
   bool m_vars_preregistered;
   int  m_drain_delay;

   std::map<std::string, std::deque<std::string> > m_outbox;

   void assignPoints();
   void postViewPoint(double x, double y, std::string label, std::string color);
   double parseXFromPoint(const std::string& pt) const;
   double parseYFromPoint(const std::string& pt) const;
   std::string parseIdFromPoint(const std::string& pt) const;
};

#endif
