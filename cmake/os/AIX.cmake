# Copyright (c) 2010, Oracle and/or its affiliates. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1335  USA 


#Enable 64 bit file offsets
SET(_LARGE_FILES 1)

IF(__AIX_COMPILER_XL)
# Fix xlC oddity - it complains about same inline function defined multiple times
# in different compilation units  
INCLUDE(CheckCXXCompilerFlag)
 CHECK_CXX_COMPILER_FLAG("-qstaticinline" HAVE_QSTATICINLINE)
 IF(HAVE_QSTATICINLINE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qstaticinline")
 ENDIF()
ENDIF()
