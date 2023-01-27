STATIC_LINKING=0

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

TARGET_NAME := x48
GIT_VERSION ?= " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif
LIBM := -lm
fpic=

ifeq ($(ARCHFLAGS),)
ifeq ($(archs),ppc)
   ARCHFLAGS = -arch ppc -arch ppc64
else
#  ARCHFLAGS = -arch i386 -arch x86_64
endif
endif

ifeq ($(STATIC_LINKING),1)
	EXT=a

ifeq ($(platform), unix)
PLAT=_unix
endif
endif

ifeq ($(platform), unix)
	EXT?=so
   TARGET := $(TARGET_NAME)_libretro$(PLAT).$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
else ifeq ($(platform), linux-portable)
	EXT?=so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
   SHARED := -shared -Wl,--no-undefined
	LIBM :=
else ifeq ($(platform), osx)
	EXT?=dylib
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -dynamiclib
# iOS
else ifneq (,$(findstring ios,$(platform)))
	EXT?=dylib
   TARGET := $(TARGET_NAME)_libretro_ios.$(EXT)
   fpic := -fPIC
   SHARED := -dynamiclib
   DEFINES := -DIOS

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif
	CC = cc -arch armv7 -isysroot $(IOSSDK)
ifeq ($(platform),ios9)
   SHARED += -miphoneos-version-min=8.0
   CC +=  -miphoneos-version-min=8.0
else
   SHARED += -miphoneos-version-min=5.0
   CC +=  -miphoneos-version-min=5.0
endif
else ifeq ($(platform), theos_ios)
	# Theos iOS
DEPLOYMENT_IOSVERSION = 5.0
TARGET = iphone:latest:$(DEPLOYMENT_IOSVERSION)
ARCHS = armv7 armv7s
TARGET_IPHONEOS_DEPLOYMENT_VERSION=$(DEPLOYMENT_IOSVERSION)
THEOS_BUILD_DIR := objs
include $(THEOS)/makefiles/common.mk

LIBRARY_NAME = $(TARGET_NAME)_libretro_ios

# QNX
else ifeq ($(platform), qnx)
	EXT?=so
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined

# Emscripten
else ifeq ($(platform), emscripten)
	EXT?=bc
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	STATIC_LINKING = 1

# PS3
else ifeq ($(platform), ps3)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	CC = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
	CC_AS = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
	AR = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-ar.exe
	PLATFORM_DEFINES := -D__CELLOS_LV2__
	STATIC_LINKING = 1

# sncps3
else ifeq ($(platform), sncps3)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_ps3.$(EXT)
	CC = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
	CC_AS = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
	AR = $(CELL_SDK)/host-win32/sn/bin/ps3snarl.exe
	PLATFORM_DEFINES := -D__CELLOS_LV2__
	STATIC_LINKING = 1

# Lightweight PS3 Homebrew SDK
else ifeq ($(platform), psl1ght)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	CC = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
	CC_AS = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
	AR = $(PS3DEV)/ppu/bin/ppu-ar$(EXE_EXT)
	PLATFORM_DEFINES := -D__CELLOS_LV2__
	STATIC_LINKING = 1

# PSP
else ifeq ($(platform), psp1)
	EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = psp-gcc$(EXE_EXT)
   AR = psp-ar$(EXE_EXT)
   PLATFORM_DEFINES := -DPSP -G0
   STATIC_LINKING = 1

# Vita
else ifeq ($(platform), vita)
	EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	CC = arm-vita-eabi-gcc$(EXE_EXT)
	AR = arm-vita-eabi-ar$(EXE_EXT)
   PLATFORM_DEFINES := -DVITA
   STATIC_LINKING = 1

# CTR (3DS)
else ifeq ($(platform), ctr)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	CFLAGS += -DARM11 -D_3DS
	CFLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard
	CFLAGS += -Wall -mword-relocations
	CFLAGS += -fomit-frame-pointer -ffast-math
        PLATFORM_DEFINES := -D_3DS
        STATIC_LINKING = 1

# Nintendo Game Cube
else ifeq ($(platform), ngc)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CC_AS = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	PLATFORM_DEFINES += -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float
	STATIC_LINKING = 1

# Nintendo Wii
else ifeq ($(platform), wii)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CC_AS = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	PLATFORM_DEFINES += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
	STATIC_LINKING = 1

# Nintendo WiiU
else ifeq ($(platform), wiiu)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CC_AS = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	PLATFORM_DEFINES += -DGEKKO -DWIIU -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
	STATIC_LINKING = 1

else
	EXT?=dll
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   SHARED := -shared -static-libgcc -Wl,--no-undefined -s

endif

ifeq ($(STATIC_LINKING),1)
fpic=
SHARED=
endif

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g
else
   CFLAGS += -O3
endif

CORE_DIR := .

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o)
CFLAGS += $(fpic) $(PLATFORM_DEFINES)

CFLAGS += 
LFLAGS := 
LDFLAGS += $(LIBM)

ifeq ($(platform), osx)
ifndef ($(NOUNIVERSAL))
   CFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)
endif
endif

with_fpic=
ifneq ($(fpic),)
   with_fpic := --with-pic=yes
endif

ifeq ($(platform), theos_ios)
COMMON_FLAGS := -DIOS $(COMMON_DEFINES) $(INCFLAGS) -I$(THEOS_INCLUDE_PATH) -Wno-error
$(LIBRARY_NAME)_CFLAGS += $(COMMON_FLAGS) $(CFLAGS)
${LIBRARY_NAME}_FILES = $(SOURCES_C)
include $(THEOS_MAKE_PATH)/library.mk
else
all: $(TARGET)

$(TARGET): $(OBJECTS) 
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CC) $(fpic) $(SHARED) $(INCLUDES) $(LFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)
endif

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET) x48_libretro.dylib x48_libretro.dll

.PHONY: clean
endif
