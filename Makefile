# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = pantheon.elf
EE_OBJS = pantheon.o
EE_LIBS = -ldraw -lgraph -lmath3d -lpacket -ldma -lkernel
PYTHON ?= python3
HRC_FILES ?= skydome.hrc
GENERATED_HEADERS := $(HRC_FILES:.hrc=_data.h)

all: assets $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)

assets: $(GENERATED_HEADERS)

# Kojima blue rim->zenith gradient (must match floor.c apply_sky_color + hrc2ps2 skydome branch)
skydome_data.h: skydome.hrc hrc2ps2.py
	$(PYTHON) hrc2ps2.py "$<" -o "$@" --color-mode gradient

clean:
	rm -f $(EE_BIN) $(EE_OBJS) $(GENERATED_HEADERS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
