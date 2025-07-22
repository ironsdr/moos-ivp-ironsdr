#ifndef NMEAUDPSender_HEADER
#define NMEAUDPSender_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class NMEAUDPSender : public AppCastingMOOSApp
{
public:
    NMEAUDPSender();
    ~NMEAUDPSender();

protected: // Standard MOOSApp functions to overload  
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    bool Iterate();
    bool OnConnectToServer();
    bool OnStartUp();

protected: // Standard AppCastingMOOSApp function to overload 
    bool buildReport();

protected:
    void registerVariables();
    std::string createGPRMC(double lat, double lon, double speed, double heading);
    std::string nmeaChecksum(const std::string& sentence);

private: // Configuration variables
    std::string m_nav_lat_var;
    std::string m_nav_lon_var;
    std::string m_nav_speed_var;
    std::string m_nav_head_var;

    int m_udp_socket;                  // UDP socket 描述符
    struct sockaddr_in m_udp_dest_addr; // 目标地址结构
    std::string m_udp_address;         // 目标IP（默认127.0.0.1）
    int m_udp_port;                    // 目标端口（默认10120）

private: // State variables
    double m_current_lat;
    double m_current_lon;
    double m_current_speed;
    double m_current_heading;
    bool m_lat_received;
    bool m_lon_received;
    bool m_speed_received;
    bool m_heading_received;
};

#endif