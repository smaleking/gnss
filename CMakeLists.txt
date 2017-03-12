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
	src/rtos/kiwi_data_types.h
	src/rtos/kiwi_task_msg.h
	src/rtos/kiwi_tasks.h
	src/rtos/kiwi_wrapper.h
   )
#3 set up am source files
set(AM_SRCS
    src/am/am.cpp
    src/am/fft_search.cpp
	src/am/am.h
	src/am/fft_search.h
   )
   
#4 set up tm source files
set(TM_SRCS
    src/tm/tm.cpp
    src/tm/tracklist.cpp
    src/tm/ShiftRegister.cpp
    src/tm/loopfilterconfig.cpp
	src/tm/loopfilterconfig.h
	src/tm/ShiftRegister.h
	src/tm/tm.h
	src/tm/tracklist.h
   )
   
#5 set up nm source files
set(NM_SRCS
    src/nm/nm.cpp
	src/nm/nm.h
   )
   
#6 set up pf source files
set(PF_SRCS
    src/pf/pf.cpp
	src/pf/pf.h
   )
   
#7 set up utils source files
set(UTILS_SRCS
    src/utils/cacode.cpp
    src/utils/sampleCAcode.cpp
	src/utils/cacode.h
	src/utils/sampleCAcode.h
   )
#8, set up kiss_fft files
set(KISS_FFT_SRCS
    src/kiss_fft/kiss_fft.c
	_kiss_fft_guts.h
	kiss_fft.h
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
