include_directories(../)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  datastream_lsl.ui  )

SET( SRC datastream_lsl.cpp streamer.cpp )

add_library(DataStreamLSL SHARED ${SRC} ${UI_SRC}  )

target_link_libraries(DataStreamLSL
    ${Qt5Widgets_LIBRARIES}
    plotjuggler_plugin_base
    lsl-static
    lslboost
    )

install(TARGETS DataStreamLSL DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

