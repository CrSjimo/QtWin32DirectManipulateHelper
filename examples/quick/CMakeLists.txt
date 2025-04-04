project(QWDMHExample_Quick)

file(GLOB _src *.h *.cpp)

qwdmh_add_example(${PROJECT_NAME}
    FEATURES cxx_std_20
    SOURCES ${_src}
    QT_LINKS Core Gui Widgets Qml Quick
    LINKS QWDMHCore
)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

qt_add_qml_module(${PROJECT_NAME}
    URI QtWin32DirectManipulateHelper.Examples.Quick
    QML_FILES main.qml
)