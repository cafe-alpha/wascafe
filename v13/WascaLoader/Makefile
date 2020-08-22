#
# Wasca Loader
# by cafe-alpha
# WWW: http://ppcenter.free.fr/pskai/
#
# See LICENSE file for details.
#

# Contains project compilation settings
include mk_config.inc

# Contains Pseudo Saturn Kai version informations
include ../mk_verinfo.inc

CC	=	$(CONF_VERBOSE)$(CONF_SH_ELF)gcc
LD	=	$(CONF_VERBOSE)$(CONF_SH_ELF)gcc
AS	=	$(CONF_VERBOSE)$(CONF_SH_ELF)as
LDFLAGS =	$(CONF_LDFLAGS)
FLAGS	=	$(CONF_FLAGS)
FLAGS_F0	=	$(CONF_FLAGS_F0)
MKDIR	=	@-$(CONF_LIBPATH)tools/my_mkdir.exe
RM	=	@-$(CONF_LIBPATH)tools/my_del.exe
COMP	=	@-$(CONF_LIBPATH)tools/comp.exe
PJ_NAME =	$(CONF_PJNAME)

EXE =	cd/0.BIN
IMG =	release/$(PJ_NAME).iso

SRC=CRT0.S $(wildcard *.c) $(wildcard *.s) $(CONF_EXTRASRC)
OBJ=$(addsuffix .o,$(addprefix obj/,$(SRC)))

PSKAISRC=$(CONF_PSKAISRC)
PSKAIOBJ=$(addsuffix .o,$(addprefix obj/pskai/,$(PSKAISRC)))

LIBSRC=$(CONF_LIBSRC)
LIBOBJ=$(addsuffix .o,$(addprefix obj/lib/,$(LIBSRC)))

all :	$(IMG)

$(IMG)	:	$(EXE)
		$(MKDIR) $(dir $(IMG))
		$(RM) $(IMG)
		mkisofs -quiet -sysid "SEGA SATURN" -volid "$(PJ_NAME)" -volset "$(PJ_NAME)" -publisher "SEGA ENTERPRISES, LTD." -preparer "SEGA ENTERPRISES, LTD." -appid "$(PJ_NAME)" -abstract "ABS.TXT" -copyright "CPY.TXT" -biblio "BIB.TXT" -generic-boot $(CONF_IP_PATH) -full-iso9660-filenames -o $(IMG) cd
		$(CONF_LIBPATH)tools/ip_header.exe $(IMG) -today -man "SEGA NEUROACID  " -prod "T-042G" -autover ../release.h -area "JTUBKAEL" -per "JA" -title "WASCA BOOTLOADER     v$(CONF_VERINFO)     By cafe-alpha   ppcenter.free.fr"


$(EXE)	:	$(OBJ) $(PSKAIOBJ) $(LIBOBJ)
		@echo "Linking (bin) ..."
		$(MKDIR) $(dir $(EXE))
		$(RM) $(EXE)
		$(LD) $(OBJ) $(PSKAIOBJ) $(LIBOBJ) $(LDFLAGS) -L$(CONF_LGCC_PATH) -lc -lgcc -o $(EXE)

obj/memtest_feedback.c.o :	memtest_feedback.c
		@echo "Processing f0 $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(EXE)
		$(CC) -c $< -o $@ $(FLAGS_F0)

obj/%.c.o :	%.c
		@echo "Processing $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(EXE)
		$(CC) -c $< -o $@ $(FLAGS)

obj/%.s.o :	%.s
		@echo "Processing $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(EXE)
		$(AS) $< -o $@
obj/%.S.o :	%.S
		@echo "Processing $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(EXE)
		$(AS) $< -o $@

obj/pskai/%.c.o :	../%.c
		@echo "Processing $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(EXE)
		$(CC) -c $< -o $@ $(FLAGS)

obj/pskai/%.s.o :	../%.s
		@echo "Processing $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(EXE)
		$(AS) $< -o $@

obj/lib/%.c.o :	$(CONF_LIBPATH)%.c
		@echo "Processing lib $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(EXE)
		$(CC) -c $< -o $@ $(FLAGS)

obj/lib/%.s.o :	$(CONF_LIBPATH)%.s
		@echo "Processing lib $< ..."
		$(MKDIR) $(dir $@)
		$(RM) $@ $(EXE)
		$(AS) $< -o $@
clean	:
		rm -f $(LIBOBJ)
		rm -f $(PSKAIOBJ)
		rm -f $(OBJ)

mrproper:
		rm -f $(LIBOBJ)
		rm -f $(PSKAIOBJ)
		rm -f $(OBJ)
		rm -f $(EXE)
		rm -f $(IMG)

makedir :
		mkdir obj
		mkdir cd


#
# end of makefile
#
