# Common Makefile definitions for applications using gstreamer libraries 

GST_INCLUDES=-I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/usr/include/gstreamer-1.0 -I$(STAGING_DIR)/usr/lib/gstreamer-1.0/include -I$(STAGING_DIR)/usr/include/glib-2.0 -I$(STAGING_DIR)/usr/lib/glib-2.0/include
GST_LIBS= -lglib-2.0 -lgobject-2.0 -lpcre -lffi -lgthread-2.0 -lgmodule-2.0 -lgstreamer-1.0 -lgstapp-1.0 -lgstbase-1.0 -L$(STAGING_DIR)/usr/lib

GST_CFLAGS=$(GST_LIBS) $(GST_INCLUDES)
