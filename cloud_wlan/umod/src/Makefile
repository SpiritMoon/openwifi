.NOTPARALLEL:
#CC为交叉编译GCC路径，以后可以跟据情况修改
#CC=~/openwrt/main/staging_dir/toolchain-mips64_gcc-4.4.1_eglibc-2.10.1/usr/bin/mips64-openwrt-linux-gnu-gcc
#CC=/workPorjectCode/openwrt/trunk/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-uclibc-gcc
#LFLAGS+=
APCOM_OBJ=cloud_wlan_ap_com.o \
	cloud_wlan_sqlite.o \
	cloud_wlan_ulog.o \
	cloud_wlan_nl_u_if.o \
	cloud_wlan_ap_local.o \
	cloud_wlan_ap_online_db.o
	

DEBUG_OBJ=cloud_wlan_cmd.o \
	cloud_wlan_sqlite.o \
	cloud_wlan_ulog.o \
	cloud_wlan_nl_u_if.o \
	cloud_wlan_ap_local.o 

PTHREAD_LIBS = -lpthread -lsqlite3
PARAM= -W -g -Wall -Wno-unused-parameter -DUSE_RUNTIME_MODEL_CHECKS=1

.PHONY: all
all: cloud_wlan_ap_com cloud_wlan_cmd
cloud_wlan_ap_com:$(APCOM_OBJ)
	$(CC) $(CFLAGS) $(LFLAGS) $(PARAM) $(PTHREAD_LIBS) -o $@ $(APCOM_OBJ)
cloud_wlan_cmd:$(DEBUG_OBJ) 
	$(CC) $(CFLAGS) $(LFLAGS) $(PARAM) $(PTHREAD_LIBS) -o $@ $(DEBUG_OBJ)
	