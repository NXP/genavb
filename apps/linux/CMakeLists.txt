set(apps management-app)

if(CONFIG_SOCKET)
  list(APPEND apps tsn-app)
endif()

if(CONFIG_SRP)
  list(APPEND apps msrp-ctrl-app)
endif()

if(CONFIG_MAAP)
  list(APPEND apps maap-ctrl-app)
endif()

if(CONFIG_AVDECC)
  list(APPEND apps genavb-controls-app genavb-controller-app aem-manager)
endif()

if(CONFIG_AVTP)
  list(APPEND apps simple-audio-app alsa-audio-app genavb-multi-stream-app genavb-video-player-app genavb-video-server-app genavb-media-app salsacamctrl simple-acf-app)
endif()

set(APPS_INSTALL_DIR ${CMAKE_BINARY_DIR}/apps/target)

foreach(app IN LISTS apps)
  set(app_source_dir ${CMAKE_SOURCE_DIR}/apps/linux/${app})
  set(app_build_dir ${CMAKE_BINARY_DIR}/apps/build/${app})

  # run cmake
  add_custom_command(
    OUTPUT ${app_build_dir}
    COMMAND ${CMAKE_COMMAND}
    ${app_source_dir}
    -B${app_build_dir}
    -DGENAVB_INCLUDE_DIR=${CMAKE_BINARY_DIR}/include # target includes prepared by include-prep
    -DGENAVB_LIB_DIR=${CMAKE_BINARY_DIR}/lib # libgenavb.so location
    -DCMAKE_INSTALL_PREFIX=${APPS_INSTALL_DIR}
    -DCMAKE_INSTALL_MESSAGE=LAZY
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    COMMENT "Generate ${app}"
    )

  # build app
  add_custom_target(${app}
    COMMAND $(MAKE) -C ${app_build_dir} DESTDIR="" install
    DEPENDS ${app_build_dir} ${genavb} include-prep
    COMMENT "Build ${app}"
    )
endforeach()

add_custom_target(apps ALL DEPENDS ${apps})

# install apps binaries/scripts/outputs into main target directory
install(DIRECTORY ${APPS_INSTALL_DIR}/usr/bin/ DESTINATION usr/bin USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${APPS_INSTALL_DIR}/etc/ DESTINATION etc USE_SOURCE_PERMISSIONS OPTIONAL)

if(${CMAKE_VERSION} GREATER_EQUAL "3.15.0")
  set_property(TARGET apps APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${APPS_INSTALL_DIR})
else()
  set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${APPS_INSTALL_DIR})
endif()
