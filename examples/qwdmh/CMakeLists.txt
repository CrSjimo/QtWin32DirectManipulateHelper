project(QWDMHExample_QWDMH)

file(GLOB _src *.h *.cpp)

qwdmh_add_example(${PROJECT_NAME}
    FEATURES cxx_std_20
    SOURCES ${_src}
    QT_LINKS Core Gui Widgets
    LINKS QWDMHCore
)