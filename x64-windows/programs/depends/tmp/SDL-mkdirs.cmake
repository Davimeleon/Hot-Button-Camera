# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/David Backer Peral/Downloads/Seek_Thermal_SDK_4.4.2.20/Hot-Button-Camera/x64-windows/programs/depends/src/SDL"
  "C:/Users/David Backer Peral/Downloads/Seek_Thermal_SDK_4.4.2.20/Hot-Button-Camera/x64-windows/programs/depends/src/SDL-build"
  "C:/Users/David Backer Peral/Downloads/Seek_Thermal_SDK_4.4.2.20/Hot-Button-Camera/x64-windows/programs/depends"
  "C:/Users/David Backer Peral/Downloads/Seek_Thermal_SDK_4.4.2.20/Hot-Button-Camera/x64-windows/programs/depends/tmp"
  "C:/Users/David Backer Peral/Downloads/Seek_Thermal_SDK_4.4.2.20/Hot-Button-Camera/x64-windows/programs/depends/src/SDL-stamp"
  "C:/Users/David Backer Peral/Downloads/Seek_Thermal_SDK_4.4.2.20/Hot-Button-Camera/x64-windows/programs/depends/src"
  "C:/Users/David Backer Peral/Downloads/Seek_Thermal_SDK_4.4.2.20/Hot-Button-Camera/x64-windows/programs/depends/src/SDL-stamp"
)

set(configSubDirs Debug;Release;MinSizeRel;RelWithDebInfo)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/David Backer Peral/Downloads/Seek_Thermal_SDK_4.4.2.20/Hot-Button-Camera/x64-windows/programs/depends/src/SDL-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/David Backer Peral/Downloads/Seek_Thermal_SDK_4.4.2.20/Hot-Button-Camera/x64-windows/programs/depends/src/SDL-stamp${cfgdir}") # cfgdir has leading slash
endif()
