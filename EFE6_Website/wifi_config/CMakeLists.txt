# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(WifiConfig)


set( WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/site")

add_custom_command( TARGET app
    PRE_BUILD
    WORKING_DIRECTORY ${WEB_SRC_DIR}
    COMMAND /usr/bin/npm run build
    VERBATIM
)

littlefs_create_partition_image( image ${WEB_SRC_DIR}/build FLASH_IN_PROJECT DEPENDS app )
