add_library(gps_parser STATIC
    ${CMAKE_CURRENT_LIST_DIR}/gps_parser.hpp
    ${CMAKE_CURRENT_LIST_DIR}/gps_parser.cpp
    )
target_link_libraries(gps_parser PRIVATE magic_enum range-v3::range-v3)
target_include_directories(gps_parser PUBLIC ${CMAKE_CURRENT_LIST_DIR})
