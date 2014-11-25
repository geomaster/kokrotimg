include(GResource)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0) 

include_directories(${kokrotimg_INCLUDE_DIRS} ${GTKMM_INCLUDE_DIRS} )
message("${kokrotimg_INCLUDE_DIRS}")
link_directories(${GTKMM_LIBRARY_DIRS} )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

glib_compile_resources(GLIB_RESOURCES_kokrotviz
  source
    ui/kokrotviz.gresource.xml
)

add_executable(kokrotviz 
  ${GLIB_RESOURCES_kokrotviz}
  main.cc
  controlWindow.cc
  visualizationWindow.cc
  utils.cc
  kokrotImgDriver.cc
  layerManager.cc
  messageTreeView.cc
  metricsTreeView.cc
  layerTreeView.cc)

target_link_libraries(kokrotviz ${CMAKE_DL_LIBS} ${GTKMM_LIBRARIES})
