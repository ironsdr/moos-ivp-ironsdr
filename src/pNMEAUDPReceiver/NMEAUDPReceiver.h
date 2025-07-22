#include "MOOS/libMOOS/App/MOOSApp.h"
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include <arpa/inet.h>
#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"

class NMEAUDPReceiver : public AppCastingMOOSApp {
public:
    NMEAUDPReceiver();
    ~NMEAUDPReceiver();
public:
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    bool OnConnectToServer();
    bool Iterate();
    bool OnStartUp();
    
protected:
    bool buildReport();
    void registerVariables();
    void OpenUDPSocket();
    void ProcessUDPData();
    bool ParseNMEAMessage(const std::string& nmea_msg);
    bool ProcessWPLMessage(const std::vector<std::string>& tokens);

private:
    int udp_socket_;
    int udp_port_ = 10110;
    struct sockaddr_in client_addr_;
    std::string moos_var_ = "NMEA_IN";
    std::string waypoint_var_ = "WPT_UPDATE_NMEA";
    
    CMOOSGeodesy m_geodesy;
    double LatOrigin = 43.783417;
    double LongOrigin = -70.146822;
    
    // Statistics
    unsigned int wpl_count_ = 0;
    unsigned int parse_errors_ = 0;
};