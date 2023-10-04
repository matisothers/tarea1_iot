# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/mati/esp/esp-idf/components/bootloader/subproject"
  "/Users/mati/Desktop/tarea1_iot/codigo_esp/build/bootloader"
  "/Users/mati/Desktop/tarea1_iot/codigo_esp/build/bootloader-prefix"
  "/Users/mati/Desktop/tarea1_iot/codigo_esp/build/bootloader-prefix/tmp"
  "/Users/mati/Desktop/tarea1_iot/codigo_esp/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/mati/Desktop/tarea1_iot/codigo_esp/build/bootloader-prefix/src"
  "/Users/mati/Desktop/tarea1_iot/codigo_esp/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/mati/Desktop/tarea1_iot/codigo_esp/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/mati/Desktop/tarea1_iot/codigo_esp/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
