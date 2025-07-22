#include <iterator>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unistd.h> 
#include "MBUtils.h"
#include "ACTable.h"
#include "NMEAUDPSender.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

NMEAUDPSender::NMEAUDPSender() :
    m_udp_port(10120),
    m_nav_lat_var("NAV_LAT"),
    m_nav_lon_var("NAV_LONG"),
    m_nav_speed_var("NAV_SPEED"),
    m_nav_head_var("NAV_HEADING"),
    m_current_lat(0.0),
    m_current_lon(0.0),
    m_current_speed(0.0),
    m_current_heading(0.0),
    m_lat_received(false),
    m_lon_received(false),
    m_speed_received(false),
    m_heading_received(false)
{
    m_udp_address = "127.0.0.1";
}

//---------------------------------------------------------
// Destructor

NMEAUDPSender::~NMEAUDPSender() {
    if (m_udp_socket >= 0) {
        close(m_udp_socket); // 关闭 socket
    }
}
//---------------------------------------------------------
// Procedure: OnNewMail()

bool NMEAUDPSender::OnNewMail(MOOSMSG_LIST &NewMail)
{
    AppCastingMOOSApp::OnNewMail(NewMail);

    MOOSMSG_LIST::iterator p;
    for(p=NewMail.begin(); p!=NewMail.end(); p++) {
        CMOOSMsg &msg = *p;
        string key    = msg.GetKey();

        if(key == m_nav_lat_var) {
            m_current_lat = msg.GetDouble();
            m_lat_received = true;
        }
        else if(key == m_nav_lon_var) {
            m_current_lon = msg.GetDouble();
            m_lon_received = true;
        }
        else if(key == m_nav_speed_var) {
            m_current_speed = msg.GetDouble();
            m_speed_received = true;
        }
        else if(key == m_nav_head_var) {
            m_current_heading = msg.GetDouble();
            m_heading_received = true;
        }
        else if(key != "APPCAST_REQ") {
            reportRunWarning("Unhandled Mail: " + key);
        }
    }
    
    return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()

bool NMEAUDPSender::Iterate() {
    AppCastingMOOSApp::Iterate();
    
    if (m_lat_received && m_lon_received) {
        string gprmc = createGPRMC(m_current_lat, m_current_lon, m_current_speed, m_current_heading);
        
        // 发送 UDP 数据
        ssize_t sent_bytes = sendto(
            m_udp_socket,
            gprmc.c_str(),
            gprmc.size(),
            0,
            (struct sockaddr*)&m_udp_dest_addr,
            sizeof(m_udp_dest_addr)
        );

        if (sent_bytes < 0) {
            reportRunWarning("UDP send failed: " + string(strerror(errno)));
        } else {
            reportEvent("Sent NMEA: " + gprmc);
        }
    }
    
    AppCastingMOOSApp::PostReport();
    return true;
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool NMEAUDPSender::OnConnectToServer()
{
    registerVariables();
    return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()

bool NMEAUDPSender::OnStartUp()
{
    AppCastingMOOSApp::OnStartUp();

    STRING_LIST sParams;
    m_MissionReader.EnableVerbatimQuoting(false);
    if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
        reportConfigWarning("No config block found for " + GetAppName());

    STRING_LIST::iterator p;
    for(p=sParams.begin(); p!=sParams.end(); p++) {
        string orig  = *p;
        string line  = *p;
        string param = tolower(biteStringX(line, '='));
        string value = line;

        bool handled = false;
        if(param == "udp_address") {
            m_udp_address = value;
            handled = true;
        }
        else if(param == "udp_port") {
            if(!MOOSIsNumeric(value)) {
                reportConfigWarning("Invalid UDP port: " + value);
            } else {
                m_udp_port = atoi(value.c_str());
            }
            handled = true;
        }
        else if(param == "nav_lat_var") {
            m_nav_lat_var = value;
            handled = true;
        }
        else if(param == "nav_lon_var") {
            m_nav_lon_var = value;
            handled = true;
        }
        else if(param == "nav_speed_var") {
            m_nav_speed_var = value;
            handled = true;
        }
        else if(param == "nav_head_var") {
            m_nav_head_var = value;
            handled = true;
        }

        if(!handled)
            reportUnhandledConfigWarning(orig);
    }

    // 创建 UDP Socket
    m_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_udp_socket < 0) {
        reportConfigWarning("Failed to create UDP socket");
        return false;
    }

    // 设置目标地址
    memset(&m_udp_dest_addr, 0, sizeof(m_udp_dest_addr));
    m_udp_dest_addr.sin_family = AF_INET;
    m_udp_dest_addr.sin_port = htons(m_udp_port);
    m_udp_dest_addr.sin_addr.s_addr = inet_addr(m_udp_address.c_str());
    
    registerVariables();
    return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void NMEAUDPSender::registerVariables()
{
    AppCastingMOOSApp::RegisterVariables();
    Register(m_nav_lat_var, 0);
    Register(m_nav_lon_var, 0);
    Register(m_nav_speed_var, 0);
    Register(m_nav_head_var, 0);
}

//---------------------------------------------------------
// Procedure: createGPRMC()

string NMEAUDPSender::createGPRMC(double lat, double lon, double speed, double heading)
{
    time_t now = time(0);
    tm *gmtm = gmtime(&now);
    
    // Format time HHMMSS
    char time_buf[7];
    strftime(time_buf, sizeof(time_buf), "%H%M%S", gmtm);
    
    // Format date DDMMYY
    char date_buf[7];
    strftime(date_buf, sizeof(date_buf), "%d%m%y", gmtm);
    
    // Convert latitude to NMEA format DDMM.MMMM,N/S
    char lat_dir = (lat >= 0) ? 'N' : 'S';
    double abs_lat = fabs(lat);
    int lat_deg = static_cast<int>(abs_lat);
    double lat_min = (abs_lat - lat_deg) * 60.0;
    
    ostringstream lat_ss;
    lat_ss << setw(2) << setfill('0') << lat_deg;
    lat_ss << setw(7) << fixed << setprecision(4) << lat_min;
    string lat_str = lat_ss.str() + "," + lat_dir;
    
    // Convert longitude to NMEA format DDDMM.MMMM,E/W
    char lon_dir = (lon >= 0) ? 'E' : 'W';
    double abs_lon = fabs(lon);
    int lon_deg = static_cast<int>(abs_lon);
    double lon_min = (abs_lon - lon_deg) * 60.0;
    
    ostringstream lon_ss;
    lon_ss << setw(3) << setfill('0') << lon_deg;
    lon_ss << setw(7) << fixed << setprecision(4) << lon_min;
    string lon_str = lon_ss.str() + "," + lon_dir;
    
    // Format speed (knots) and heading
    ostringstream speed_ss;
    speed_ss << fixed << setprecision(1) << speed;
    
    ostringstream head_ss;
    head_ss << fixed << setprecision(1) << heading;
    
    // Construct GPRMC sentence
    ostringstream gprmc_ss;
    gprmc_ss << "GPRMC," << time_buf << ",A," 
             << lat_str << "," << lon_str << ","
             << speed_ss.str() << "," << head_ss.str() << ","
             << date_buf << ",,,A";
    
    string gprmc = gprmc_ss.str();
    string checksum = nmeaChecksum(gprmc);
    
    return "$" + gprmc + "*" + checksum + "\r\n";
}

//---------------------------------------------------------
// Procedure: nmeaChecksum()

string NMEAUDPSender::nmeaChecksum(const string& sentence)
{
    unsigned char checksum = 0;
    for(char c : sentence) {
        checksum ^= c;
    }
    
    ostringstream ss;
    ss << hex << uppercase << setw(2) << setfill('0') << (int)checksum;
    return ss.str();
}

//------------------------------------------------------------
// Procedure: buildReport()

bool NMEAUDPSender::buildReport() 
{
    m_msgs << "============================================\n";
    m_msgs << "NMEA UDP Sender Status\n";
    m_msgs << "============================================\n\n";
    
    m_msgs << "UDP Configuration:\n";
    m_msgs << "  Address: " << m_udp_address << "\n";
    m_msgs << "  Port:    " << m_udp_port << "\n\n";
    
    m_msgs << "MOOS Variables:\n";
    m_msgs << "  Latitude:  " << m_nav_lat_var << "\n";
    m_msgs << "  Longitude: " << m_nav_lon_var << "\n";
    m_msgs << "  Speed:     " << m_nav_speed_var << "\n";
    m_msgs << "  Heading:   " << m_nav_head_var << "\n\n";
    
    m_msgs << "Current Values:\n";
    ACTable actab(4);
    actab << "Latitude | Longitude | Speed (kts) | Heading";
    actab.addHeaderLines();
    actab << m_current_lat << m_current_lon << m_current_speed << m_current_heading;
    m_msgs << actab.getFormattedString();

    return true;
}