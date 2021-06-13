
include_directories(
    ../../include
    paho.mqtt.c/src)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  datastream_mqtt.ui  )

SET( SRC datastream_mqtt.cpp )

add_library(DataStreamMQTT SHARED ${SRC} ${UI_SRC}  )

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(DataStreamMQTT
        ${Qt5Widgets_LIBRARIES} plotjuggler_plugin_base
        paho-mqtt3c
        paho-mqtt3a  )
else()
    target_link_libraries(DataStreamMQTT
        ${Qt5Widgets_LIBRARIES} plotjuggler_plugin_base
        paho-mqtt3c-static
        paho-mqtt3a-static)
endif()

install(TARGETS DataStreamMQTT DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )




