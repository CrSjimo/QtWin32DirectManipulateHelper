set(QWDMH_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR})

macro(qwdmh_add_example _target)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(${_target})
    qm_configure_target(${_target} ${ARGN})
endmacro()

add_subdirectory(qwdmh)