#
# Copyright (C) 2013-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=t113_i-evb1_auto
BOARDNAME:=t113_i-evb1_auto
FEATURES:=fpu dt
CPU_TYPE:=cortex-a7
CPU_SUBTYPE:=neon
MAINTAINER:=Allwinner

KERNEL_PATCHVER:=5.4
UBOOT_PATCHVER:=2018
KERNELNAME:=zImage dtbs

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES = base-files libc libgcc busybox

$(eval $(call BuildTarget))
