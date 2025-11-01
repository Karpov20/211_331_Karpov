set(SOURCES
    main.cpp
    mainwindow.cpp
    crypto/qaesencryption.cpp
<<<<<<< Updated upstream
=======
    security/securitymanager.cpp
>>>>>>> Stashed changes
)

set(HEADERS
    mainwindow.h
    crypto/qaesencryption.h
<<<<<<< Updated upstream
=======
    security/securitymanager.h
>>>>>>> Stashed changes
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
