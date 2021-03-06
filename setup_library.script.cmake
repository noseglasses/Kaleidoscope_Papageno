#  -*- mode: cmake -*-
# Kaleidoscope-Papageno -- Papageno features for Kaleidoscope
# Copyright (C) 2017 noseglasses <shinynoseglasses@gmail.com>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(module_source_dir "")

if(NOT "${KALEIDOSCOPE_MODULE_SOURCE_DIR}" STREQUAL "")
   set(module_source_dir "${KALEIDOSCOPE_MODULE_SOURCE_DIR}")
else()
   set(module_source_dir "${CMAKE_SOURCE_DIR}")
endif()

set(papageno_library_directory "${module_source_dir}/../papageno")

# message("KALEIDOSCOPE_MODULE_SOURCE_DIR: ${KALEIDOSCOPE_MODULE_SOURCE_DIR}")
# message("papageno_library_directory: ${papageno_library_directory}")

if(NOT "${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
   message("Using toolchain file \"${CMAKE_TOOLCHAIN_FILE}\"")
   set(CMAKE_TOOLCHAIN_FILE_SPEC "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
elseif(NOT KALEIDOSCOPE_HOST_BUILD)
   set(toolchain_file "${module_source_dir}/3rd_party/Papageno/cmake/toolchains/Toolchain-avr-gcc.cmake")
   message("Using toolchain file \"${toolchain_file}\"")
   set(CMAKE_TOOLCHAIN_FILE_SPEC "-DCMAKE_TOOLCHAIN_FILE=${toolchain_file}")
endif()

if(NOT "${PAPAGENO_PLATFORM}" STREQUAL "")
   message("Building papageno for platform ${PAPAGENO_PLATFORM}")
   set(PAPAGENO_PLATFORM_SPEC "-DPAPAGENO_PLATFORM=${PAPAGENO_PLATFORM}")
elseif(NOT KALEIDOSCOPE_HOST_BUILD)
   message("Building papageno for platform atmega32u4")
   set(PAPAGENO_PLATFORM_SPEC "-DPAPAGENO_PLATFORM=atmega32u4")
else()
   message("Virtual build => Building papageno for platform generic")
   set(PAPAGENO_PLATFORM_SPEC "-DPAPAGENO_PLATFORM=generic")
endif()

if(NOT EXISTS "${papageno_library_directory}")
   
   file(MAKE_DIRECTORY "${papageno_library_directory}")

   # Configure build of a Papageno Arduino library.
   #
   execute_process(
      COMMAND "${CMAKE_COMMAND}" 
         "-DPAPAGENO_ARDUINO_BUILD_DIR=${papageno_library_directory}"
         "${CMAKE_TOOLCHAIN_FILE_SPEC}"
         "${PAPAGENO_PLATFORM_SPEC}"
         "-DPAPAGENO_BUILD_GLOCKENSPIEL=FALSE"
         "${module_source_dir}/3rd_party/Papageno"
      WORKING_DIRECTORY "${papageno_library_directory}"
   )
   
   # Build a Papageno Arduino library.
   #
   execute_process(
      COMMAND "${CMAKE_COMMAND}" --build "${papageno_library_directory}" --target arduino_symlinks
   )
   
   if(NOT EXISTS "${papageno_library_directory}/src/papageno.h")
      message(FATAL_ERROR "Failed setting up papageno")
   endif()
   
endif()
