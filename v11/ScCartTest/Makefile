#
# Cartridge Test Utility
# by cafe-alpha.
# WWW: http://ppcenter.free.fr/pskai/
#
# See LICENSE file for details.
#

# Contains project compilation settings
include mk_config.inc

CC	=	$(CONF_VERBOSE)$(CONF_SH_ELF)gcc
LD	=	$(CONF_VERBOSE)$(CONF_SH_ELF)gcc
AS	=	$(CONF_VERBOSE)$(CONF_SH_ELF)as
LDFLAGS =	$(CONF_LDFLAGS)
FLAGS	=	$(CONF_FLAGS)
MKDIR	=	@-$(CONF_LIBPATH)tools/my_mkdir.exe
RM	=	@-$(CONF_LIBPATH)tools/my_del.exe

PJ_NAME =	$(CONF_PJNAME)
IMG =	release/$(PJ_NAME).iso
EXE =	cd/0.bin

SRC=CRT0.S $(wildcard *.c) $(wildcard *.s) $(CONF_EXTRASRC)
OBJ=$(addsuffix .o,$(addprefix obj/,$(SRC)))

LIBSRC=$(CONF_LIBSRC)
LIBOBJ=$(addsuffix .o,$(addprefix obj/lib/,$(LIBSRC)))

all :	$(IMG)

$(IMG)	:	$(EXE)
		$(MKDIR) $(dir $(IMG))
		$(RM) $(IMG)
		$(RM) cd/ietx3win.tds
		mkisofs -quiet -sysid "SEGA SATURN" -volid "$(PJ_NAME)" -volset "$(PJ_NAME)" -publisher "SEGA ENTERPRISES, LTD." -preparer "SEGA ENTERPRISES, LTD." -appid "$(PJ_NAME)" -abstract "ABS.TXT" -copyright "CPY.TXT" -biblio "BIB.TXT" -generic-boot $(CONF_IP_PATH) -full-iso9660-filenames -o $(IMG) cd
		$(CONF_LIBPATH)tools/ip_header.exe $(IMG) -today -man "CAFE-ALPHA" -prod "T-042G" -autover release.h -area "JTUBKAEL" -per "J" -title $(CONF_IPTITLE)

$(EXE)	:	$(OBJ) $(LIBOBJ)
		@echo "Linking ..."
		$(MKDIR) $(dir $(EXE))
		$(RM) $(EXE) $(IMG)
		$(LD) $(OBJ) $(LIBOBJ) $(LDFLAGS) -L$(CONF_LGCC_PATH) -lc -lgcc -o $(EXE)

obj/%.c.o :	%.c
		@echo "Processing $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(IMG)
		$(CC) -c $< -o $@ $(FLAGS)

obj/%.s.o :	%.s
		@echo "Processing $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(IMG)
		$(AS) $< -o $@
obj/%.S.o :	%.S
		@echo "Processing $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(IMG)
		$(AS) $< -o $@

obj/lib/%.c.o :	$(CONF_LIBPATH)%.c
		@echo "Processing lib $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(IMG)
		$(CC) -c $< -o $@ $(FLAGS)

obj/lib/%.s.o :	$(CONF_LIBPATH)%.s
		@echo "Processing lib $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(IMG)
		$(AS) $< -o $@

#
# end of makefile
#
