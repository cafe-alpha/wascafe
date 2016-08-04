CONF_VERBOSE = @
CONF_SH_ELF = saturn-sh2-elf-
CONF_FLAGS = -Wall -Werror -m2 -O2 -fno-strict-aliasing 
CONF_LDFLAGS = -nostartfiles -T bart.lnk -Wl,-Map=obj/main_elf.map 
CONF_LGCC_PATH = C:/SaturnOrbit/saturn-sdk/toolchain_elf/lib/gcc/sh-elf/5.1.0
CONF_IP_PATH = C:/SaturnOrbit/COMMON/IP_GAS/IP_elf.BIN
CONF_PJNAME = ScCartTest
CONF_IPTITLE = "Cartridge Test  Utility         By cafe-alpha   ppcenter.free.fr"
CONF_LIBPATH = ../
CONF_LIBSRC =    
CONF_EXTRASRC = 
