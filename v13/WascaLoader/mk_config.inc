CONF_VERBOSE = @
CONF_SH_ELF = saturn-sh2-elf-
CONF_FLAGS = -Wall -m2 -O2 -fno-strict-aliasing -fdata-sections -ffunction-sections -I$(CONF_LIBPATH)satcom_lib/ -DSC_SATURN -DSC_USE_IAPETUS -DLOGO_DISABLE -DSMPC_USE_POLLING
CONF_LDFLAGS = -nostartfiles -T bart.lnk -Wl,-Map=obj/main_elf.map -Wl,--gc-sections
CONF_LGCC_PATH = D:/dev/SaturnOrbit/saturn-sdk/toolchain_elf/lib/gcc/sh-elf/5.1.0
CONF_IP_PATH = D:/dev/SaturnOrbit/COMMON/IP_GAS/IP_elf.BIN
CONF_PJNAME = WascaLoader
CONF_LIBPATH = ../../
CONF_LIBSRC =  satcom_lib/sc_common.c satcom_lib/sc_compress.c satcom_lib/sc_romfs.c satcom_lib/sc_saturn.c satcom_lib/iapetus/cd/cd.c satcom_lib/iapetus/cd/cdfs.c satcom_lib/iapetus/sh2/sh2int.c satcom_lib/sdcard/sdcard_packet.c satcom_lib/sdcard/fat_io_lib/fat_access.c satcom_lib/sdcard/fat_io_lib/fat_cache.c satcom_lib/sdcard/fat_io_lib/fat_filelib.c satcom_lib/sdcard/fat_io_lib/fat_format.c satcom_lib/sdcard/fat_io_lib/fat_global.c satcom_lib/sdcard/fat_io_lib/fat_internal.c satcom_lib/sdcard/fat_io_lib/fat_list.c satcom_lib/sdcard/fat_io_lib/fat_misc.c satcom_lib/sdcard/fat_io_lib/fat_string.c satcom_lib/sdcard/fat_io_lib/fat_table.c satcom_lib/sdcard/fat_io_lib/fat_write.c satcom_lib/sdcard/sdcard_fat.c   satcom_lib/iapetus/ar/commlink.c satcom_lib/iapetus/cd/mpeg.c satcom_lib/iapetus/cd/cdsect.c satcom_lib/iapetus/scu/dsp.c satcom_lib/iapetus/scu/scudma.c satcom_lib/iapetus/scu/extram.c satcom_lib/iapetus/peripherals/smpc.c satcom_lib/iapetus/sh2/sci.c satcom_lib/iapetus/sh2/sh2dma.c satcom_lib/iapetus/sh2/timer.c satcom_lib/iapetus/sound/sound.c satcom_lib/iapetus/iapetus.c 
CONF_PSKAISRC = conio.c debug_console.c filebrowser.c fonts.c menu.c soft_reset.c vdp.c BackupMngr/fonts_data.s
