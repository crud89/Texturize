###################################################################################################
#####                                                                                         #####
##### Call this function in order to add an application depending on a cache option.          #####
#####                                                                                         #####
##### Syntax example:                                                                         #####
##### > OPTION(BUILD_APP "My App" ON)                                                         #####
##### > DEFINE_APP(BUILD_APP "app/MyApp")                                                     #####
#####                                                                                         #####
###################################################################################################

FUNCTION(DEFINE_APP OPTION_NAME APP_DIRECTORY)
  IF(OPTION_NAME)
    MESSAGE(STATUS "Adding app at '${APP_DIRECTORY}'...")
    ADD_SUBDIRECTORY(${APP_DIRECTORY})
  ELSE(OPTION_NAME)
    MESSAGE(STATUS "Skipping app at '${APP_DIRECTORY}'")
  ENDIF(OPTION_NAME)
ENDFUNCTION(DEFINE_APP OPTION_NAME APP_DIRECTORY)