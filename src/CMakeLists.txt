# Add library targets
#####################
SET(SOURCES flow_device_manager.c fdm_register.c fdm_subscribe.c fdm_licensee_verification.c
	fdm_hmac.c fdm_server_session.c fdm_get_client_list.c fdm_provision_constrained.c)
ADD_LIBRARY(flowdevicemanager SHARED ${SOURCES})

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(JSON ${STRICT_CHECK} json-c)
INCLUDE_DIRECTORIES(${JSON_INCLUDE_DIRS})

FIND_LIBRARY(LIB_AWA libawa.so PATHS ${STAGING_DIR}/usr/lib)
TARGET_LINK_LIBRARIES(flowdevicemanager ${LIB_AWA} json-c blobmsg_json)

# Add executable targets
########################
ADD_EXECUTABLE(flow_device_manager_ubusd flow_device_manager_ubus.c)
TARGET_LINK_LIBRARIES(flow_device_manager_ubusd flowdevicemanager ubus ubox json-c blobmsg_json)

# Add install targets
######################
INSTALL(TARGETS flowdevicemanager LIBRARY DESTINATION lib)
INSTALL(TARGETS flow_device_manager_ubusd RUNTIME DESTINATION bin)
