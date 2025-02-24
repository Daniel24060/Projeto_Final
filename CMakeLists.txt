# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
include(pico_sdk_import.cmake)

project(neopixel_pio C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable
add_executable(neopixel_pio neopixel_pio.c inc/ssd1306_i2c.c)

pico_set_program_name(neopixel_pio "neopixel_pio")
pico_set_program_version(neopixel_pio "0.1")

# Generate PIO header
pico_generate_pio_header(neopixel_pio ${CMAKE_CURRENT_LIST_DIR}/ws2818b.pio)

# Habilitar UART e USB
pico_enable_stdio_uart(neopixel_pio 1)  # Habilitar UART
pico_enable_stdio_usb(neopixel_pio 1)

# Adicionar bibliotecas padrão e I2C
target_link_libraries(neopixel_pio
        pico_stdlib
        hardware_pio
        hardware_clocks
        hardware_i2c
        hardware_pwm
        hardware_adc)  # Adicionando I2C

# Adicionar diretórios de cabeçalhos
target_include_directories(neopixel_pio PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/inc  # Exemplo de diretório para biblioteca do display
)

# Adicionar saídas extras
pico_add_extra_outputs(neopixel_pio)
add_compile_options(-fmax-include-depth=300)
