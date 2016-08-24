cmake_minimum_required(VERSION 2.6)
project(SoftGNSS)

set(myTarget SoftGNSS)

#0 define compile options
set(DEFS qchen_debug OUTPUT)
set(CMAKE_C_FLAGS "-O0 -std=c++11 -ggdb3 -Wall -pthread")

#1 set up the include files
 set(INCS
    src/rtos
    src/am
    src/tm
    src/nm
    src/pf
    src/inc
    src/utils
    src/kiss_fft
   )

#2 set up rtos source files
set(RTOS_SRCS
    src/rtos/kiwi_task_msg.cpp
    src/rtos/kiwi_wrapper.cpp
   )
#3 set up am source files
set(AM_SRCS
    src/am/am.cpp
    src/am/fft_search.cpp
   )
#4 set up tm source files
set(TM_SRCS
    src/tm/tm.cpp
    src/tm/tracklist.cpp
    src/tm/ShiftRegister.cpp
    src/tm/loopfilterconfig.cpp
   )
#5 set up nm source files
set(NM_SRCS
    src/nm/nm.cpp
   )
#6 set up pf source files
set(PF_SRCS
    src/pf/pf.cpp
   )
#7 set up utils source files
set(UTILS_SRCS
    src/utils/cacode.cpp
    src/utils/sampleCAcode.cpp
   )
#8, set up kiss_fft files
set(KISS_FFT_SRCS
    src/kiss_fft/kiss_fft.c
  )

set(SRCS
    ${RTOS_SRCS}
    ${AM_SRCS}
    ${TM_SRCS}
    ${NM_SRCS}
    ${PF_SRCS}
    ${UTILS_SRCS}
    ${KISS_FFT_SRCS}
    src/main.cpp
   )
include_directories(${INCS})
add_executable(${myTarget} ${SRCS})

#3 set up the link lib
set(MY_LINK_LIBS m pthread)
target_link_libraries(${myTarget} ${MY_LINK_LIBS})
set_property(
    TARGET ${myTarget}
    PROPERTY COMPILE_DEFINITIONS ${DEFS}
    )
