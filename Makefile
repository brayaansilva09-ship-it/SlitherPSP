TARGET = SlitherPSP
OBJS = main.o
​CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
​LIBS = -lSDL_gfx -lSDL -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspsdk -lc -lpspnet -lpspnet_inet -lpspnet_apctl -lpspnet_resolver -lpsputility -lpspuser -lpspkernel
​EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Slither.io PSP
PSP_EBOOT_SFO = PARAM.SFO
​PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
​iso: $(EXTRA_TARGETS)
mkdir -p iso_root/PSP_GAME/SYSDIR
cp (TARGET).prx iso_root/PSP_GAME/SYSDIR/BOOT.BIN 2>/dev/null \vert{}\vert{} psp-strip(TARGET).elf -o iso_root/PSP_GAME/SYSDIR/BOOT.BIN
cp $(PSP_EBOOT_SFO) iso_root/PSP_GAME/PARAM.SFO
genisoimage -iso-level 4 -xa -A "PSP GAME" -V "SLITHER_PSP" -sysid "PSP GAME" -o SlitherPSP.iso iso_root/
rm -rf iso_root
