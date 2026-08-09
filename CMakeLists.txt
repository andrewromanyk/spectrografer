cmake_minimum_required(VERSION 4.2)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("spectografer")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(raylib)
find_package(FFTW3)
find_package(SndFile)

add_subdirectory ("fftw")
add_subdirectory ("main")