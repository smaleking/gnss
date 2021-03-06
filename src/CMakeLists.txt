cmake_minimum_required(VERSION 2.6)
project(SoftGNSS)

set(myTarget SoftGNSS)

#0 define compile options
set(DEFS qchen_debug OUTPUT)
if(WIN32)
set(CMAKE_C_FLAGS "-O0 -std=c++11 -Wall")
endif()

if(UNIX)
set(CMAKE_C_FLAGS "-O0 -std=c++11 -ggdb3 -Wall -pthread")
endif()

#1 set up the include files
 set(INCS
    rtos
    am
    tm
    nm
    pf
    inc
    utils
    kiss_fft
   )

#2 set up rtos source files
set(RTOS_SRCS
    rtos/kiwi_task_msg.cpp
    rtos/kiwi_wrapper.cpp	
   )
file(GLOB_RECURSE RTOS_HDRS rtos/*.h)
source_group("rtos" FILES ${RTOS_SRCS} ${RTOS_HDRS})   
   
#3 set up am source files
set(AM_SRCS
    am/am.cpp
    am/fft_search.cpp	
   )
file(GLOB_RECURSE AM_HDRS am/*.h)
source_group("am" FILES ${AM_SRCS} ${AM_HDRS})      
   
   
#4 set up tm source files
set(TM_SRCS
    tm/tm.cpp
    tm/tracklist.cpp
    tm/ShiftRegister.cpp
    tm/loopfilterconfig.cpp	
   )
file(GLOB_RECURSE TM_HDRS tm/*.h)
source_group("tm" FILES ${TM_SRCS} ${TM_HDRS})         
   
#5 set up nm source files
set(NM_SRCS
    Nm/nm.cpp	
   )
file(GLOB_RECURSE NM_HDRS nm/*.h)
source_group("nm" FILES ${NM_SRCS} ${NM_HDRS})            
   
#6 set up pf source files
set(PF_SRCS
    pf/pf.cpp	
   )
file(GLOB_RECURSE PF_HDRS pf/*.h)
source_group("pf" FILES ${PF_SRCS} ${PF_HDRS})            
   
#7 set up utils source files
set(UTILS_SRCS
    utils/cacode.cpp
    utils/sampleCAcode.cpp
   )
file(GLOB_RECURSE UTILS_HDRS utils/*.h)
source_group("utils" FILES ${UTILS_SRCS} ${UTILS_HDRS})               
   
#8, set up kiss_fft files
set(KISS_FFT_SRCS
    kiss_fft/kiss_fft.c	
  )
file(GLOB_RECURSE KISS_FFT_HDRS kiss_fft/*.h)
source_group("fft" FILES ${KISS_FFT_SRCS} ${KISS_FFT_HDRS})                 

set(SRCS
    ${RTOS_SRCS}
    ${AM_SRCS}
    ${TM_SRCS}
    ${NM_SRCS}
    ${PF_SRCS}
    ${UTILS_SRCS}
    ${KISS_FFT_SRCS}
    main.cpp
   )
   
set(HDRS
	${RTOS_HDRS}
    ${AM_HDRS}
    ${TM_HDRS}
    ${NM_HDRS}
    ${PF_HDRS}
    ${UTILS_HDRS}
    ${KISS_FFT_HDRS}
   )
   
include_directories(${INCS})
add_executable(${myTarget} ${SRCS} ${HDRS})

if(UNIX)
#3 set up the link lib
set(MY_LINK_LIBS m pthread)
target_link_libraries(${myTarget} ${MY_LINK_LIBS})
set_property(
    TARGET ${myTarget}
    PROPERTY COMPILE_DEFINITIONS ${DEFS}
    )
endif()
