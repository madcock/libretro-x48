#include "libretro.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/time.h>

#include "x48/rpl.h"
#include "x48/plateform.h"
#include "x48/gif.h"

#include "zip/ziptool.h"

#define maxByCycle 400 // 50 fois par frame

int borderX = 1;
int borderY = 1;
int zoom = 1;

extern unsigned char background_gif[];
extern unsigned int background_gif_len;

bool old_mouse_l = false;

u8 *snapshot = NULL;
u32 snapshotLength;


char fullscreen = 1;

void updateFromEnvironnement();

int height, width;
u16 *pixels;
u16 bgcolor;

int winH, winW;

u16 *background;

int order = 0; // For the surrounder pixel

char Core_Key_Sate[512];
char Core_old_Key_Sate[512];

char file_path[2048];
int loadFile_flag = 0;

retro_log_printf_t log_cb;
retro_video_refresh_t video_cb;

static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

bool loadGame(void);

#define RGB15(R, G, B) ((((R) & 0xF8) << 8) | (((G) & 0xFC) << 3) | (((B) & 0xF8) >> 3))

// end of crocods variable

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
    va_list va;

    (void)level;

    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

static int KeySymToCPCKey[RETROK_LAST];
static int KeySymToCPCKey[RETROK_LAST];

struct CrocoKeyMap {
    unsigned port;
    unsigned index;

    int scanCode;
} crocokeymap[] = {
    {0, RETRO_DEVICE_ID_JOYPAD_A,      BUTTON_A        },
    {0, RETRO_DEVICE_ID_JOYPAD_B,      BUTTON_B        },
    {0, RETRO_DEVICE_ID_JOYPAD_UP,     BUTTON_UP       },
    {0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  BUTTON_RIGHT    },
    {0, RETRO_DEVICE_ID_JOYPAD_LEFT,   BUTTON_LEFT     },
    {0, RETRO_DEVICE_ID_JOYPAD_DOWN,   BUTTON_DOWN     },
    {0, RETRO_DEVICE_ID_JOYPAD_X,      BUTTON_C        },
    {0, RETRO_DEVICE_ID_JOYPAD_Y,      BUTTON_D        },    // 7
    {0, RETRO_DEVICE_ID_JOYPAD_L,      BUTTON_E        },
    {0, RETRO_DEVICE_ID_JOYPAD_R,      BUTTON_F        },
    {0, RETRO_DEVICE_ID_JOYPAD_SELECT, BUTTON_EVAL     },
    {0, RETRO_DEVICE_ID_JOYPAD_START,  BUTTON_ENTER    },    // 11

    {1, RETRO_DEVICE_ID_JOYPAD_A,      BUTTON_A        },
    {1, RETRO_DEVICE_ID_JOYPAD_B,      BUTTON_B        },
    {1, RETRO_DEVICE_ID_JOYPAD_UP,     BUTTON_UP       },
    {1, RETRO_DEVICE_ID_JOYPAD_RIGHT,  BUTTON_RIGHT    },
    {1, RETRO_DEVICE_ID_JOYPAD_LEFT,   BUTTON_LEFT     },
    {1, RETRO_DEVICE_ID_JOYPAD_DOWN,   BUTTON_DOWN     },
    {1, RETRO_DEVICE_ID_JOYPAD_X,      LAST_BUTTON     },
    {1, RETRO_DEVICE_ID_JOYPAD_Y,      LAST_BUTTON     },
    {1, RETRO_DEVICE_ID_JOYPAD_L,      LAST_BUTTON     },
    {1, RETRO_DEVICE_ID_JOYPAD_R,      LAST_BUTTON     },
    {1, RETRO_DEVICE_ID_JOYPAD_SELECT, BUTTON_EVAL     },
    {1, RETRO_DEVICE_ID_JOYPAD_START,  BUTTON_ENTER    }

};



void retro_init(void)
{
    char *savedir = NULL;
    int i;

    environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &savedir);

    width = 131;
    height = 64; // DISP_ROWS

    bgcolor =  RGB15(123, 133, 97);

    // pixels = (u16*)malloc(width*height*2);  // <-- correct


//
//

    for (i = 0; i < RETROK_LAST; i++) {
        KeySymToCPCKey[i] = LAST_BUTTON;
    }


    KeySymToCPCKey[RETROK_a] = BUTTON_A;

    KeySymToCPCKey[RETROK_0] = BUTTON_0;
    KeySymToCPCKey[RETROK_1] = BUTTON_1;
    KeySymToCPCKey[RETROK_2] = BUTTON_2;
    KeySymToCPCKey[RETROK_3] = BUTTON_3;
    KeySymToCPCKey[RETROK_4] = BUTTON_4;
    KeySymToCPCKey[RETROK_5] = BUTTON_5;
    KeySymToCPCKey[RETROK_6] = BUTTON_6;
    KeySymToCPCKey[RETROK_7] = BUTTON_7;
    KeySymToCPCKey[RETROK_8] = BUTTON_8;
    KeySymToCPCKey[RETROK_9] = BUTTON_9;

    KeySymToCPCKey[RETROK_d] = BUTTON_D;
    KeySymToCPCKey[RETROK_e] = BUTTON_E;
    KeySymToCPCKey[RETROK_f] = BUTTON_F;


    KeySymToCPCKey[RETROK_RETURN] = BUTTON_ENTER;
    KeySymToCPCKey[RETROK_PLUS] = BUTTON_PLUS;
    KeySymToCPCKey[RETROK_MINUS] = BUTTON_MINUS;
    KeySymToCPCKey[RETROK_ASTERISK] = BUTTON_MUL;
    KeySymToCPCKey[RETROK_SLASH] = BUTTON_DIV;

//

    disp.disp_image = malloc(sizeof(disp_image_t));
    disp.disp_image->bytes_per_line = NIBBLES_PER_ROW;

    disp.menu_image = malloc(sizeof(menu_image_t));
    disp.menu_image->bytes_per_line = NIBBLES_PER_ROW;

    disp.mapped = 1;

    resetOnStartup = 1;

    init_emulator();


    init_plateform();

    emulate_start();




    u32 bckWidth, bckHeight;
    ReadBackgroundGifInfo(&bckWidth, &bckHeight, (unsigned char *)&background_gif, background_gif_len);
    background = (u16 *)malloc(bckWidth * bckHeight * 2);
    ReadBackgroundGif16(background, (unsigned char *)&background_gif, background_gif_len);

    pixels = (u16 *)malloc(bckWidth * bckHeight * 2);

    u32 size = (width + borderX * 2) * (height + borderY * 2) * 20;
    for (i = 0; i < size / 2; i++) {
        pixels[i] = bgcolor;
    }
    winW = width + borderX * 2;
    winH = height + borderY * 2;


    fprintf(stderr, "Background %dx%d %p\n", bckWidth, bckHeight, background);
    fprintf(stderr, "%d > %d\n", size, bckWidth * bckHeight * 2);

} /* retro_init */

void retro_deinit(void)
{
    exit_emulator();
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port;
    (void)device;


}

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name     = "x48";
    info->need_fullpath    = false;
    info->valid_extensions = NULL; //  "lib";

#ifdef GIT_VERSION
    info->library_version = "git" GIT_VERSION;
#else
    info->library_version = "svn";
#endif

}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    info->timing.fps = 60.0;
    info->timing.sample_rate = 44100.0;

    info->geometry.base_width   = width + borderX * 2;
    info->geometry.base_height  = height + borderY * 2;

    info->geometry.max_width    = 800;
    info->geometry.max_height   = 600;

    info->geometry.aspect_ratio = width / height;
}

void retro_set_environment(retro_environment_t cb)
{
    struct retro_log_callback logging;

    environ_cb = cb;

    if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
        log_cb = logging.log;
    } else {
        log_cb = fallback_log;
    }

    log_cb = fallback_log;

    bool no_rom = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

    static const struct retro_variable vars[] = {
        // { "crocods_computertype", "Machine Type (Restart); CPC 464|CPC 6128" },
        // { "crocods_vdpsync", "VDP Sync Type (Restart); Auto|50Hz|60Hz" },
        {"crocods_greenmonitor", "Color Monitor; color|green"         },
        {"crocods_resize",       "Resize; Auto|320x200|Overscan"      },
        {"crocods_hack",         "Speed hack; no|yes"                 },
        {NULL,                   NULL                                 },
    };

    cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)vars);


    static const struct retro_controller_description port[] = {
        {"RetroPad",      RETRO_DEVICE_JOYPAD     },
        {"RetroKeyboard", RETRO_DEVICE_KEYBOARD   },
    };

    static const struct retro_controller_info ports[] = {
        {port, 2},
        {port, 2},
        {NULL, 0},
    };

    cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void *)ports);

} /* retro_set_environment */

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

void retro_reset(void)
{

}


void updateFromEnvironnement()
{


}

void retro_key_down(int key)
{
    log_cb(RETRO_LOG_INFO, "key: %d\n", key);
}

static char keyPressed[24] = {0};

void loadSnapshot(void)
{

    if (snapshot != NULL) {
        LireSnapshotMem(snapshot);
    }
}

void fullScreenEnter(void)
{
    int i;
    struct retro_game_geometry geometry;

    borderX = 1;
    borderY = 1;
    zoom = 1;

    geometry.base_width   = width + borderX * 2;
    geometry.base_height  = height + borderY * 2;
    geometry.aspect_ratio = width / height;

    environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);

    fullscreen = 1;

    u32 size = (width + borderX * 2) * (height + borderY * 2) * 20;

    for (i = 0; i < size / 2; i++) {
        pixels[i] = bgcolor;
    }

    winW = width + borderX * 2;
    winH = height + borderY * 2;

    fprintf(stderr, "Go to fullscreen: PC:%d\n", (int)saturn.PC);
} /* fullScreenEnter */

void fullScreenLeave(void)
{
    int i;
    struct retro_game_geometry geometry;

    borderX = 20;
    borderY = 65 + 8;
    zoom = 2;

    geometry.base_width = 600;
    geometry.base_height = 420;
    geometry.aspect_ratio = 600.0 / 420.0;

    environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);

    fullscreen = 0;

    memcpy(pixels, background, 600 * 420 * 2);

    winW = 600;
    winH = 420;

    fprintf(stderr, "Go to windowed: PC:%d\n", (int)saturn.PC);


} /* fullScreenLeave */

void retro_run(void)
{
    static int pos = 0;


    static bool updated = false;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
        updateFromEnvironnement();
    }

    input_poll_cb();

    int i;

/*
 * struct CrocoKeyMap {
 *  unsigned port;
 *  unsigned index;
 *
 *  int scanCode;
 */

    for (i = 0; i < 24; i++) {
        int scanCode = crocokeymap[i].scanCode;

        if (scanCode != LAST_BUTTON) {
            if (input_state_cb(crocokeymap[i].port, RETRO_DEVICE_JOYPAD, 0, crocokeymap[i].index)) {
                if (keyPressed[i] == 0) {
                    keyPressed[i] = 1;

                    //      int retour = LoadObject("/Users/miguelvanhove/Downloads/diam20/Diamonds");
                    // button_pressed(BUTTON_EVAL);

                    button_pressed(scanCode);
                }
            } else {
                if (keyPressed[i] == 1) {
                    keyPressed[i] = 0;

                    button_released(scanCode);
                }
            }
        }
    }



    for (i = 0; i < RETROK_LAST; i++) {
        int scanCode = KeySymToCPCKey[i];

        if (scanCode != LAST_BUTTON) {
            int state = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, i);

            if (state != Core_Key_Sate[i]) {
                Core_Key_Sate[i] = state;

                if ((scanCode == BUTTON_F) && (state == 1)) {
                    if (fullscreen) {
                        fullScreenLeave();
                    } else {
                        fullScreenEnter();


                    }
                }


                else if ((scanCode == BUTTON_E) && (state == 1)) {


                fprintf(stderr,  " Load binary \n");

                    // int retour = LoadObject("/Users/miguelvanhove/Downloads/diam20/Diamonds");
                    // int retour = read_bin_file("/Users/miguelvanhove/Downloads/diam20a/DIAMONDS");

                int retour = read_bin_file("/Users/miguelvanhove/Dropbox/temp/hp/beaubour.var");

                }

                else if ((scanCode == BUTTON_D) && (state == 1)) {


                fprintf(stderr,  " Load binary (old) \n");

                    // int retour = LoadObject("/Users/miguelvanhove/Downloads/diam20/Diamonds");
                    int retour = LoadObject("/Users/miguelvanhove/Downloads/diam20a/DIAMONDS");
                }

// }
                else if (state == 1) {
                    button_pressed(scanCode);
                } else {
                    button_released(scanCode);
                }

            }
        }
    }


    int16_t mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    int16_t mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
    bool mouse_l    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);


    if (mouse_x) {
        fprintf(stderr, "Mouse X: %d\n", mouse_x);
    }
    if (mouse_y) {
        fprintf(stderr, "Mouse Y: %d\n", mouse_y);
    }

    // fprintf(stderr, "Mouse left: %d,%d (%d)\n", mouse_x, mouse_y, mouse_l);


    if (mouse_l != old_mouse_l) {
        old_mouse_l = mouse_l;


        if (mouse_l) {
            if (fullscreen) {
                // fprintf(stderr, "Leave fullscreen\n");

                fullScreenLeave();

                fullscreen = 0;
            } else {
                // fullscreen=1;

                int pointer_x = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
                int pointer_y = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

                fprintf(stderr,  "Pointer: (%04X, %04X).\n", pointer_x, pointer_y);

                pointer_x = ((pointer_x + 0x7fff) * winW) / 65536;
                pointer_y = ((pointer_y + 0x7fff) * winH) / 65536;

                fprintf(stderr,  "         (%6d, %6d) (%d,%d).\n", pointer_x, pointer_y, winW, winH);


                if ((pointer_x >= 20) && (pointer_x <= 281) && (pointer_y >= 67) && (pointer_y <= 207)) {
                    fullScreenEnter();
                }

                // fprintf(stderr, "Go fullscreen\n");

                // fullScreenEnter();



            }
        }

    }


//    bool pointer_pressed = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
//    int16_t pointer_x = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
//    int16_t pointer_y = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
//    if (pointer_pressed) {
//       fprintf(stderr,  "Pointer: (%6d, %6d).\n", pointer_x, pointer_y);

// }


    if (loadFile_flag != 0) {
        switch (loadFile_flag) {
            case 420:
                // button_released(BUTTON_EVAL);
                // saturn.PC = 0x00000;
                break;
            case 240:
                fprintf(stderr, "Load %s\n", file_path);
                // int retour = LoadObject("/Users/miguelvanhove/Downloads/diam20/Diamonds");
                int retour = LoadObject(file_path);

                fprintf(stderr, "Retour: %d\n", retour);
                break;
            case 200:
                // button_pressed(BUTTON_ON);
                break;
            case 150:
                // button_released(BUTTON_ON);
                break;
            case 80:
                button_pressed(BUTTON_EVAL);
                break;
            case 10:
                button_released(BUTTON_EVAL);
                break;
        } /* switch */
        loadFile_flag--;
    }



    emulate_frame();

    // fprintf(stderr, "Frame: PC:%d\n", (int)saturn.PC);

    // fprintf(stderr, "Frame: update:%ld, info:%d, lines:%d, offset:%d\n", disp.display_update, disp.disp_info, disp.lines, disp.offset);

    disp.display_update = 0; // Necessaire ????

    // disp.disp_image->data[rand()%(height*NIBBLES_PER_ROW)]=rand()%255;

    if (zoom > 1) {

        int x, y;
        int npos = 0;

        u8 *data;
        int y0;

        int zoomX, zoomY;

        int yb = disp.lines + 1;

        int spos0 = borderY * winW + borderX;

        for (y = 0; y < height; y++) {

            if (y < yb) {
                y0 = 0;
                data = (u8 *)disp.disp_image->data;
            } else {
                y0 = yb;
                data = (u8 *)disp.menu_image->data;
            }

            for (zoomY = 0; zoomY < zoom; zoomY++) {

                npos = (y - y0) * NIBBLES_PER_ROW;

                for (x = 0; x < NIBBLES_PER_ROW - 1; x++) {
                    u8 nibble = data[npos];
                    int z;

                    if (x == NIBBLES_PER_ROW - 2) {
                        for (z = 0; z < 4; z++) {
                            for (zoomX = 0; zoomX < zoom; zoomX++) {

                                if ((3 - z) < 2) {
                                    pixels[spos0 + x * 4 * zoom + (3 - z) * zoom + zoomX] = ((nibble & 1) == 1) ? RGB15(0, 0, 0) : bgcolor;
                                }
                            }

                            nibble = nibble / 4;
                        }
                    } else {
                        for (z = 0; z < 4; z++) {
                            for (zoomX = 0; zoomX < zoom; zoomX++) {
                                pixels[spos0 + x * 4 * zoom + (3 - z) * zoom + zoomX] = ((nibble & 1) == 1) ? RGB15(0, 0, 0) : bgcolor;
                            }

                            nibble = nibble / 4;
                        }
                    }


                    npos++;
                }
                spos0 += winW;

            }


        }


    } else {


        int x, y;
        int spos = 0;
        int npos = 0;

        u8 *data;
        int y0;

        int yb = disp.lines + 1;

        for (y = 0; y < height; y++) {

            if (y < yb) {
                y0 = 0;
                data = (u8 *)disp.disp_image->data;
            } else {
                y0 = yb;
                data = (u8 *)disp.menu_image->data;
            }

            npos = (y - y0) * NIBBLES_PER_ROW;
            spos = (y + borderY) * (width + borderX * 2) + borderX;

            for (x = 0; x < NIBBLES_PER_ROW; x++) {
                u8 nibble = data[npos];

                int z;
                for (z = 0; z < 4; z++) {
                    // pixels[spos+3-z] = ((nibble & 1)==1) ? RGB15(86,90,80) : RGB15(123,133,97);
                    pixels[spos + 3 - z] = ((nibble & 1) == 1) ? RGB15(0, 0, 0) : bgcolor;
                    nibble = nibble / 4;
                }
                spos += 4;

                npos++;
            }
        }

    }

// memcpy(pixels, disp.disp_image->data, height*width);

/*
 *  pixels[pos]=bgcolor;
 *
 *  switch(order) {
 *  case 0:
 *      pos++;
 *      if (pos==width+BORDER*2-1) {
 *          order++;
 *      }
 *      break;
 *  case 1:
 *      pos+=width+BORDER*2;
 *      if (pos==(width+BORDER*2)*(height+BORDER*2)-1) {
 *          order++;
 *      }
 *      break;
 *  case 2:
 *      pos--;
 *      if (pos==(width+BORDER*2)*((height-1)+BORDER*2)) {
 *          order++;
 *      }
 *      break;
 *  case 3:
 *      pos-=width+BORDER*2;
 *      if (pos==0) {
 *          order=0;
 *      }
 *      break;
 *  }
 *
 *  pixels[pos]=rand();
 */

    video_cb(pixels, winW, winH, winW * 2);



// if (sndSamplerToPlay>0) {

//     GB_sample_t sample[sndSamplerToPlay];
//     int x;

//     crocods_copy_sound_buffer(&gb, sample, sndSamplerToPlay);
//     audio_batch_cb((int16_t*)&sample, sndSamplerToPlay);
// }


} /* retro_run */




bool retro_load_game(const struct retro_game_info *info)
{
    struct retro_frame_time_callback frame_cb;
    struct retro_input_descriptor desc[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left"  },
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up"    },
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down"  },
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Pause" },
        {0},
    };

    log_cb(RETRO_LOG_INFO, "begin of load games %s\n", info);

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    // Init pixel format

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        log_cb(RETRO_LOG_INFO, "begin of load games 2\n");
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "XRGG565 is not supported.\n");
        return 0;
    }

    log_cb(RETRO_LOG_INFO, "begin of load gamer s\n");


    if (info != NULL) {

        strcpy(file_path, info->path);
        loadGame();
    }


    log_cb(RETRO_LOG_INFO, "end of load gamer s\n");

    return true;
} /* retro_load_game */

bool loadGame(void)
{
    FILE *fic;

    u8 *dsk;
    long dsk_size;

    fic = fopen(file_path, "rb");
    if (fic == NULL) {
        return 0;
    }
    fseek(fic, 0, SEEK_END);
    dsk_size = ftell(fic);
    fseek(fic, 0, SEEK_SET);

    dsk = (u8 *)malloc(dsk_size);
    if (dsk == NULL) {
        return 0;
    }
    fread(dsk, 1, dsk_size, fic);
    fclose(fic);

    char header[32];

    if (dsk_size < 32) {
        return false;
    }

    memcpy(header, dsk, 32);

    if (!memcmp(header, "84PH", 4)) {

        snapshotLength = (u32)dsk_size;

        snapshot = (u8 *)malloc(snapshotLength);
        memcpy(snapshot, dsk, snapshotLength);

        if (snapshot != NULL) {
            loadSnapshot();
            return true;
        }
    }

// Send to Stack

    fprintf(stderr, "Load %s\n", file_path);

    loadFile_flag = 600;

    log_cb(RETRO_LOG_INFO, "send file %s to Stack\n", file_path);


    return true;

} /* loadGame */

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
    return RETRO_REGION_PAL;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
    (void)type;
    (void)info;
    (void)num;
    return false;
}

size_t retro_serialize_size(void)
{
    return getSnapshotSize();
}

bool retro_serialize(void *data_, size_t size)
{
    u32 len;
    char *buffer = getSnapshot(&len);

    fprintf(stderr, "retro_serialize %d vs %d\n", (int)size, len);


    if (buffer != NULL) {
        if (len > size) {
            free(buffer);
            return false;
        }

        memcpy(data_, buffer, len);

        fprintf(stderr, "will free %p\n", buffer);


        free(buffer);

        return true;
    }
    return false;
} /* retro_serialize */

bool retro_unserialize(const void *data_, size_t size)
{
    LireSnapshotMem((u8 *)data_);

    return true;
}


void * retro_get_memory_data(unsigned id)
{
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    return 0;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    (void)index;
    (void)enabled;
    (void)code;
}
