cmake_minimum_required(VERSION 3.25)
project(watch_u575_astra C CXX ASM)
set(WATCH_FIRMWARE_VERSION "dev" CACHE STRING "Firmware release version")
set(WATCH_SOURCE_DATE_EPOCH "0" CACHE STRING "Reproducible build epoch")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CPU -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard)
add_compile_options(${CPU} -ffunction-sections -fdata-sections -fno-common -fstack-usage
  -Wall -Wextra -Wno-unused-parameter "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
add_compile_options(
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>)
add_compile_definitions(USE_HAL_DRIVER STM32U575xx)
add_compile_definitions(WATCH_FIRMWARE_VERSION="${WATCH_FIRMWARE_VERSION}"
  WATCH_SOURCE_DATE_EPOCH=${WATCH_SOURCE_DATE_EPOCH})
set(HAL Drivers/STM32U5xx_HAL_Driver)
set(RTOS ThirdParty/FreeRTOS-Kernel)
set(PORT ${RTOS}/portable/GCC/ARM_CM33_NTZ/non_secure)
set(ASTRA ThirdParty/oled-ui-astra-0.0.2-alpha/Core/Src)
set(U8G2 ${ASTRA}/hal/hal_dreamCore/components/oled/graph_lib/u8g2)
set_source_files_properties(${PORT}/port.c PROPERTIES COMPILE_DEFINITIONS "SysTick_Handler=rtos_tick_handler")
include_directories(App BSP Platform UI Services Assets/fonts Config ${HAL}/Inc ${HAL}/Inc/Legacy Drivers/CMSIS/Include
  Drivers/CMSIS/Device/ST/STM32U5xx/Include ${RTOS}/include ${PORT} ${ASTRA} ${U8G2})
set(HAL_MODULES hal hal_adc hal_adc_ex hal_cortex hal_dma hal_dma_ex hal_exti hal_flash hal_flash_ex
  hal_gpio hal_i2c hal_i2c_ex hal_icache hal_pwr hal_pwr_ex hal_rcc hal_rcc_ex hal_rtc hal_rtc_ex
  hal_tim hal_tim_ex hal_uart hal_uart_ex)
foreach(M IN LISTS HAL_MODULES)
  list(APPEND HAL_SOURCES ${HAL}/Src/stm32u5xx_${M}.c)
endforeach()
file(GLOB U8G2_SOURCES CONFIGURE_DEPENDS ${U8G2}/*.c)
set(ASTRA_SOURCES ${ASTRA}/hal/hal.cpp ${ASTRA}/astra/ui/launcher.cpp ${ASTRA}/astra/ui/element/page/item.cpp)
file(GLOB APP_SOURCES CONFIGURE_DEPENDS App/*.cpp BSP/*.c Platform/*.cpp UI/*.cpp Services/*.cpp Assets/fonts/*.cpp)
add_executable(watch_astra ${APP_SOURCES} ${ASTRA_SOURCES} ${U8G2_SOURCES} ${HAL_SOURCES}
  Vendor/ST/system_stm32u5xx.c Vendor/ST/startup_stm32u575xx.s
  ${RTOS}/tasks.c ${RTOS}/queue.c ${RTOS}/list.c ${PORT}/port.c ${PORT}/portasm.c)
set_target_properties(watch_astra PROPERTIES SUFFIX .elf)
target_compile_options(watch_astra PRIVATE
  $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Debug>:-g3>
  $<$<CONFIG:Release>:-Os> $<$<CONFIG:Release>:-g1>)
target_link_options(watch_astra PRIVATE ${CPU} --specs=nano.specs --specs=nosys.specs
  -T${CMAKE_SOURCE_DIR}/cmake/stm32u575ri.ld -Wl,--gc-sections
  -Wl,-Map=watch_astra.map -Wl,--print-memory-usage)
target_link_libraries(watch_astra PRIVATE m)
set_property(TARGET watch_astra APPEND PROPERTY LINK_DEPENDS ${CMAKE_SOURCE_DIR}/cmake/stm32u575ri.ld)
add_custom_command(TARGET watch_astra POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex watch_astra.elf watch_astra.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary watch_astra.elf watch_astra.bin
  COMMAND ${CMAKE_SIZE} watch_astra.elf WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
