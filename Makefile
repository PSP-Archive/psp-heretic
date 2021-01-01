TARGET = heretic
PSPSDK = $(shell psp-config --pspsdk-path)
PSPBIN = $(PSPSDK)/../bin

PSP    = yes

BUILD_PRX=1
PSP_FW_VERSION=303

LIBS = -lSDL_mixer -lsmpeg -lSDL -lGL -lGLU -lpsprtc -lpspirkeyb -lpsppower -lpspvfpu -lmad
LIBS += -lvorbisidec -lpspgum -lpspgu -lpsphprm -lm -lpspaudio -lstdc++ -lcrypto
CFLAGS = -O2 -g -Wall --fast-math -fno-unit-at-a-time -fdiagnostics-show-option -fno-exceptions
CFLAGS += -G0 -I/usr/local/pspdev/psp/include/SDL

OBJS =											\
am_map.o d_main.o d_net.o f_finale.o g_game.o i_endoom.o in_lude.o info.o mn_menu.o	\
m_random.o p_ceilng.o p_doors.o p_enemy.o p_floor.o p_inter.o p_lights.o p_map.o	\
p_maputl.o p_mobj.o p_plats.o p_pspr.o p_saveg.o p_setup.o p_sight.o p_spec.o		\
p_switch.o p_telept.o p_tick.o p_user.o r_bsp.o r_data.o r_draw.o r_main.o r_plane.o	\
r_segs.o r_things.o sb_bar.o sounds.o s_sound.o						\
											\
dbopl.o d_event.o d_iwad.o d_loop.o d_mode.o i_main.o i_oplmusic.o i_sdlmusic.o		\
i_sdlsound.o i_sound.o i_system.o i_timer.o i_video.o m_argv.o m_bbox.o	m_config.o	\
m_controls.o memio.o m_fixed.o midifile.o m_misc.o mus2mid.o opl.o opl_queue.o		\
opl_sdl.o opl_timer.o sha1.o tables.o txt_io.o txt_sdl.o v_video.o w_checksum.o		\
w_file.o w_file_stdc.o w_wad.o z_zone.o							\
											\
deh_ammo.o deh_frame.o deh_htext.o deh_htic.o deh_io.o deh_main.o deh_mapping.o		\
deh_sound.o deh_str.o deh_text.o deh_thing.o deh_weapon.o				\
											\
xmn_main.o xmn_psp.o xmn_md5.o

OBJS   += disablefpuexceptions.o

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP-Heretic by nitr8 (R1)
PSP_EBOOT_ICON = ICON0.PNG
PSP_EBOOT_UNKPNG = PIC0.PNG
PSP_EBOOT_SND0 = SND0.AT3
#PSP_EBOOT_PIC1 = PIC1.PNG

ifeq ($(PSP),yes)
include $(PSPSDK)/lib/build.mak
else

all: heretic

heretic: $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o heretic

clean:
	rm -f $(OBJS) heretic

endif
