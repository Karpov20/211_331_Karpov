set(SOURCES
    main.cpp
    mainwindow.cpp
    crypto/qaesencryption.cpp
    security/securitymanager.cpp
)

set(HEADERS
    mainwindow.h
    crypto/qaesencryption.h
    security/securitymanager.h
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
