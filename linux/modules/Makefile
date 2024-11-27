ifeq ($(config),)
$(error config variable is not set)
endif

ifeq ($(target),)
$(error target variable is not set)
endif

CUR_DIR := $(shell pwd)
MODULES_OBJ_DIR ?= $(CUR_DIR)/build/$(target)/$(config)
GENAVB_ROOT_DIR ?= $(CUR_DIR)/../..
GENAVB_INCLUDE ?= $(GENAVB_ROOT_DIR)/include
PREFIX ?= $(MODULES_OBJ_DIR)/target

MODULES_SRC := $(GENAVB_ROOT_DIR)/linux/modules/common/*
MODULES_SRC += $(GENAVB_ROOT_DIR)/linux/modules/std/*
MODULES_SRC += $(GENAVB_ROOT_DIR)/linux/modules/avb/*
MODULES_SRC += $(GENAVB_ROOT_DIR)/common/os/*
MODULES_SRC += $(GENAVB_ROOT_DIR)/public/sr_class.c
MODULES_SRC += $(GENAVB_ROOT_DIR)/public/qos.c

all: modules

$(MODULES_OBJ_DIR)/modules.stamp: $(MODULES_SRC)
	@echo Generate modules sources in $(MODULES_OBJ_DIR)
	@mkdir -p $(MODULES_OBJ_DIR)
	@cp -a $(MODULES_SRC) $(MODULES_OBJ_DIR)
	@cp -a Makefile.modules $(MODULES_OBJ_DIR)/Makefile
	@touch $@

modules_sources: $(MODULES_OBJ_DIR)/modules.stamp

install modules_install: modules

modules modules_install install clean: modules_sources
	$(MAKE) -C $(MODULES_OBJ_DIR) $@ MAKEFLAGS= GENAVB_INCLUDE=$(GENAVB_INCLUDE) PREFIX=$(PREFIX)

.PHONY: modules modules_sources modules_install install clean
