# Makefile for use with GNU make
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

include options.mk

# common vars
include vars.mk

all: $(TGTS)

include FICS/build.mk

# common rules
include common.mk

.PHONY: clean install-init install tidy

include $(TARGETS_DIR)clean.mk
include $(TARGETS_DIR)install.mk
include $(TARGETS_DIR)tidy.mk
