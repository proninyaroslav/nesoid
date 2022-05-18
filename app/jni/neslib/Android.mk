LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := user

#LOCAL_ARM_MODE := arm

# This is the target being built.
LOCAL_MODULE := libnes

# All of the source files that we will compile.
LOCAL_SRC_FILES := \
	fce.c \
	video.c \
	general.c \
	endian.c \
	svga.c \
	sound.c \
	nsf.c \
	fds.c \
	netplay.c \
	ines.c \
	state.c \
	unif.c \
	input.c \
	file.c \
	fileio.c \
	memfile.c \
	cart.c \
	crc32.c \
	memory.c \
	cheat.c \
	debug.c \
	md5.c \
	vsuni.c \
	palette.c \
	ppu.c \
	movie.c \
	fceu098.c \
	ppu098.c \
	mappers/113.c \
	mappers/15.c \
	mappers/151.c \
	mappers/16.c \
	mappers/17.c \
	mappers/18.c \
	mappers/180.c \
	mappers/183.c \
	mappers/184.c \
	mappers/193.c \
	mappers/200.c \
	mappers/201.c \
	mappers/202.c \
	mappers/203.c \
	mappers/204.c \
	mappers/21.c \
	mappers/212.c \
	mappers/213.c \
	mappers/214.c \
	mappers/215.c \
	mappers/217.c \
	mappers/22.c \
	mappers/225.c \
	mappers/226.c \
	mappers/227.c \
	mappers/228.c \
	mappers/229.c \
	mappers/23.c \
	mappers/230.c \
	mappers/231.c \
	mappers/232.c \
	mappers/234.c \
	mappers/240.c \
	mappers/241.c \
	mappers/242.c \
	mappers/244.c \
	mappers/246.c \
	mappers/24and26.c \
	mappers/25.c \
	mappers/255.c \
	mappers/27.c \
	mappers/32.c \
	mappers/33.c \
	mappers/40.c \
	mappers/41.c \
	mappers/42.c \
	mappers/43.c \
	mappers/46.c \
	mappers/50.c \
	mappers/51.c \
	mappers/59.c \
	mappers/6.c \
	mappers/60.c \
	mappers/61.c \
	mappers/62.c \
	mappers/65.c \
	mappers/67.c \
	mappers/68.c \
	mappers/69.c \
	mappers/71.c \
	mappers/72.c \
	mappers/73.c \
	mappers/75.c \
	mappers/76.c \
	mappers/77.c \
	mappers/79.c \
	mappers/8.c \
	mappers/80.c \
	mappers/82.c \
	mappers/83.c \
	mappers/85.c \
	mappers/86.c \
	mappers/89.c \
	mappers/91.c \
	mappers/92.c \
	mappers/97.c \
	mappers/99.c \
	mappers/emu2413.c \
	mappers/mmc2and4.c \
	mappers/simple.c \
	boards/112.c \
	boards/117.c \
	boards/164.c \
	boards/183.c \
	boards/185.c \
	boards/186.c \
	boards/187.c \
	boards/189.c \
	boards/208.c \
	boards/222.c \
	boards/235.c \
	boards/57.c \
	boards/8157.c \
	boards/8237.c \
	boards/88.c \
	boards/90.c \
	boards/95.c \
	boards/bmc13in1jy110.c \
	boards/bmc42in1r.c \
	boards/bmc64in1nr.c \
	boards/bmc70in1.c \
	boards/bmcgk192.c \
	boards/bonza.c \
	boards/cc21.c \
	boards/datalatch.c \
	boards/deirom.c \
	boards/dream.c \
	boards/fk23c.c \
	boards/h2288.c \
	boards/karaoke.c \
	boards/kof97.c \
	boards/konami-qtai.c \
	boards/malee.c \
	boards/mmc1.c \
	boards/mmc3.c \
	boards/mmc5.c \
	boards/n106.c \
	boards/novel.c \
	boards/sachen.c \
	boards/sheroes.c \
	boards/sl1632.c \
	boards/sonic5.c \
	boards/subor.c \
	boards/super24.c \
	boards/supervision.c \
	boards/t-262.c \
	boards/tengen.c \
	boards/__dummy_mapper.c \
	input/cursor.c \
	input/zapper.c \
	input/powerpad.c \
	input/arkanoid.c \
	input/shadow.c \
	input/fkb.c \
	input/hypershot.c \
	input/mahjong.c \
	input/oekakids.c \
	input/ftrainer.c \
	input/quiz.c \
	input/toprider.c \
	input/bworld.c \
	input/suborkb.c \
	zlib/zip.c \
	zlib/unzip.c \
	zlib/ioapi.c

LOCAL_CFLAGS += -DASM_6502
LOCAL_SRC_FILES += \
	ncpu.S \
	giz_blit.s \
	giz_blit_rev.s

LOCAL_SRC_FILES += \
	drivers/android/debug.c \
	drivers/android/file.c \
	drivers/android/netplay.c \
	drivers/android/nesengine.cpp

# All of the shared libraries we link against.
#LOCAL_SHARED_LIBRARIES := \
#	libutils \
#	libz

# Static libraries.
#LOCAL_STATIC_LIBRARIES := \
#	libunz

# Also need the JNI headers.
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../common \

LOCAL_LDLIBS := -lz -llog

LOCAL_DISABLE_FATAL_LINKER_WARNINGS = true

# Special compiler flags.
LOCAL_CFLAGS += -O3 -fvisibility=hidden -fno-integrated-as

LOCAL_CFLAGS += \
	-DHAVE_ASPRINTF \
	-DPSS_STYLE=1 \
	-DLSB_FIRST=1 \
	-DFRAMESKIP=1 \
	-DZLIB

# Don't prelink this library.  For more efficient code, you may want
# to add this library to the prelink map and set this to true. However,
# it's difficult to do this for applications that are not supplied as
# part of a system image.

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

