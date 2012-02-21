set(URBI_ROOT_DIR "/usr" CACHE PATH "Urbi root directory.")

find_package (OpenCV REQUIRED)
find_package (Urbi REQUIRED)
find_package (PkgConfig REQUIRED)
find_package (Boost REQUIRED thread)

#pkg_check_modules (facet REQUIRED facet)

link_directories (${BOOST_LIBRARYDIR} ${facet_LIBRARY_DIRS})

include_directories (${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${URBI_INCLUDE_DIRS} ${facet_INCLUDE_DIRS})


if (WIN32)
  add_definitions( -DBOOST_ALL_DYN_LINK )
endif (WIN32)

add_library (ucamera SHARED urbicamera.cpp)
add_library (ucolordetector SHARED urbicolordetector.cpp)
add_library (uobjectdetector SHARED urbiobjectdetector.cpp)
add_library (umovedetector SHARED urbimovedetector.cpp)
#add_library (ufacet SHARED urbifacet.cpp)

target_link_libraries (ucamera ${OpenCV_LIBS} ${URBI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries (ucolordetector ${OpenCV_LIBS} ${URBI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries (uobjectdetector ${OpenCV_LIBS} ${URBI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries (umovedetector ${OpenCV_LIBS} ${URBI_LIBRARIES} ${Boost_LIBRARIES})
#target_link_libraries (ufacet ${OpenCV_LIBS} ${URBI_LIBRARIES} ${facet_LIBRARIES})

set_target_properties (ucamera PROPERTIES
  VERSION 0.0.1
  SOVERSION 0.0.1)

set_target_properties (ucolordetector PROPERTIES
  VERSION 0.0.1
  SOVERSION 0.0.1)

set_target_properties (uobjectdetector PROPERTIES
  VERSION 0.0.1
  SOVERSION 0.0.1)

set_target_properties (umovedetector PROPERTIES
  VERSION 0.0.1
  SOVERSION 0.0.1)
  
#set_target_properties (ufacet PROPERTIES
#  VERSION 0.0.1
#  SOVERSION 0.0.1)
  
install (TARGETS ucamera ucolordetector uobjectdetector umovedetector DESTINATION lib/gostai/uobjects COMPONENT libraries)
