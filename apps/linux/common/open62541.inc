# Common Makefile definitions for applications using open62541 (opcua) library

OPCUA_INCLUDES= -I$(STAGING_DIR)/usr/include/open62541
OPCUA_LIBS= -lopen62541 -L$(STAGING_DIR)/usr/lib

OPCUA_CFLAGS=$(OPCUA_LIBS) $(OPCUA_INCLUDES)
