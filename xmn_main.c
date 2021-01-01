#include <malloc.h>
#include <pspdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "doomdef.h"
//#include "h2_main.h"
#include "i_sound.h"
#include "m_argv.h"

#include "xmn_main.h"
#include "xmn_psp.h"
#include "xmn_PSPm.h"

#define MAXDEPTH	16
#define MAXDIRNUM	1024
#define PATHLIST_H	3
#define REPEAT_TIME	0x40000
#define BUFSIZE		65536
#define PWADPath	"WADS/PWAD/"
#define IWADPath	"WADS/IWAD/"

static 			SceCtrlData ctl;

dirent_t		dlist[MAXDIRNUM];
dirent_t		dirlist[MAXDIRNUM];
dirent_t		dlist2[MAXDIRNUM];

char			check[MAXPATH];
char			now_path[MAXPATH];
char			buf[BUFSIZE];
char			heretic_wad_dir[256];
char			dehacked_file[256];
char			path_tmp[MAXPATH];

int			txtLoaded = 0;
int			txtPos = 0;
int			WADLoaded = 0;
int			dlist_num;
int			dlist_num2;
int			dlist_start;
int			dlist_curpos;
int			dlist_drawstart;
int			cbuf_start[MAXDEPTH];
int			cbuf_curpos[MAXDEPTH];
int			now_depth;
int			load_dehacked = 0;
int			extra_wad_loaded = 0;

#pragma GCC diagnostic push						// FOR PSP: SceIo... operators
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"	// FOR PSP: SceIo... operators

void Get_DirList(char *path)
{
    int ret, fd;

    // Directory read
    fd = sceIoDopen(path);

    dlist_num = 0;
    ret = 1;

    while((ret > 0) && (dlist_num < MAXDIRNUM))
    {
	ret = sceIoDread(fd, &dlist[dlist_num]);

	if (dlist[dlist_num].name[0] == '.' && dlist[dlist_num].name[1] != '.')
	    continue;

	if (ret > 0)
	    dlist_num++;
    }
    sceIoDclose(fd);

    if (dlist_start >= dlist_num)
	dlist_start = dlist_num - 1;

    if (dlist_start < 0)         
	dlist_start = 0;

    if (dlist_curpos >= dlist_num)
	dlist_curpos = dlist_num - 1;

    if (dlist_curpos < 0)
	dlist_curpos = 0;
}

void GetDirList(char *root)
{
    int dfd;
    int current = 0;

    dfd = sceIoDopen(root);

    if (dfd > 0)
    {
	while (sceIoDread(dfd, &dirlist[current]) > 0)
	    current++;

	sceIoDclose(dfd);
    }
}

void Read_Key2()
{
    static int n = 0;

    sceCtrlReadBufferPositive(&ctl, 1);

    now_pad = ctl.Buttons;
    new_pad = now_pad & ~old_pad;

    if (old_pad == now_pad)
    {
	n++;

	if (n >= 25)
	{
	    new_pad = now_pad;
	    n = 20;
	}
    }
    else
    {
	n = 0;
	old_pad = now_pad;
    }
}

int Control_DirList(void)
{
    int i;

    char *temp;
    char line[BUFSIZ];
    char iwad_term[] = "IWAD";
    char pwad_term[] = "PWAD";

    FILE *file;

    Read_Key2();
    pgWaitV();

    if (new_pad & PSP_CTRL_UP)
    {
	if (dlist_curpos > 0)
	{
	    dlist_curpos--;

	    if (dlist_curpos < dlist_start) 
		dlist_start = dlist_curpos;
	}
	txtLoaded = 0;
	WADLoaded = 0;
    }

    if (new_pad & PSP_CTRL_DOWN)
    {
	if (dlist_curpos < (dlist_num - 1))
	{
	    dlist_curpos++;

	    if (dlist_curpos >= (dlist_start + PATHLIST_H))
		dlist_start++;
	}
	txtLoaded = 0;
	WADLoaded = 0;
    }

    if (new_pad & PSP_CTRL_CROSS)
    {
	txtLoaded = 0;
	WADLoaded = 0;

	if (dlist[dlist_curpos].type & TYPE_DIR) 
	{
	    if (dlist[dlist_curpos].name[0] == '.')
	    {
		if (now_depth > 0)
		{
		    for (i = 0; i < MAXPATH; i++)
		    {
			if (heretic_wad_dir[i] == 0)
			    break;
		    }
		    i--;

		    while (i > 4)
		    {
			if (heretic_wad_dir[i - 1] == '/')
			{
			    heretic_wad_dir[i] = 0;

			    break;
			}
			i--;
		    }
		    now_depth--;

		    dlist_start  = cbuf_start[now_depth];
		    dlist_curpos = cbuf_curpos[now_depth];

		    return 1;
		}
		return 3;
	    }
	    if (now_depth < MAXDEPTH) 
	    {
		strcat(heretic_wad_dir, dlist[dlist_curpos].name);
		strcat(heretic_wad_dir, "/");

		cbuf_start[now_depth] = dlist_start;
		cbuf_curpos[now_depth] = dlist_curpos;
		dlist_start = 0;
		dlist_curpos = 0;

		now_depth++;

		return 1;
	    }
	}
	else
	{
	    strcpy(check, heretic_wad_dir);
	    strcat(check, dlist[dlist_curpos].name);

	    file = fopen(check, "r");

	    if (file != NULL)
	    {
		while (fgets(line, sizeof(line), file))
		{
		    temp = line;

		    if (strncmp(iwad_term, temp, 4) == 0) 
		    {
			strcpy(target, check);

			return 3;
		    }
		}
	    }
/*
	    else
	    {
		perror(target); 
	    }
*/
	    fclose(file);

	    return -1;
	}
    }

    if (new_pad & PSP_CTRL_SQUARE)
    {
	txtLoaded = 0;

	if (dlist[dlist_curpos].type & TYPE_FILE)
	{
	    load_dehacked = 1;

	    strcpy(dehacked_file, heretic_wad_dir);
	    strcat(dehacked_file, dlist[dlist_curpos].name);
	}
	return 3;
    }

    if (new_pad & PSP_CTRL_CIRCLE)
    {
	txtLoaded = 0;

	load_dehacked = 0;
	load_extra_wad = 0;

	strcpy(dehacked_file, "");
	strcpy(extra_wad, "");
	strcpy(target, "");

	return 3;
    }

    if (new_pad & PSP_CTRL_START)
	return 2;

    if (new_pad & PSP_CTRL_TRIANGLE)
    {
	txtLoaded = 0;
	WADLoaded = 0;

	if (dlist[dlist_curpos].type & TYPE_FILE)
	{
	    load_extra_wad = 1;

	    strcpy(check, heretic_wad_dir);
	    strcat(check, dlist[dlist_curpos].name);

	    file = fopen(check, "r");

	    if (file != NULL)
	    {
		while (fgets(line, sizeof(line), file))
		{
		    temp = line;

		    if (strncmp(pwad_term, temp, 4) == 0) 
		    {
			strcpy(extra_wad, check);

			extra_wad_loaded = 1;

			return 3;
		    }
		    else
			load_extra_wad = 0;
		}
	    }
/*
	    else
	    {
		perror(target); 
	    }
*/
	    fclose(file);

	    return -1;
	}
	return 3;
    }

    if (new_pad & PSP_CTRL_LTRIGGER)
    {
	if (txtPos >= 0)
	    txtPos--;

	return 1;
    }

    if (new_pad & PSP_CTRL_RTRIGGER)
    {
	if (txtPos < 600)
	    txtPos++;

	return 1;
    }
    return 0;
}

void displayFile()
{
    int foundWAD = 0;
    int foundTxt = 0;
    int i = 0;

    if (dlist[dlist_curpos].type & TYPE_FILE)
    {
	for (i = 0; i < MAXPATH; i++) 
	{
	    if (dlist[dlist_curpos].name[i] == 0) 
		break;
	}
	i--;

	while (i > 2)
	{
	    if (dlist[dlist_curpos].name[i-1] == '.')
	    {
		if (dlist[dlist_curpos].name[i] == 't' || dlist[dlist_curpos].name[i] == 'T')
		    foundTxt = 1;
		else if (dlist[dlist_curpos].name[i] == 'w' || dlist[dlist_curpos].name[i] == 'W')
		    foundWAD = 1;

		break;
	    }
	    i--;
	}
    }

    if (foundTxt == 1)
    {
	if (txtLoaded == 0)
	{
	    txtPos = 0;

	    strcpy(path_tmp, heretic_wad_dir);
	    strcat(path_tmp, dlist[dlist_curpos].name);

	    FILE* fp = fopen(path_tmp, "r");

	    fseek(fp, SEEK_END, 0);
	    fseek(fp, SEEK_SET, 0);

	    i = 0;

	    while (!feof(fp) && i < 600)
	    {
		fgets(buf + 100 * i, 100, fp);
		i++;
	    }
	    fclose(fp);

	    txtLoaded = 1;
	}
	int j = 0;

	for (i = txtPos; i < txtPos + 12; i++)
	{
	    mh_print(30, 152 + j * 10, buf + 100 * i, rgb2col(50, 255, 50), 0, 0);

	    j++;
	}
    }
    else if (foundWAD == 1)
    {
	if (WADLoaded == 0)
	{
	    strcpy(path_tmp, heretic_wad_dir);
	    strcat(path_tmp, dlist[dlist_curpos].name);

	    W_CheckSize(1);

	    if(fsize == 4095992)
	    {
		mh_print(47, 111, "HERETIC IWAD VERSION: HERETIC BETA DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH SHAREWARE & BETA VERSIONS OF HERETIC. PLEASE REGISTER !",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 5120300)
	    {
		mh_print(47, 111, "HERETIC IWAD VERSION: HERETIC SHAREWARE v1.0 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH SHAREWARE & BETA VERSIONS OF HERETIC. PLEASE REGISTER !",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 5120920)
	    {
		mh_print(47, 111, "HERETIC IWAD VERSION: HERETIC SHAREWARE v1.2 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH SHAREWARE & BETA VERSIONS OF HERETIC. PLEASE REGISTER !",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 11096488)
	    {
		mh_print(47, 111, "HERETIC IWAD VERSION: HERETIC REGISTERED v1.0 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 11095516)
	    {
		mh_print(47, 111, "HERETIC IWAD VERSION: HERETIC REGISTERED v1.2 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 14189976)
	    {
		mh_print(47, 111, "HERETIC IWAD VERSION: HERETIC S.O.T.S.R. v1.3 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize != 4095992  && fsize != 5120300  && fsize != 5120920  && fsize != 11096488 &&
		    fsize != 11095516 && fsize != 14189976)
	    {
		mh_print(47, 111, "!!! WARNING: THIS WAD CANNOT BE USED AS A MAIN GAME IWAD  -  UNKNOWN FILE !!!",
			rgb2col(255, 0, 0), 0, 0);
	    }
//	    WADLoaded = 1;
	}
    }
}

void drawDirectory()
{
    int	i, col;

    pgFillvram(0);

    strncpy(path_tmp, heretic_wad_dir, 40);

    mh_print(0, 0,
    "                                 PSP-HERETIC RELEASE 1 BY NITR8                                 ",
	    rgb2col(255, 255, 0), 80000, 75);

    mh_print(47, 12, "SELECT THE GAME'S MAIN IWAD. YOU CAN ALSO SELECT A PWAD AND DEH FILE TO LOAD.",
	    rgb2col(255, 255, 255), 0, 0);

    mh_print(47, 67, "---------", rgb2col(255, 255, 255), 0, 0);
    mh_print(47, 73, "BUTTONS:", rgb2col(50, 255, 50), 0, 0);
    mh_print(47, 79, "---------", rgb2col(255, 255, 255), 0, 0);

    mh_print(92, 63, "|", rgb2col(255, 255, 255), 0, 0);
    mh_print(92, 73, "|", rgb2col(255, 255, 255), 0, 0);
    mh_print(92, 83, "|", rgb2col(255, 255, 255), 0, 0);

    mh_print(102, 63, "SQUARE  - select DEH file", rgb2col(255, 255, 255), 0, 0);
    mh_print(102, 73, "START   - launch the game", rgb2col(255, 255, 255), 0, 0);
    mh_print(102, 83, "CIRCLE  - clear entries", rgb2col(255, 255, 255), 0, 0);

    mh_print(242, 63, "CROSS     - select main IWAD", rgb2col(255, 255, 255), 0, 0);
    mh_print(242, 73, "TRIANGLE  - select opt. PWAD", rgb2col(255, 255, 255), 0, 0);
    mh_print(242, 83, "L/R-TRIG. - scroll text UP & DOWN", rgb2col(255, 255, 255), 0, 0);

    i = dlist_start;

    while (i < (dlist_start + PATHLIST_H))
    {
	if (i < dlist_num)
	{
	    col = rgb2col(255, 255, 255);

	    if (dlist[i].type & TYPE_DIR)
		col = rgb2col(255, 255, 0);

	    if (i == dlist_curpos)
		col = rgb2col(255, 0, 0);

	    strncpy(path_tmp, dlist[i].name, 40);

	    mh_print(47, ((i - dlist_start) + 2) * 10 + 8, path_tmp, col, 0, 0);
	}
	i++;
    }

    if (dlist_start != 0)
	mh_print(223, 28, "-", rgb2col(255, 255, 50), 0, 0);
	
    if (i != dlist_num && dlist_num >= PATHLIST_H)
	mh_print(223, 48, "+", rgb2col(255, 255, 50), 0, 0);

    displayFile();

    mh_print(232, 28, "|", rgb2col(255, 255, 255), 0, 0);
    mh_print(232, 38, "|", rgb2col(255, 255, 255), 0, 0);
    mh_print(232, 48, "|", rgb2col(255, 255, 255), 0, 0);

    mh_print(232, 63, "|", rgb2col(255, 255, 255), 0, 0);
    mh_print(232, 73, "|", rgb2col(255, 255, 255), 0, 0);
    mh_print(232, 83, "|", rgb2col(255, 255, 255), 0, 0);

    mh_print(242, 28, "Main IWAD:", rgb2col(50, 255, 50), 0, 0);
    mh_print(292, 28, target, rgb2col(50, 255, 50), 0, 0);
    mh_print(242, 38, "opt. PWAD:", rgb2col(50, 255, 50), 0, 0);
    mh_print(292, 38, extra_wad, rgb2col(50, 255, 50), 0, 0);

    mh_print(242, 48, ".DEH-File:", rgb2col(50, 255, 50), 0, 0);
    mh_print(292, 48, dehacked_file, rgb2col(50, 255, 50), 0, 0);

    mh_print(47, 20, "-----------------------------------------------------------------------------",
	    rgb2col(255, 255, 255), 0, 0);

    mh_print(47, 56, "-----------------------------------------------------------------------------",
	    rgb2col(255, 255, 255), 0, 0);

    mh_print(47, 90, "-----------------------------------------------------------------------------",
	    rgb2col(255, 255, 255), 0, 0);

    mh_print(47, 97, "WARNING: LOADING IWAD'S AS PWAD, VICE VERSA OR NON-HERETIC WADS, MAY CRASH!!!",
	    rgb2col(255, 0, 0), 0, 0);

    mh_print(47, 104, "-----------------------------------------------------------------------------",
	    rgb2col(255, 255, 255), 0, 0);

    mh_print(47, 118, "-----------------------------------------------------------------------------",
	    rgb2col(255, 255, 255), 0, 0);

    mh_print(47, 132, "-----------------------------------------------------------------------------",
	    rgb2col(255, 255, 255), 0, 0);

    mh_print(0, 139,
    "                                    | TEXT LISTING BELOW |                                     ",
	    rgb2col(255, 255, 0), 0, 0);

    mh_print(0, 146,
    "                                    ----------------------",
	    rgb2col(255, 255, 0), 0, 0);

    pgScreenFlipV();
}

void CreateDirs(void)
{
    char createPWADDir[121];
    char createIWADDir[121];

    strcpy(createPWADDir, PWADPath);
    strcpy(createIWADDir, IWADPath);

    createPWADDir[120] = 0;
    createIWADDir[120] = 0;

    mkdir(createPWADDir, S_IRWXU | S_IRWXG | S_IRWXO);
    mkdir(createIWADDir, S_IRWXU | S_IRWXG | S_IRWXO);
}

void enterMenu(char *path)
{
    CreateDirs();

    strcat(heretic_wad_dir, "WADS/");

    Get_DirList(heretic_wad_dir);

    dlist_start = 0;
    dlist_curpos = 0;
    now_depth = 0;
	
    while (1)
    {
	drawDirectory();
	
	switch (Control_DirList())
	{
	    case 1:
		Get_DirList(heretic_wad_dir);

		break;

	    case 2:
		pgFillvram(0);
		pgScreenFlipV();

		pspDebugScreenInit();

		D_DoomMain (); 

		break;

	    case 3:
		break;
	}
    }    
}

