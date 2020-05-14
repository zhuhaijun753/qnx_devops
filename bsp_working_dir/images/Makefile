ROOT_DIR := $(notdir $(CURDIR))
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)


SRC := $(CURDIR)/../src
BOARD_ROOT := $(SRC)/hardware/startup/boards
HOST_MKIFS := mkifs

HOST_MKIFS := mkifs

MKIFSFLAGS := -s QNX_usage -s QNX_info
SUFFIXES := .build .ifs .raw

.PHONY: all clean

all: ifs-s32g-evb.ui

clean:
	$(RM_HOST) ifs-s32g-evb.ui
	$(RM_HOST) ifs-s32g-evb.raw
	$(RM_HOST) qnx-ifs


ifs-s32g-evb.ui: evb.build
	$(HOST_MKIFS) -vvv -r../install $(MKIFSFLAGS) $^ ifs-s32g-evb.raw

	mki -a 0x80080000 -A arm64 ifs-s32g-evb.raw ifs-s32g-evb.ui
	cp ifs-s32g-evb.raw qnx-ifs
	rm ifs-s32g-evb.raw

