#############################################
##                                         ##
##    Copyright (C) 2019-2021 Julian Uy    ##
##  https://sites.google.com/site/awertyb  ##
##                                         ##
##   See details of license at "LICENSE"   ##
##                                         ##
#############################################

TOOL_TRIPLET_PREFIX ?= i686-w64-mingw32-
CC := $(TOOL_TRIPLET_PREFIX)gcc
CXX := $(TOOL_TRIPLET_PREFIX)g++
AR := $(TOOL_TRIPLET_PREFIX)ar
WINDRES := $(TOOL_TRIPLET_PREFIX)windres
STRIP := $(TOOL_TRIPLET_PREFIX)strip
GIT_TAG := $(shell git describe --abbrev=0 --tags)
INCFLAGS += -I. -I.. -Iexternal/jxrlib -Iexternal/jxrlib/common/include -Iexternal/jxrlib/image/sys -Iexternal/jxrlib/jxrgluelib
ALLSRCFLAGS += $(INCFLAGS) -DGIT_TAG=\"$(GIT_TAG)\"
CFLAGS += -O3 -flto
CFLAGS += $(ALLSRCFLAGS) -Wall -Wno-unused-value -Wno-format -DNDEBUG -DWIN32 -D_WIN32 -D_WINDOWS 
CFLAGS += -D_USRDLL -DUNICODE -D_UNICODE 
CFLAGS += -DDISABLE_PERF_MEASUREMENT
CXXFLAGS += $(CFLAGS) -fpermissive
WINDRESFLAGS += $(ALLSRCFLAGS) --codepage=65001
LDFLAGS += -static -static-libgcc -shared -Wl,--add-stdcall-alias
LDLIBS +=

%.o: %.c
	@printf '\t%s %s\n' CC $<
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp
	@printf '\t%s %s\n' CXX $<
	$(CXX) -c $(CXXFLAGS) -o $@ $<

%.o: %.rc
	@printf '\t%s %s\n' WINDRES $<
	$(WINDRES) $(WINDRESFLAGS) $< $@

JXRLIB_SOURCES += external/jxrlib/image/decode/JXRTranscode.c external/jxrlib/image/decode/decode.c external/jxrlib/image/decode/postprocess.c external/jxrlib/image/decode/segdec.c external/jxrlib/image/decode/strInvTransform.c external/jxrlib/image/decode/strPredQuantDec.c external/jxrlib/image/decode/strdec.c external/jxrlib/image/encode/encode.c external/jxrlib/image/encode/segenc.c external/jxrlib/image/encode/strFwdTransform.c external/jxrlib/image/encode/strPredQuantEnc.c external/jxrlib/image/encode/strenc.c external/jxrlib/image/sys/adapthuff.c external/jxrlib/image/sys/image.c external/jxrlib/image/sys/perfTimerANSI.c external/jxrlib/image/sys/strPredQuant.c external/jxrlib/image/sys/strTransform.c external/jxrlib/image/sys/strcodec.c external/jxrlib/jxrgluelib/JXRGlue.c external/jxrlib/jxrgluelib/JXRGlueJxr.c external/jxrlib/jxrgluelib/JXRGluePFC.c external/jxrlib/jxrgluelib/JXRMeta.c
SOURCES := extractor.c $(JXRLIB_SOURCES)
OBJECTS := $(SOURCES:.c=.o)
OBJECTS := $(OBJECTS:.cpp=.o)
OBJECTS := $(OBJECTS:.rc=.o)

BINARY ?= ifjxr_unstripped.spi
BINARY_STRIPPED ?= ifjxr.spi
ARCHIVE ?= ifjxr.7z

all: $(BINARY_STRIPPED)

archive: $(ARCHIVE)

clean:
	rm -f $(OBJECTS) $(BINARY) $(BINARY_STRIPPED) $(ARCHIVE)

$(ARCHIVE): $(BINARY_STRIPPED)
	rm -f $(ARCHIVE)
	7z a $@ $^

$(BINARY_STRIPPED): $(BINARY)
	@printf '\t%s %s\n' STRIP $@
	$(STRIP) -o $@ $^

$(BINARY): $(OBJECTS) 
	@printf '\t%s %s\n' LNK $@
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
