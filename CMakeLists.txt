##########################################################################
cmake_minimum_required(VERSION 3.15)
##########################################################################
project("portenta-x8-eeprom-utils")
##########################################################################
set(EEPMAKE_TARGET eepmake)
set(EEPDUMP_TARGET eepdump)
##########################################################################
add_executable(${EEPMAKE_TARGET} eepmake.c)
add_executable(${EEPDUMP_TARGET} eepdump.c)
##########################################################################
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${EEPMAKE_TARGET} PRIVATE -Wall -Werror -Wextra -Wpedantic)
  target_compile_options(${EEPDUMP_TARGET} PRIVATE -Wall -Werror -Wextra -Wpedantic)
endif()
##########################################################################
install(TARGETS ${EEPMAKE_TARGET} ${EEPDUMP_TARGET} DESTINATION bin)
##########################################################################
