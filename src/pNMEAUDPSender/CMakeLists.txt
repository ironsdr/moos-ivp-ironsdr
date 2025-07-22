#--------------------------------------------------------
# The CMakeLists.txt for:                       pNMEAUDPSender
# Author(s):                              Berry
#--------------------------------------------------------

SET(SRC
  NMEAUDPSender.cpp
  NMEAUDPSender_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pNMEAUDPSender ${SRC})

TARGET_LINK_LIBRARIES(pNMEAUDPSender
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread)

