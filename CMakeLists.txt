#
#   This file is a part of Tiny-Shading-Language or TSL, an open-source cross
#   platform programming shading language.
#
#   Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.
#
#   TSL is a free software written for educational purpose. Anyone can distribute
#   or modify it under the the terms of the GNU General Public License Version 3 as
#   published by the Free Software Foundation. However, there is NO warranty that
#   all components are functional in a perfect manner. Without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along with
#   this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
#

cmake_minimum_required (VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# define platform
if (WIN32)
    set(TSL_PLATFORM_WIN true)
    set(TSL_PLATFORM_MAC false)
    set(TSL_PLATFORM_LINUX false)
    set(TSL_PLATFORM_NAME "Windows" )
elseif(APPLE)
    set(TSL_PLATFORM_WIN false)
    set(TSL_PLATFORM_MAC true)
    set(TSL_PLATFORM_LINUX false)
    set(TSL_PLATFORM_NAME "Mac OS" )
elseif(UNIX)
    set(TSL_PLATFORM_WIN false)
    set(TSL_PLATFORM_MAC false)
    set(TSL_PLATFORM_LINUX true)
    set(TSL_PLATFORM_NAME "Linux" )
endif()

project (TSL)

macro(source_group_by_dir source_files)
    set(sgbd_cur_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
    foreach(sgbd_file ${${source_files}})
        string(REGEX REPLACE ${sgbd_cur_dir}/\(.*\) \\1 sgbd_fpath ${sgbd_file})
        string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
        string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
        string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
        if(sgbd_nogroup)
            set(sgbd_group_name "\\")
        endif(sgbd_nogroup)
        source_group(${sgbd_group_name} FILES ${sgbd_file})
    endforeach(sgbd_file)
endmacro(source_group_by_dir)

file(GLOB_RECURSE project_cpps src/*.cpp)
file(GLOB_RECURSE project_cs src/*.c)
file(GLOB_RECURSE project_ccs src/*.cc)

set(all_files ${project_headers} ${project_cpps} ${project_cs} ${project_ccs})
source_group_by_dir(all_files)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${TSL_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TSL_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TSL_SOURCE_DIR}/bin")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244" )

# this is to avoid compilation issue in the file generated by flex
add_definitions( -DYY_NO_UNISTD_H )

add_executable(TSL ${all_files})