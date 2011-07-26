set(URBI_ROOT_DIR "/usr" CACHE PATH "Urbi root directory.")

find_package (OpenCV REQUIRED)
find_package (Urbi REQUIRED)

find_package (Boost REQUIRED)

include_directories (${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${URBI_INCLUDE_DIRS})

add_library (urbicamera SHARED urbicamera.cpp)
add_library (urbicolordetector SHARED urbicolordetector.cpp)
add_library (urbiobjectdetector SHARED urbiobjectdetector.cpp)

target_link_libraries (urbicamera ${OpenCV_LIBS} ${URBI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries (urbicolordetector ${OpenCV_LIBS} ${URBI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries (urbiobjectdetector ${OpenCV_LIBS} ${URBI_LIBRARIES} ${Boost_LIBRARIES})

set_target_properties (urbicamera PROPERTIES
  VERSION 0.0.1
  SOVERSION 0.0.1)

set_target_properties (urbicolordetector PROPERTIES
  VERSION 0.0.1
  SOVERSION 0.0.1)

set_target_properties (urbiobjectdetector PROPERTIES
  VERSION 0.0.1
  SOVERSION 0.0.1)
  
install (TARGETS urbicamera urbicolordetector urbiobjectdetector DESTINATION lib COMPONENT libraries)
