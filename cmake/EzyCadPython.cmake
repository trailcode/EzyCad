# EzyCad — resolve CPython for embedding (especially Windows / MSVC + multi-config).
#
# Why this exists:
# - FindPython3 can leave Python3_LIBRARY_RELEASE empty while Python3_FOUND is true.
# - Python3::Python may map Debug builds to python3xx_d.lib (often missing).
# - On Windows we resolve libs\python*.lib from the selected interpreter and use the release import library.

macro(ezycad_configure_embedded_python)
  set(EZYCAD_HAVE_PYTHON FALSE)
  set(EZYCAD_PYTHON_LINK_MODE "")
  set(EZYCAD_PYTHON_RUNTIME_DLL "")

  if(Python3_FOUND)
    if(WIN32)
      set(_ezycad_py_implib "")
      if(DEFINED Python3_LIBRARY_RELEASE AND EXISTS "${Python3_LIBRARY_RELEASE}")
        set(_ezycad_py_implib "${Python3_LIBRARY_RELEASE}")
      endif()

      if(_ezycad_py_implib STREQUAL "" AND DEFINED Python3_LIBRARIES)
        foreach(_l IN LISTS Python3_LIBRARIES)
          if(EXISTS "${_l}")
            string(TOLOWER "${_l}" _l_lower)
            if(_l_lower MATCHES "\\.lib$" AND NOT _l_lower MATCHES "_d\\.lib$")
              set(_ezycad_py_implib "${_l}")
              break()
            endif()
          endif()
        endforeach()
      endif()

      if(_ezycad_py_implib STREQUAL "" AND Python3_EXECUTABLE)
        execute_process(
          COMMAND "${Python3_EXECUTABLE}" -c
            "import glob,os,sys;p=os.path.join(sys.base_prefix,'libs');c=[x for x in glob.glob(os.path.join(p,'python*.lib')) if not x.lower().endswith('_d.lib')];c.sort(key=len,reverse=True);print(c[0] if c else '',end='')"
          OUTPUT_VARIABLE _ezycad_py_implib
          OUTPUT_STRIP_TRAILING_WHITESPACE
          RESULT_VARIABLE _ezycad_py_implib_rc
          ERROR_QUIET
        )
        if(NOT _ezycad_py_implib_rc EQUAL 0)
          set(_ezycad_py_implib "")
        endif()
      endif()

      set(_ezycad_py_include "")
      if(DEFINED Python3_INCLUDE_DIRS)
        list(LENGTH Python3_INCLUDE_DIRS _ezycad_py_inc_n)
        if(_ezycad_py_inc_n GREATER 0)
          list(GET Python3_INCLUDE_DIRS 0 _ezycad_py_include)
        endif()
      endif()
      if(_ezycad_py_include STREQUAL "" AND Python3_EXECUTABLE)
        execute_process(
          COMMAND "${Python3_EXECUTABLE}" -c "import os,sys;print(os.path.join(sys.base_prefix,'include'),end='')"
          OUTPUT_VARIABLE _ezycad_py_include
          OUTPUT_STRIP_TRAILING_WHITESPACE
          RESULT_VARIABLE _ezycad_py_inc_rc
          ERROR_QUIET
        )
        if(NOT _ezycad_py_inc_rc EQUAL 0)
          set(_ezycad_py_include "")
        endif()
      endif()

      set(_ezycad_py_dll "")
      if(DEFINED Python3_RUNTIME_LIBRARY_RELEASE AND EXISTS "${Python3_RUNTIME_LIBRARY_RELEASE}")
        set(_ezycad_py_dll "${Python3_RUNTIME_LIBRARY_RELEASE}")
      endif()
      if((_ezycad_py_dll STREQUAL "" OR NOT EXISTS "${_ezycad_py_dll}") AND Python3_EXECUTABLE)
        execute_process(
          COMMAND "${Python3_EXECUTABLE}" -c
            "import glob,os,sys;p=sys.base_prefix;c=sorted(glob.glob(os.path.join(p,'python3*.dll')));r=[x for x in c if not x.lower().endswith('_d.dll')];print((r[0] if r else (c[0] if c else '')),end='')"
          OUTPUT_VARIABLE _ezycad_py_dll
          OUTPUT_STRIP_TRAILING_WHITESPACE
          RESULT_VARIABLE _ezycad_py_dll_rc
          ERROR_QUIET
        )
        if(NOT _ezycad_py_dll_rc EQUAL 0)
          set(_ezycad_py_dll "")
        endif()
      endif()

      set(_ezycad_py_h "${_ezycad_py_include}/Python.h")
      if(_ezycad_py_implib AND EXISTS "${_ezycad_py_implib}" AND EXISTS "${_ezycad_py_h}")
        if(NOT TARGET ezycad_embed_python)
          add_library(ezycad_embed_python SHARED IMPORTED GLOBAL)
        endif()
        set_target_properties(ezycad_embed_python PROPERTIES
          IMPORTED_IMPLIB_DEBUG "${_ezycad_py_implib}"
          IMPORTED_IMPLIB_RELEASE "${_ezycad_py_implib}"
          IMPORTED_IMPLIB_RELWITHDEBINFO "${_ezycad_py_implib}"
          IMPORTED_IMPLIB_MINSIZEREL "${_ezycad_py_implib}"
          INTERFACE_INCLUDE_DIRECTORIES "${_ezycad_py_include}"
        )
        if(_ezycad_py_dll AND EXISTS "${_ezycad_py_dll}")
          set_target_properties(ezycad_embed_python PROPERTIES
            IMPORTED_LOCATION_DEBUG "${_ezycad_py_dll}"
            IMPORTED_LOCATION_RELEASE "${_ezycad_py_dll}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${_ezycad_py_dll}"
            IMPORTED_LOCATION_MINSIZEREL "${_ezycad_py_dll}"
          )
          set(EZYCAD_PYTHON_RUNTIME_DLL "${_ezycad_py_dll}")
        endif()
        set(EZYCAD_HAVE_PYTHON TRUE)
        set(EZYCAD_PYTHON_LINK_MODE "imported_target")
      else()
        message(STATUS "Python console: disabled (Windows: failed to resolve embed paths).")
      endif()
    else()
      set(EZYCAD_HAVE_PYTHON TRUE)
      set(EZYCAD_PYTHON_LINK_MODE "python3_target")
    endif()
  endif()
endmacro()

