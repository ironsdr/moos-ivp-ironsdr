/************************************************************/
/*    NAME: Berry                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: NMEAUDPReceiver.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "NMEAUDPReceiver.h"
#include <netinet/in.h>
#include <string>       
#include <vector>

using namespace std;

//---------------------------------------------------------
// Constructor()

NMEAUDPReceiver::NMEAUDPReceiver()
{
}

//---------------------------------------------------------
// Destructor

NMEAUDPReceiver::~NMEAUDPReceiver()
{
}

bool NMEAUDPReceiver::OnStartUp()
{

  m_geodesy.Initialise(LatOrigin,LongOrigin);
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
   if(param == "udp_port") {
      udp_port_ = atoi(value.c_str());
      handled = true;
    }
    else if(param == "moos_var") {
     moos_var_ = value;
     handled = true;
    }
    else if(param == "waypoint_var") {
     waypoint_var_ = value;
     handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);

  }
  
  OpenUDPSocket();
  registerVariables();	
  return(true);
}



bool NMEAUDPReceiver::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString(); 
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

     if(key == "FOO") 
       cout << "great!";

     else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
       reportRunWarning("Unhandled Mail: " + key);
   }
	
   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool NMEAUDPReceiver::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool NMEAUDPReceiver::Iterate()
{
  AppCastingMOOSApp::Iterate();
  // Do your thing here!
      // 主循环处理UDP数据
  ProcessUDPData();
  AppCastingMOOSApp::PostReport();
  return(true);
}

void NMEAUDPReceiver::ProcessUDPData() {
    char buffer[1024];
    socklen_t client_len = sizeof(client_addr_);
    
    int n = recvfrom(udp_socket_, buffer, sizeof(buffer), 0, 
                    (struct sockaddr*)&client_addr_, &client_len);
    
    if (n > 0) {
        std::string nmea_msg(buffer, n);
        m_Comms.Notify(moos_var_, nmea_msg);
        MOOSTrace("Received: %s\n", nmea_msg.c_str());
        cout << "Received: " << nmea_msg.c_str() << endl;

              // Parse the NMEA message
        if(!ParseNMEAMessage(nmea_msg)) {
            parse_errors_++;
        }  
    }
}

bool NMEAUDPReceiver::ParseNMEAMessage(const std::string& nmea_msg)
{
    // Basic NMEA message validation
    if(nmea_msg.empty() || nmea_msg[0] != '$') {
        return false;
    }
    
    // Split into tokens
    vector<string> tokens = parseString(nmea_msg, ',');
    if(tokens.empty()) {
        return false;
    }
    
    // Remove checksum if present
    size_t asterisk_pos = tokens.back().find('*');
    if(asterisk_pos != string::npos) {
        tokens.back() = tokens.back().substr(0, asterisk_pos);
    }
    
    // Check message type and process accordingly
    if(tokens[0] == "$ECWPL" || tokens[0] == "$WPL") {
         MOOSTrace("ERROR opening UDP socket\n");
        return ProcessWPLMessage(tokens);
    }
    
    return false;
}


bool NMEAUDPReceiver::ProcessWPLMessage(const std::vector<string>& tokens)
{
    if(tokens.size() < 5) {
        reportRunWarning("Invalid WPL message - not enough tokens");
        return false;
    }
    
    try {
        // Parse latitude (DDMM.MMM format)
        string lat_str = tokens[1];
        string lat_hemi = tokens[2];
        double lat_deg = stod(lat_str.substr(0, 2));
        double lat_min = stod(lat_str.substr(2));
        double latitude = lat_deg + (lat_min / 60.0);
        if(lat_hemi == "S") {
            latitude = -latitude;
        }
        
        // Parse longitude (DDDMM.MMM format)
        string lon_str = tokens[3];
        string lon_hemi = tokens[4];
        double lon_deg = stod(lon_str.substr(0, 3));
        double lon_min = stod(lon_str.substr(3));
        double longitude = lon_deg + (lon_min / 60.0);
        if(lon_hemi == "W") {
            longitude = -longitude;
        }
        
        double north, east;
        m_geodesy.LatLong2LocalUTM(latitude, longitude, north, east);
        string waypoint_str = "points=" + doubleToStringX(east, 6) + "," + doubleToStringX(north, 6);
      //  string waypoint_str = "points=" + doubleToStringX(latitude, 8) + "," + doubleToStringX(longitude, 8);
        // Publish to MOOSDB
        m_Comms.Notify(waypoint_var_, waypoint_str);
        wpl_count_++;
        
        
        return true;
    }
    catch(const exception& e) {
        reportRunWarning("Error parsing WPL message: " + string(e.what()));
        return false;
    }
}


void NMEAUDPReceiver::OpenUDPSocket() {
    udp_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (udp_socket_ < 0) {
        MOOSTrace("ERROR opening UDP socket\n");
        exit(1);
    }

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(udp_port_);

    if (bind(udp_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        MOOSTrace("ERROR binding UDP port %d\n", udp_port_);
        exit(1);
    }
}

void NMEAUDPReceiver::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  // Register("FOOBAR", 0);
}


bool NMEAUDPReceiver::buildReport() 
{

    m_msgs << "NMEA UDP Receiver Status\n";
    m_msgs << "========================\n\n";
    
    m_msgs << "UDP Port: " << udp_port_ << "\n";
    m_msgs << "Listening for NMEA messages...\n\n";
    
    m_msgs << "Message Statistics:\n";
    m_msgs << "------------------\n";
    m_msgs << "Waypoints Received: " << wpl_count_ << "\n";
    m_msgs << "Parse Errors:       " << parse_errors_ << "\n\n";
    
    m_msgs << "Configuration:\n";
    m_msgs << "--------------\n";
    m_msgs << "Raw NMEA MOOS Variable: " << moos_var_ << "\n";
    m_msgs << "Waypoint MOOS Variable: " << waypoint_var_ << "\n";

    
  return(true);
}




