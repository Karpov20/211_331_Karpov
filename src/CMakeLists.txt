set(APP_SOURCES
    main.cpp
    mainwindow.cpp
    crypto/qaesencryption.cpp
    security/securitymanager.cpp
)

set(APP_HEADERS
    mainwindow.h
    crypto/qaesencryption.h
    security/securitymanager.h
)

add_executable(${PROJECT_NAME}
    ${APP_SOURCES}
    ${APP_HEADERS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(transactions_tool
    datagen.cpp
    crypto/qaesencryption.cpp
    crypto/qaesencryption.h
)

target_link_libraries(transactions_tool PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
)

target_include_directories(transactions_tool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
