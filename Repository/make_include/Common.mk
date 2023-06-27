# Set architecture, word size, and float ABI
ARCH		= arm
WORDSIZE	= 64
FLOATABI	= hard

#Common tools
PKGCFG          = pkg-config
MKDIR           = mkdir
RM              = rm
CXX             = gcc
MAKE            = make
CP              = cp

# Configure compiler and linker for ARM architecture
ARCH_CFLAGS	= -march=armv8-a

ifeq ($(CONFIG),Debug)
  CONFIG_CFLAGS         = -O0 -g -Wall
else
  CONFIG_CFLAGS         = -O3
endif

COMMON_CFLAGS   = $(CONFIG_CFLAGS) $(ARCH_CFLAGS) -fPIC