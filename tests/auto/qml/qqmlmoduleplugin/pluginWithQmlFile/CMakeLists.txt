# Generated from pluginWithQmlFile.pro.

#####################################################################
## pluginWithQmlFile Generic Library:
#####################################################################

qt_add_cmake_library(pluginWithQmlFile
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestPluginWithQmlFile"
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:pluginWithQmlFile.pro:<TRUE>:
# IMPORT_FILES = "qmldir" "MyQmlFile.qml"
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(pluginWithQmlFile)
# special case begin
file(COPY qmldir MyQmlFile.qml
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestPluginWithQmlFile"
)
# special case end
