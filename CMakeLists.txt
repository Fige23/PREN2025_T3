# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 3.10.0)

# THE VERSION NUMBER
SET (Tutorial_VERSION_MAJOR 1)
SET (Tutorial_VERSION_MINOR 0)

if (DEFINED ENV{SdkRootDirPath})
	SET(SdkRootDirPath "$ENV{SdkRootDirPath}")
endif()

if (NOT DEFINED SdkRootDirPath)
	MESSAGE(FATAL_ERROR "SDK path could not be determined. Please update SdkRootDirPath value from your CMake presets file(s).")
endif()

# RESET FLAG VALUES
SET(CMAKE_ASM_FLAGS_DEBUG "")
SET(CMAKE_ASM_FLAGS_RELEASE "")
SET(CMAKE_C_FLAGS_DEBUG "")
SET(CMAKE_C_FLAGS_RELEASE "")
SET(CMAKE_CXX_FLAGS_DEBUG "")
SET(CMAKE_CXX_FLAGS_RELEASE "")
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "")

SET(CMAKE_STATIC_LIBRARY_PREFIX)
SET(CMAKE_STATIC_LIBRARY_SUFFIX)

SET(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
SET(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

# CURRENT DIRECTORY
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

SET(EXECUTABLE_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})
SET(LIBRARY_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})

set(CMAKE_USER_MAKE_RULES_OVERRIDE "user_override.cmake")

project(PREN_Puzzleroboter)

# ENABLE ASM
ENABLE_LANGUAGE(ASM)

set(MCUX_SDK_PROJECT_NAME PREN_Puzzleroboter.elf)

include(${ProjDirPath}/flags.cmake)

add_executable(${MCUX_SDK_PROJECT_NAME} "${ProjDirPath}/CMSIS/mpu_armv7.h"
"${ProjDirPath}/CMSIS/core_cm4.h"
"${ProjDirPath}/CMSIS/cmsis_version.h"
"${ProjDirPath}/CMSIS/cmsis_gcc.h"
"${ProjDirPath}/CMSIS/cmsis_compiler.h"
"${ProjDirPath}/board/pin_mux.h"
"${ProjDirPath}/board/pin_mux.c"
"${ProjDirPath}/board/peripherals.h"
"${ProjDirPath}/board/peripherals.c"
"${ProjDirPath}/board/clock_config.h"
"${ProjDirPath}/board/clock_config.c"
"${ProjDirPath}/board/board.h"
"${ProjDirPath}/board/board.c"
"${ProjDirPath}/device/system_MK22F51212.h"
"${ProjDirPath}/device/system_MK22F51212.c"
"${ProjDirPath}/device/MK22F51212_features.h"
"${ProjDirPath}/device/MK22F51212.h"
"${ProjDirPath}/device/fsl_device_registers.h"
"${ProjDirPath}/drivers/fsl_uart.h"
"${ProjDirPath}/drivers/fsl_uart.c"
"${ProjDirPath}/drivers/fsl_smc.h"
"${ProjDirPath}/drivers/fsl_smc.c"
"${ProjDirPath}/drivers/fsl_rtc.h"
"${ProjDirPath}/drivers/fsl_rtc.c"
"${ProjDirPath}/drivers/fsl_port.h"
"${ProjDirPath}/drivers/fsl_i2c.h"
"${ProjDirPath}/drivers/fsl_i2c.c"
"${ProjDirPath}/drivers/fsl_gpio.h"
"${ProjDirPath}/drivers/fsl_gpio.c"
"${ProjDirPath}/drivers/fsl_ftm.h"
"${ProjDirPath}/drivers/fsl_ftm.c"
"${ProjDirPath}/drivers/fsl_dspi.h"
"${ProjDirPath}/drivers/fsl_dspi.c"
"${ProjDirPath}/drivers/fsl_common_arm.h"
"${ProjDirPath}/drivers/fsl_common_arm.c"
"${ProjDirPath}/drivers/fsl_common.h"
"${ProjDirPath}/drivers/fsl_common.c"
"${ProjDirPath}/drivers/fsl_clock.h"
"${ProjDirPath}/drivers/fsl_clock.c"
"${ProjDirPath}/drivers/fsl_adc16.h"
"${ProjDirPath}/drivers/fsl_adc16.c"
"${ProjDirPath}/source/platform.h"
"${ProjDirPath}/source/main.c"
"${ProjDirPath}/source/proto/serial_port_tinyk22.c"
"${ProjDirPath}/source/proto/serial_port.h"
"${ProjDirPath}/source/proto/proto_io.h"
"${ProjDirPath}/source/proto/proto_io.c"
"${ProjDirPath}/source/proto/protocol.h"
"${ProjDirPath}/source/proto/protocol.c"
"${ProjDirPath}/source/proto/console_uart_sim.h"
"${ProjDirPath}/source/proto/console_uart_sim.c"
"${ProjDirPath}/source/proto/cmd.h"
"${ProjDirPath}/source/proto/cmd.c"
"${ProjDirPath}/source/utils/util.h"
"${ProjDirPath}/source/utils/util.c"
"${ProjDirPath}/source/utils/parse_kv.h"
"${ProjDirPath}/source/utils/parse_kv.c"
"${ProjDirPath}/source/utils/ftm3.h"
"${ProjDirPath}/source/utils/ftm3.c"
"${ProjDirPath}/source/test_tools/test_tools.h"
"${ProjDirPath}/source/test_tools/test_tools.c"
"${ProjDirPath}/source/test_tools/demo_draw.h"
"${ProjDirPath}/source/test_tools/demo_draw.c"
"${ProjDirPath}/source/test_tools/debug.h"
"${ProjDirPath}/source/test_tools/debug.c"
"${ProjDirPath}/source/test_tools/calibration.h"
"${ProjDirPath}/source/test_tools/calibration.c"
"${ProjDirPath}/source/motion/motion.h"
"${ProjDirPath}/source/motion/motion.c"
"${ProjDirPath}/source/motion/limit_switch.h"
"${ProjDirPath}/source/motion/limit_switch.c"
"${ProjDirPath}/source/position/position.h"
"${ProjDirPath}/source/position/position.c"
"${ProjDirPath}/source/io/io.h"
"${ProjDirPath}/source/io/io.c"
"${ProjDirPath}/source/job/job_move.h"
"${ProjDirPath}/source/job/job_move.c"
"${ProjDirPath}/source/job/job_motion_finish.h"
"${ProjDirPath}/source/job/job_motion_finish.c"
"${ProjDirPath}/source/job/job_home.h"
"${ProjDirPath}/source/job/job_home.c"
"${ProjDirPath}/source/job/job.h"
"${ProjDirPath}/source/job/job.c"
"${ProjDirPath}/source/config/robot_config.h"
"${ProjDirPath}/source/config/motion_config.h"
"${ProjDirPath}/source/config/home_config.h"
"${ProjDirPath}/source/config/hardware_config.h"
"${ProjDirPath}/source/config/encoder_config.h"
"${ProjDirPath}/source/config/communication_config.h"
"${ProjDirPath}/source/config/calibration_config.h"
"${ProjDirPath}/source/config/build_config.h"
"${ProjDirPath}/source/com/uart1.c"
"${ProjDirPath}/source/com/uart0.c"
"${ProjDirPath}/source/com/uart.h"
"${ProjDirPath}/source/app/poll.h"
"${ProjDirPath}/source/app/poll.c"
"${ProjDirPath}/source/app/init.h"
"${ProjDirPath}/source/app/init.c"
"${ProjDirPath}/source/app/bot_engine.h"
"${ProjDirPath}/source/app/bot_engine.c"
"${ProjDirPath}/startup/system.h"
"${ProjDirPath}/startup/system.c"
"${ProjDirPath}/startup/startup_mk22f51212.c"
)




if(${LIBRARY_TYPE} STREQUAL "REDLIB")
    target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE $ENV{ARMGCC_DIR}/redlib/include)
endif()

set(c_include_directories "${ProjDirPath}/source/app"
"${ProjDirPath}/source/test_tools"
"${ProjDirPath}/source/job"
"${ProjDirPath}/drivers"
"${ProjDirPath}/source/position"
"${ProjDirPath}/device"
"${ProjDirPath}/CMSIS"
"${ProjDirPath}/source/config"
"${ProjDirPath}/source/motion"
"${ProjDirPath}/source/proto"
"${ProjDirPath}/source/io"
"${ProjDirPath}/source/com"
"${ProjDirPath}/source/utils"
"${ProjDirPath}/startup"
"${ProjDirPath}/source"
"${ProjDirPath}/board"
)


target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:C>:${c_include_directories}>)

set(cpp_include_directories "")


target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${cpp_include_directories}>)

set(include_files "")


target_precompile_headers(${MCUX_SDK_PROJECT_NAME} PRIVATE ${include_files})

set(link_directories "")


set(link_libraries "")


target_link_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE ${link_directories})
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE ${TARGET_LINK_SYSTEM_LIBRARIES} ${link_libraries})
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)





