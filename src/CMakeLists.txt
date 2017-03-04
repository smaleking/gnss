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
#3 set up am source files
set(AM_SRCS
    am/am.cpp
    am/fft_search.cpp
   )
#4 set up tm source files
set(TM_SRCS
    tm/tm.cpp
    tm/tracklist.cpp
    tm/ShiftRegister.cpp
    tm/loopfilterconfig.cpp
   )
#5 set up nm source files
set(NM_SRCS
    nm/nm.cpp
   )
#6 set up pf source files
set(PF_SRCS
    pf/pf.cpp
   )
#7 set up utils source files
set(UTILS_SRCS
    utils/cacode.cpp
    utils/sampleCAcode.cpp
   )
#8, set up kiss_fft files
set(KISS_FFT_SRCS
    kiss_fft/kiss_fft.c
  )

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
include_directories(${INCS})
add_executable(${myTarget} ${SRCS})

#3 set up the link lib
if(UNIX)
set(MY_LINK_LIBS m pthread)
target_link_libraries(${myTarget} ${MY_LINK_LIBS})
endif()
set_property(
    TARGET ${myTarget}
    PROPERTY COMPILE_DEFINITIONS ${DEFS}
    )
