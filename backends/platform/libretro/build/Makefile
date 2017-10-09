##LIBRETRO
DEBUG=0
USE_ZLIB   = 1
USE_TREMOR = 0
USE_VORBIS = 1
USE_FLAC   = 1
USE_MAD    = 1
USE_FAAD   = 1
USE_PNG    = 1
USE_JPEG   = 1
USE_THEORADEC=1
USE_FREETYPE2=1
HAVE_MT32EMU=0

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

ifeq ($(shell uname -a),)
   EXE_EXT = .exe
endif

TARGET_NAME := scummvm
GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	CXXFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

TARGET_64BIT = 0
ifeq ($(shell uname -m), x86_64)
   BUILD_64BIT = 1
else
   BUILD_64BIT = 0
endif

LD        = $(CXX)
AR        = ar cru
RANLIB    = ranlib

ifeq ($(platform), unix)
   TARGET  := $(TARGET_NAME)_libretro.so
   DEFINES += -fPIC
   LDFLAGS += -shared -Wl,--version-script=../link.T -fPIC
   TARGET_64BIT := $(BUILD_64BIT)
# OS X
else ifeq ($(platform), osx)
   TARGET  := $(TARGET_NAME)_libretro.dylib
   DEFINES += -fPIC
   LDFLAGS += -dynamiclib -fPIC
ifneq ($(shell uname -p),powerpc)
   arch = intel
   TARGET_64BIT := $(BUILD_64BIT)
else
   arch = ppc
endif

# iOS
else ifneq (,$(findstring ios,$(platform)))
   TARGET  := $(TARGET_NAME)_libretro_ios.dylib
   DEFINES += -fPIC -DHAVE_POSIX_MEMALIGN=1 -DIOS
   LDFLAGS += -dynamiclib -fPIC

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

   CXX      = clang++ -arch armv7 -isysroot $(IOSSDK) -marm

ifeq ($(platform),ios9)
	CXX     += -miphoneos-version-min=8.0
else
	CXX     += -miphoneos-version-min=5.0
endif

# QNX
else ifeq ($(platform), qnx)
   TARGET  := $(TARGET_NAME)_libretro_$(platform).so
   DEFINES += -fPIC -DSYSTEM_NOT_SUPPORTING_D_TYPE
   LDFLAGS += -shared -Wl,--version-script=../link.T -fPIC
   CC = qcc -Vgcc_ntoarmv7le
   CXX = QCC -Vgcc_ntoarmv7le
   LD = QCC -Vgcc_ntoarmv7le
   AR = qcc -Vgcc_ntoarmv7le -A
   RANLIB="${QNX_HOST}/usr/bin/ntoarmv7-ranlib"

# PS3
else ifeq ($(platform), ps3)
   TARGET  := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
   CXX = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-g++.exe
   AR = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-ar.exe rcs
   DEFINES += -DPLAYSTATION3
	STATIC_LINKING=1

# Nintendo Wii
else ifeq ($(platform), wii)
   TARGET := $(TARGET_NAME)_libretro_wii.a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT) rcs
   DEFINES += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float -D__ppc__ -I$(DEVKITPRO)/libogc/include
	STATIC_LINKING=1

# Vita
else ifeq ($(platform), vita)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = arm-vita-eabi-gcc$(EXE_EXT)
	CXX = arm-vita-eabi-g++$(EXE_EXT)
	AR = arm-vita-eabi-ar$(EXE_EXT) rcs
	DEFINES += -DVITA
	STATIC_LINKING = 1

else ifeq ($(platform), android-armv7)
   TARGET  := $(TARGET_NAME)_libretro_android.so
   DEFINES += -fPIC -Wno-multichar -D_ARM_ASSEM_
   LDFLAGS += -shared -Wl,--version-script=../link.T -fPIC
   TOOLSET = arm-linux-androideabi-
   USE_VORBIS = 0
   USE_THEORADEC = 0
   USE_TREMOR = 1
else ifneq (,$(findstring armv,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.so
   SHARED := -shared -Wl,--no-undefined
   DEFINES += -fPIC -Wno-multichar -D_ARM_ASSEM_
   CC = gcc
   USE_VORBIS = 0
   USE_THEORADEC = 0
   USE_TREMOR = 1
ifneq (,$(findstring cortexa8,$(platform)))
   DEFINES += -marm -mcpu=cortex-a8
else ifneq (,$(findstring cortexa9,$(platform)))
   DEFINES += -marm -mcpu=cortex-a9
endif
   DEFINES += -marm
ifneq (,$(findstring neon,$(platform)))
   DEFINES += -mfpu=neon
   HAVE_NEON = 1
endif
ifneq (,$(findstring softfloat,$(platform)))
   DEFINES += -mfloat-abi=softfp
else ifneq (,$(findstring hardfloat,$(platform)))
   DEFINES += -mfloat-abi=hard
endif
   DEFINES += -DARM

# Emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_$(platform).bc
	STATIC_LINKING = 1

else
	CC = gcc
	TARGET_64BIT := $(BUILD_64BIT)
   TARGET  := $(TARGET_NAME)_libretro.dll
   DEFINES += -DHAVE_FSEEKO -DHAVE_INTTYPES_H -fno-permissive
   LDFLAGS += -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=../link.T
endif

ifeq ($(DEBUG), 1)
   DEFINES += -O0 -g
else
   DEFINES += -O3
endif

ifeq ($(TARGET_64BIT), 1)
   DEFINES += -DSIZEOF_SIZE_T=8 -DSCUMM_64BITS
else
   DEFINES += -DSIZEOF_SIZE_T=4
endif

###SCUMM VM
CORE_DIR = ../../../..
srcdir   = $(CORE_DIR)
VPATH    = $(CORE_DIR)

# Define toolset
ifdef TOOLSET
    CC        = $(TOOLSET)gcc
    CXX       = $(TOOLSET)g++
    LD        = $(TOOLSET)g++
    AR        = $(TOOLSET)ar cru
    RANLIB    = $(TOOLSET)ranlib
endif

MKDIR         = mkdir -p
RM            = rm -f
RM_REC        = rm -rf

ifeq ($(HAVE_MT32EMU),1)
USE_MT32EMU = 1
DEFINES += -DUSE_MT32EMU
endif

# Define build flags
DEFINES       += -D__LIBRETRO__ -DNONSTANDARD_PORT -DUSE_RGB_COLOR -DUSE_OSD -DDISABLE_TEXT_CONSOLE -DFRONTEND_SUPPORTS_RGB565 -Wno-multichar
DEFINES       += -DSCUMMVMKOR
DEPDIR        = .deps
HAVE_GCC3     = true
USE_RGB_COLOR = true

# Compile platform specific parts (e.g. filesystem)
ifeq ($(platform), win)
WIN32 = 1
else
POSIX = 1
endif

BACKEND := libretro

include Makefile.common
