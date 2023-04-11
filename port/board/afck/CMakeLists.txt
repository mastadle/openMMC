#Select which microcontroller and board are being used
if (NOT TARGET_CONTROLLER)
  set(TARGET_CONTROLLER "LPC1764" CACHE STRING "Target MMC Controller")
endif()

if (NOT TARGET_BOARD_NAME)
  set(TARGET_BOARD_NAME "AFCK TIQI" CACHE STRING "Board Name")
endif()

#List all modules used by this board
set(TARGET_MODULES
  # "FRU" # Don't use filed replaceable unit
  "NO_IPMI"
  "PAYLOAD"
  "SDR" # Don't need sensor data repository but sensor_init and sdr_head are required by other modules
  "SCANSTA1101"
  "ADN4604"
  "DAC_AD84XX"
  # "HOTSWAP_SENSOR"
  "SENSOR"
  # "EEPROM_AT24MAC" # only needed to write/read fru
  "LM75"
  "MAX6642"
  "INA220_VOLTAGE"
  "INA220_CURRENT"
  "HPM"
  # "RTM" # We don't have a rear transition module yet
  "UART_DEBUG"
  "SYSUTILS"
  "FLASH_SPI"
  )

if (NOT DISABLE_WATCHDOG)
  list(APPEND TARGET_MODULES "WATCHDOG")
endif()

set(BOARD_PATH ${CMAKE_CURRENT_SOURCE_DIR})

#Include the modules sources
##Sensors SDR list
set( PROJ_SRCS ${PROJ_SRCS} ${BOARD_PATH}/sdr_list.c )
set( PROJ_SRCS ${PROJ_SRCS} ${BOARD_PATH}/i2c_mapping.c )

if (";${TARGET_MODULES};" MATCHES ";PAYLOAD;")
  set(PROJ_SRCS ${PROJ_SRCS} ${BOARD_PATH}/payload.c)
  set(MODULES_FLAGS "${MODULES_FLAGS} -DMODULE_PAYLOAD")
endif()

#Extra definitions
if (FRU_WRITE_EEPROM)
  message(STATUS "FRU EEPROM will be written if no valid data is found!")
  set(MODULES_FLAGS "${MODULES_FLAGS} -DFRU_WRITE_EEPROM")
  set(FRU_WRITE_EEPROM false CACHE BOOL "Set this flag to enable writing the board ID on the EEPROM" FORCE)
endif()

#Set the variables in the main scope
set(TARGET_MODULES ${TARGET_MODULES} PARENT_SCOPE)
set(MODULES_FLAGS ${MODULES_FLAGS} PARENT_SCOPE)
set(PROJ_SRCS ${PROJ_SRCS} PARENT_SCOPE)
set(PROJ_HDRS ${PROJ_HDRS} ${BOARD_PATH})
set(PROJ_HDRS ${PROJ_HDRS} PARENT_SCOPE)
