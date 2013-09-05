# FasTC
# Copyright (c) 2013 University of North Carolina at Chapel Hill.
# All rights reserved.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for educational, research, and non-profit purposes, without
# fee, and without a written agreement is hereby granted, provided that the
# above copyright notice, this paragraph, and the following four paragraphs
# appear in all copies.
#
# Permission to incorporate this software into commercial products may be
# obtained by contacting the authors or the Office of Technology Development
# at the University of North Carolina at Chapel Hill <otd@unc.edu>.
#
# This software program and documentation are copyrighted by the University of
# North Carolina at Chapel Hill. The software program and documentation are
# supplied "as is," without any accompanying services from the University of
# North Carolina at Chapel Hill or the authors. The University of North
# Carolina at Chapel Hill and the authors do not warrant that the operation of
# the program will be uninterrupted or error-free. The end-user understands
# that the program was developed for research purposes and is advised not to
# rely exclusively on the program for any reason.
#
# IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
# AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
# OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
# THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
# AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
#
# THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
# DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
# STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
# AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
# THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
# ENHANCEMENTS, OR MODIFICATIONS.
#
# Please send all BUG REPORTS to <pavel@cs.unc.edu>.
#
# The authors may be contacted via:
#
# Pavel Krajcevski
# Dept of Computer Science
# 201 S Columbia St
# Frederick P. Brooks, Jr. Computer Science Bldg
# Chapel Hill, NC 27599-3175
# USA
# 
# <http://gamma.cs.unc.edu/FasTC/>

# - Try to find libPVRTexLib
# Once done this will define
#  PVRTEXLIB_FOUND - System has PVRTexLib
#  PVRTEXLIB_INCLUDE_DIRS - The PVRTexLib include directories
#  PVRTEXLIB_LIBRARIES - The libraries needed to use PVRTexLib

IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  find_path(
    PVRTEXLIB_INCLUDE_DIR PVRTexture.h
    PATHS "/Applications/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library/Include"
  )

  find_library(PVRTEXLIB_LIB PVRTexLib
    PATHS "/Applications/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library/OSX_x86/Static"
          "/Applications/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library/OSX_x86/Dynamic"
  )
ELSEIF(MSVC)
  find_path(
    PVRTEXLIB_INCLUDE_DIR PVRTexture.h
    PATHS "C:/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library/Include"
  )

  IF(${CMAKE_GENERATOR} MATCHES Win64)
    find_library(PVRTEXLIB_LIB PVRTexLib
      PATHS "C:/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library/Windows_x86_64/Static"
            "C:/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library/Windows_x86_64/Dynamic"
    )
  ELSE()
    find_library(PVRTEXLIB_LIB PVRTexLib
      PATHS "C:/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library/Windows_x86_32/Static"
            "C:/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library/Windows_x86_32/Dynamic"
    )
  ENDIF()
ENDIF()

set(PVRTEXLIB_LIBRARIES ${PVRTEXLIB_LIB} )
set(PVRTEXLIB_INCLUDE_DIRS ${PVRTEXLIB_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PVRTexLib  DEFAULT_MSG
                                  PVRTEXLIB_LIB PVRTEXLIB_INCLUDE_DIR)

mark_as_advanced(PVRTEXLIB_INCLUDE_DIR PVRTEXLIB_LIB )