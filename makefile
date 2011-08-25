ROOT=$(DEVKITARM)/bin

CC=$(ROOT)/arm-eabi-gcc
CXX=$(ROOT)/arm-eabi-g++
AS=$(ROOT)/arm-eabi-as
OBJCOPY=$(ROOT)/arm-eabi-objcopy

THUMB = #-mthumb

INCLUDES="$(DEVKITPRO)/libnds/include"
LIBDIR="$(DEVKITPRO)/libnds/lib"
COPY_TO=/cygdrive/l/default.nds
#GAME_COPY_TO=/cygdrive/l/gamearm.bin

#INCLUDES="/home/simon/devkitpro/devkitARM/include"
#LIBNDS_INCLUDES="/home/simon/devkitpro/devkitARM/include/nds"
#LIBDIR="/home/simon/devkitpro/devkitARM/lib/"
#COPY_TO=/media/disk/DEFAULT.NDS

OPTS=-Dstricmp=strcasecmp -I $(INCLUDES) -DARM9 -DR21 -DGAME_HARD_LINKED -mthumb-interwork -Os -g -mtune=arm946e-s -march=armv5te $(THUMB) -fno-rtti -fno-exceptions -Wno-write-strings
#OPTS+=-finstrument-functions -mpoke-function-name

LD_OPTS=-specs=ds_arm9.specs -L$(LIBDIR) -lfat -lnds9 -lm -Wl,-Map,$(notdir $*.map),-zmuldefs

%.o: %.c
	echo compiling $^ to $@
	$(CXX) $(OPTS) -c $^ -o $@
	
%.o: %.cpp
	echo compiling $^ to $@
	$(CXX) -g -mthumb-interwork -I $(INCLUDES) -I $(LIBNDS_INCLUDES) -Os -DARM7 -fno-rtti -fno-exceptions -Wno-write-strings -c $^ -o $@

all: quake2.nds

clean:
	echo cleaning...
	rm -f $(ALL_ARM9_OBJS) $(ALL_ARM7_OBJS) quake2.elf quake2.bin quake2.nds arm7.bin arm7.elf
clean_game:
	echo cleaning game...
	rm -f $(GAME_OBJS)

CLIENT_OBJS= \
	client/cl_cin.o \
	client/cl_ents.o \
	client/cl_fx.o \
	client/cl_input.o \
	client/cl_inv.o \
	client/cl_main.o \
	client/cl_newfx.o \
	client/cl_parse.o \
	client/cl_pred.o \
	client/cl_scrn.o \
	client/cl_tent.o \
	client/cl_view.o \
	client/console.o \
	client/keys.o \
	client/menu.o \
	client/qmenu.o \
	client/snd_dma.o \
	client/snd_mem.o \
	client/snd_mix.o \
	client/x86.o

GAME_OBJS= \
	game/g_ai.o \
	game/g_chase.o \
	game/g_cmds.o \
	game/g_combat.o \
	game/g_func.o \
	game/g_items.o \
	game/g_main.o \
	game/g_misc.o \
	game/g_monster.o \
	game/g_phys.o \
	game/g_save.o \
	game/g_spawn.o \
	game/g_svcmds.o \
	game/g_target.o \
	game/g_trigger.o \
	game/g_turret.o \
	game/g_utils.o \
	game/g_weapon.o \
	game/m_actor.o \
	game/m_berserk.o \
	game/m_boss2.o \
	game/m_boss3.o \
	game/m_boss31.o \
	game/m_boss32.o \
	game/m_brain.o \
	game/m_chick.o \
	game/m_flash.o \
	game/m_flipper.o \
	game/m_float.o \
	game/m_flyer.o \
	game/m_gladiator.o \
	game/m_gunner.o \
	game/m_hover.o \
	game/m_infantry.o \
	game/m_insane.o \
	game/m_medic.o \
	game/m_move.o \
	game/m_mutant.o \
	game/m_parasite.o \
	game/m_soldier.o \
	game/m_supertank.o \
	game/m_tank.o \
	game/p_client.o \
	game/p_hud.o \
	game/p_trail.o \
	game/p_view.o \
	game/p_weapon.o
	
GAME_WRAPPER= game/libc_wrap.o
game/libc_wrap.o: game/libc_wrap.c
	echo compiling game wrapper
	$(CC) -c $^ -o $@

NULL_OBJS= \
	game/q_shared.o \
	null/cd_null.o \
	null/in_null.o \
	null/net_udp.o \
	null/snd_null.o \
	null/swimp_null.o \
	null/sys_null.o \
	null/vid_null.o \
	null/m_flash_dll.o \
	null/glob.o \
	null/vid_menu.o
	
QCOMMON_OBJS= \
	qcommon/cmd.o \
	qcommon/cmodel.o \
	qcommon/common.o \
	qcommon/crc.o \
	qcommon/cvar.o \
	qcommon/files.o \
	qcommon/md4.o \
	qcommon/net_chan.o \
	qcommon/pmove.o
	
REF_SOFT_OBJS= \
	ref_soft/r_aclip.o \
	ref_soft/r_alias.o \
	ref_soft/r_bsp.o \
	ref_soft/r_draw.o \
	ref_soft/r_edge.o \
	ref_soft/r_image.o \
	ref_soft/r_light.o \
	ref_soft/r_main.o \
	ref_soft/r_misc.o \
	ref_soft/r_model.o \
	ref_soft/r_part.o \
	ref_soft/r_poly.o \
	ref_soft/r_polyse.o \
	ref_soft/r_rast.o \
	ref_soft/r_scan.o \
	ref_soft/r_sprite.o \
	ref_soft/r_surf.o
	
REF_NDS_OBJS= \
	ref_nds/r_aclip.o \
	ref_nds/r_alias.o \
	ref_nds/r_bsp.o \
	ref_nds/r_draw.o \
	ref_nds/r_edge.o \
	ref_nds/r_image.o \
	ref_nds/r_light.o \
	ref_nds/r_main.o \
	ref_nds/r_misc.o \
	ref_nds/r_model.o \
	ref_nds/r_part.o \
	ref_nds/r_poly.o \
	ref_nds/r_polyse.o \
	ref_nds/r_rast.o \
	ref_nds/r_scan.o \
	ref_nds/r_sprite.o \
	ref_nds/r_surf.o

REF_GL_OBJS= \
	ref_gl/gl_draw.o \
	ref_gl/gl_image.o \
	ref_gl/gl_light.o \
	ref_gl/gl_mesh.o \
	ref_gl/gl_model.o \
	ref_gl/gl_rmain.o \
	ref_gl/gl_rmisc.o \
	ref_gl/gl_rsurf.o \
	ref_gl/gl_warp.o

	
SERVER_OBJS= \
	server/sv_ccmds.o \
	server/sv_ents.o \
	server/sv_game.o \
	server/sv_init.o \
	server/sv_main.o \
	server/sv_send.o \
	server/sv_user.o \
	server/sv_world.o \

LOOSE_OBJS= \
	ds_main.o \
	ds_files.o \
	ds_ipc.o \
	malloc.o \
	memory.o \
	cyg-profile.o \
	ds_3d.o \
	ds_textures.o \
	ram.o \
	r_cache.o \
	keyboard/touchkeyboard.o \
	keyboard/keyboard_pal.o \
	keyboard/keyboard.o

keyboard/keyboard.o: keyboard/keyboard.raw
	$(ROOT)/bin2s $< | $(AS) -o $(@)

keyboard/keyboard_pal.o: keyboard/keyboard_pal.raw
	$(ROOT)/bin2s $< | $(AS) -o $(@)
	
ARM7_OBJS= \
	arm7_main.o \
	snd_null_7.o \
	ref_nds/r_bsp_7.o

ALL_ARM9_OBJS=$(LOOSE_OBJS) $(SERVER_OBJS) $(REF_NDS_OBJS) $(QCOMMON_OBJS) $(NULL_OBJS) $(GAME_OBJS) $(CLIENT_OBJS)
ALL_ARM7_OBJS=$(ARM7_OBJS)

gamearm.elf: $(GAME_OBJS) game/q_shared.o $(GAME_WRAPPER)
	echo linking $@
	$(CC) $(GAME_WRAPPER) game/q_shared.o $(GAME_OBJS) -o $@ -Wl,-T game_link.x -lm -nostartfiles

quake2.elf: $(ALL_ARM9_OBJS) stamp.o
	echo linking $@
	$(CXX) $(ALL_ARM9_OBJS) keyboard/keyboard_pal.o keyboard/touchkeyboard.o stamp.o $(LD_OPTS) -o $@

arm7.elf: $(ALL_ARM7_OBJS)
	echo linking $@
	$(CXX) $(ALL_ARM7_OBJS) -mthumb-interwork -marm -specs=ds_arm7.specs -L$(LIBDIR) -lnds7 -o $@ -Wl,-zmuldefs

stamp.o: $(ALL_ARM9_OBJS) stamp.c
	$(CXX) -c stamp.c -o stamp.o
	
quake2.nds: quake2.elf arm7.elf #gamearm.bin
	echo making and copying $(COPY_TO)
	$(ROOT)/ndstool -c quake2.nds -9 quake2.elf -7 arm7.elf > output.txt
	#dlditool M3sd.dldi quake2.nds
	#cp quake2.nds $(COPY_TO)

