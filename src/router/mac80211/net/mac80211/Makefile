EXTRA_CFLAGS += -g3 -Wno-unused-function -Wno-unused-variable
ccflags-y := -g3 -Wno-unused-function -Wno-unused-variable
ccflags-m := -g3 -Wno-unused-function -Wno-unused-variable


obj-$(CPTCFG_MAC80211) += mac80211.o

mac80211-y := combine.o

CFLAGS_trace.o := -I$(src)


$(obj)/regdb.c: $(src)/../wireless/db.txt $(src)/../wireless/genregdb.awk
	@$(AWK) -f $(src)/../wireless/genregdb.awk < $< > $@

clean-files := regdb.c

#EXTRA_LDFLAGS += -flto -fwhole-program -flto=jobserver -fuse-linker-plugin

#ccflags-y += -flto -fwhole-program -fno-fat-lto-objects -flto=jobserver -fuse-linker-plugin
