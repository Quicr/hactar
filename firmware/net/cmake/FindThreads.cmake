# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindThreads
-----------

This module determines the thread library of the system.

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.1

This module defines the following :prop_tgt:`IMPORTED` target:

``Threads::Threads``
  The thread library, if found.

Result Variables
^^^^^^^^^^^^^^^^

The following variables are set:

``Threads_FOUND``
  If a supported thread library was found.
``CMAKE_THREAD_LIBS_INIT``
  The thread library to use. This may be empty if the thread functions
  are provided by the system libraries and no special flags are needed
  to use them.
``CMAKE_USE_WIN32_THREADS_INIT``
  If the found thread library is the win32 one.
``CMAKE_USE_PTHREADS_INIT``
  If the found thread library is pthread compatible.
``CMAKE_HP_PTHREADS_INIT``
  If the found thread library is the HP thread library.

Variables Affecting Behavior
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. variable:: THREADS_PREFER_PTHREAD_FLAG

  .. versionadded:: 3.1

  If the use of the -pthread compiler and linker flag is preferred then
  the caller can set this variable to TRUE. The compiler flag can only be
  used with the imported target. Use of both the imported target as well
  as this switch is highly recommended for new code.

  This variable has no effect if the system libraries provide the
  thread functions, i.e. when ``CMAKE_THREAD_LIBS_INIT`` will be empty.
#]=======================================================================]

# message(FATAL_ERROR "IDF-PTHREADS is being used")

set(Threads_FOUND TRUE)

set(IDF_PTHREADS ${IDF_PATH}/components/pthread/include)
add_library(Threads::Threads INTERFACE IMPORTED)

if(THREADS_HAVE_PTHREAD_ARG)
  set_property(TARGET Threads::Threads
                PROPERTY INTERFACE_COMPILE_OPTIONS "$<$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>:SHELL:-Xcompiler -pthread>"
                                                  "$<$<AND:$<NOT:$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>>,$<NOT:$<COMPILE_LANGUAGE:Swift>>>:-pthread>")
endif()

if(CMAKE_THREAD_LIBS_INIT)
  set_property(TARGET Threads::Threads PROPERTY INTERFACE_LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
endif()
