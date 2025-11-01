set(SOURCES
    main.cpp
    mainwindow.cpp
    crypto/qaesencryption.cpp
)

set(HEADERS
    mainwindow.h
    crypto/qaesencryption.h
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
)
