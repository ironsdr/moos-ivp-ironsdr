#--------------------------------------------------------
# The CMakeLists.txt for:                       pNMEAUDPReceiver
# Author(s):                              Berry
#--------------------------------------------------------

SET(SRC
  NMEAUDPReceiver.cpp
  NMEAUDPReceiver_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pNMEAUDPReceiver ${SRC})

TARGET_LINK_LIBRARIES(pNMEAUDPReceiver
   ${MOOS_LIBRARIES}
   MOOSGeodesy
   apputil
   mbutil
   m
   pthread)

