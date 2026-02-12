##############################################################################
# FILE:        moos-ivp-extend/src/CMakeLists.txt
# DATE:        Spring 2026
# DESCRIPTION: CMakeLists.txt file for the moos-ivp-extend source directory
# AUTHOR: David Irons
##############################################################################


#============================================================================
# Add the libraries in the current directory to the include path
#============================================================================
FILE(GLOB LOCAL_LIBRARY_DIRS ./lib_*)
INCLUDE_DIRECTORIES(${LOCAL_LIBRARY_DIRS})

#============================================================================
# List the subdirectories to build...
#============================================================================
ADD_SUBDIRECTORY(lib_behaviors-test)
ADD_SUBDIRECTORY(pExampleApp)
ADD_SUBDIRECTORY(pXRelayTest)
ADD_SUBDIRECTORY(pOdometry) 
##############################################################################
#                           END of CMakeLists.txt
##############################################################################

