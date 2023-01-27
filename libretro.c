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
#include "zip/miniz.h"

#define RETRO_DEVICE_AUTO     RETRO_DEVICE_JOYPAD
#define RETRO_DEVICE_GAMEPAD  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)
#define RETRO_DEVICE_ARKANOID RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_MOUSE, 0)
#define RETRO_DEVICE_ZAPPER   RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_POINTER, 0)

#define PXtoPOS(x) ((x) / 1)
#define PYtoPOS(y) ((y) / 1)



void draw_zoomcursor(unsigned short int *surface, int x, int y);
void draw_cross(unsigned short int *surface, int x, int y);
void restore_background_cross(unsigned short int *surface, int x, int y);

#define maxByCycle 400            // 50 fois par frame

int borderX = 1;
int borderY = 1;
int zoom = 1;

int last_press;

int px = -1, py = -1;

extern unsigned char background_gif[];
extern unsigned int background_gif_len;

bool old_mouse_l = false;

u8 *snapshot = NULL;
u32 snapshotLength;

int object_size;
char *object;

char fullscreen = 1;

void updateFromEnvironnement();

int height, width;
u16 *pixels;
u16 bgcolor;

int winH, winW;

u16 *background;

int order = 0; // For the surrounder pixel

int p_x, p_y;

char keypad_pressed[HANDLED_BUTTON];

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
    {0, RETRO_DEVICE_ID_JOYPAD_A,      BUTTON_A},
    {0, RETRO_DEVICE_ID_JOYPAD_B,      BUTTON_B},
    {0, RETRO_DEVICE_ID_JOYPAD_UP,     BUTTON_UP},
    {0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  BUTTON_RIGHT},
    {0, RETRO_DEVICE_ID_JOYPAD_LEFT,   BUTTON_LEFT},
    {0, RETRO_DEVICE_ID_JOYPAD_DOWN,   BUTTON_DOWN},
    {0, RETRO_DEVICE_ID_JOYPAD_X,      BUTTON_C},
    {0, RETRO_DEVICE_ID_JOYPAD_Y,      BUTTON_D},            // 7
    {0, RETRO_DEVICE_ID_JOYPAD_L,      BUTTON_E},
    {0, RETRO_DEVICE_ID_JOYPAD_R,      BUTTON_F},
    {0, RETRO_DEVICE_ID_JOYPAD_SELECT, BUTTON_EVAL},
    {0, RETRO_DEVICE_ID_JOYPAD_START,  BUTTON_ENTER},        // 11

    {1, RETRO_DEVICE_ID_JOYPAD_A,      BUTTON_A},
    {1, RETRO_DEVICE_ID_JOYPAD_B,      BUTTON_B},
    {1, RETRO_DEVICE_ID_JOYPAD_UP,     BUTTON_UP},
    {1, RETRO_DEVICE_ID_JOYPAD_RIGHT,  BUTTON_RIGHT},
    {1, RETRO_DEVICE_ID_JOYPAD_LEFT,   BUTTON_LEFT},
    {1, RETRO_DEVICE_ID_JOYPAD_DOWN,   BUTTON_DOWN},
    {1, RETRO_DEVICE_ID_JOYPAD_X,      LAST_BUTTON},
    {1, RETRO_DEVICE_ID_JOYPAD_Y,      LAST_BUTTON},
    {1, RETRO_DEVICE_ID_JOYPAD_L,      LAST_BUTTON},
    {1, RETRO_DEVICE_ID_JOYPAD_R,      LAST_BUTTON},
    {1, RETRO_DEVICE_ID_JOYPAD_SELECT, BUTTON_EVAL},
    {1, RETRO_DEVICE_ID_JOYPAD_START,  BUTTON_ENTER}

};

struct keyScr {
    int x, y;
    int w, h;
} keyScrs[LAST_BUTTON + 1] = {
    {19, 270, 32, 21}, //  BUTTON_A        0
    {65, 270, 32, 21}, // BUTTON_B        1
    {111, 270, 32, 21}, // BUTTON_C        2
    {157, 270, 32, 21}, // BUTTON_D        3
    {203, 270, 32, 21}, // BUTTON_E        4
    {249, 270, 32, 21}, // BUTTON_F        5

    {319, 42, 32, 24}, // BUTTON_MTH      6
    {364, 42, 32, 24}, // BUTTON_PRG      7
    {411, 42, 32, 24}, // BUTTON_CST      8
    {457, 42, 32, 24}, // BUTTON_VAR      9
    {504, 42, 32, 24}, // BUTTON_UP       10
    {550, 42, 32, 24}, // BUTTON_NXT      11

    {319, 87, 32, 24}, // BUTTON_COLON    12
    {364, 87, 32, 24}, // BUTTON_STO      13
    {411, 87, 32, 24}, // BUTTON_EVAL     14
    {457, 87, 32, 24}, // BUTTON_LEFT     15
    {504, 87, 32, 24}, // BUTTON_DOWN     16
    {550, 87, 32, 24}, // BUTTON_RIGHT    17

    {319, 132, 32, 24}, // BUTTON_SIN      18
    {364, 132, 32, 24}, // BUTTON_COS      19
    {411, 132, 32, 24},// BUTTON_TAN      20
    {457, 132, 32, 24}, // BUTTON_SQRT     21
    {504, 132, 32, 24}, // BUTTON_POWER    22
    {550, 132, 32, 24}, // BUTTON_INV      23

    {319, 177, 77, 24}, // BUTTON_ENTER    24
    {411, 177, 32, 24}, // BUTTON_NEG      25
    {457, 177, 32, 24}, // BUTTON_EEX      26
    {504, 177, 32, 24}, // BUTTON_DEL      27
    {550, 177, 32, 24}, // BUTTON_BS       28

    {319, 222, 32, 24}, // BUTTON_ALPHA    29
    {371, 222, 42, 24}, // BUTTON_7        30
    {427, 222, 42, 24}, // BUTTON_8        31
    {483, 222, 42, 24}, // BUTTON_9        32
    {539, 222, 42, 24}, // BUTTON_DIV      33

    {319, 267, 32, 24}, // BUTTON_SHL      34
    {371, 267, 42, 24}, // BUTTON_4        35
    {427, 267, 42, 24}, // BUTTON_5        36
    {483, 267, 42, 24}, // BUTTON_6        37
    {539, 267, 42, 24}, // BUTTON_MUL      38

    {319, 312, 32, 24}, // BUTTON_SHR      39
    {371, 312, 42, 24}, // BUTTON_1        40
    {427, 312, 42, 24}, // BUTTON_2        41
    {483, 312, 42, 24}, // BUTTON_3        42
    {539, 312, 42, 24}, // BUTTON_MINUS    43

    {319, 357, 32, 24}, // BUTTON_ON       44
    {371, 357, 42, 24}, // BUTTON_0        45
    {427, 357, 42, 24}, // BUTTON_PERIOD   46
    {483, 357, 42, 24}, // BUTTON_SPC      47
    {539, 357, 42, 24} // BUTTON_PLUS     48
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

static unsigned input_type[4];

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    if (port >= 4)
        return;

    input_type[port] = device;
}

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name = "x48";
    info->need_fullpath = false;
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

    info->geometry.base_width = width + borderX * 2;
    info->geometry.base_height = height + borderY * 2;

    info->geometry.max_width = 800;
    info->geometry.max_height = 600;

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
        {"crocods_greenmonitor", "Color Monitor; color|green"},
        {"crocods_resize",       "Resize; Auto|320x200|Overscan"},
        {"crocods_hack",         "Speed hack; no|yes"},
        {NULL,                   NULL},
    };

    cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)vars);

    static const struct retro_controller_description port[] = {
        {"Auto", RETRO_DEVICE_AUTO},
        {"Gamepad", RETRO_DEVICE_GAMEPAD},
        {"Arkanoid", RETRO_DEVICE_ARKANOID},
        {"Zapper", RETRO_DEVICE_ZAPPER},
        {NULL, 0}
    };

    static const struct retro_controller_info ports[] = {
        {port, 4},
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
    saturn.PC = 0;
}


void updateFromEnvironnement()
{


}

void retro_key_down(int key)
{
    log_cb(RETRO_LOG_INFO, "key: %d\n", key);
}

static char keyPressed[24] = {0};


void fullScreenEnter(void)
{
    int i;
    struct retro_game_geometry geometry;

    borderX = 1;
    borderY = 1;
    zoom = 1;

    geometry.base_width = width + borderX * 2;
    geometry.base_height = height + borderY * 2;
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

    // int16_t p_x = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    // int16_t p_y = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    // int p_press = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);

    int16_t mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    int16_t mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
    bool mouse_l = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);

    if ((p_x + mouse_x >= 0) && (p_x + mouse_x < winW)) {
        p_x += mouse_x;
    }
    if ((p_y + mouse_y >= 0) && (p_y + mouse_y < winH)) {
        p_y += mouse_y;
    }



    // if (!input_state_cb(0, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN)) {
    //     int cur_x = input_state_cb(0, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X);
    //     int cur_y = input_state_cb(0, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y);

    //     cur_x = (cur_x + 0x7FFF) * winW / (0x7FFF * 2);
    //     cur_y = (cur_y + 0x7FFF) * winH / (0x7FFF * 2);

    //     fprintf(stderr,  " Lightgun %d %d\n", cur_x, cur_y);
    // }

    int i;

/*
 * struct CrocoKeyMap {
 *  unsigned port;
 *  unsigned index;
 *
 *  int scanCode;
 */

    if (1 == 0) {

        for (i = 0; i < 24; i++) {
            int scanCode = crocokeymap[i].scanCode;

            if (scanCode != LAST_BUTTON) {
                if (input_state_cb(crocokeymap[i].port, RETRO_DEVICE_JOYPAD, 0, crocokeymap[i].index)) {
                    if (keyPressed[i] == 0) {
                        keyPressed[i] = 1;

                        fprintf(stderr,  " RETRO_DEVICE_JOYPAD %d %d\n", scanCode, i);

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

    }

    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)) {
        keypad_pressed[BUTTON_A] = 2;
        button_pressed(BUTTON_A);
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) {
        keypad_pressed[BUTTON_F] = 2;
        button_pressed(BUTTON_F);
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A)) {
        keypad_pressed[BUTTON_A] = 2;
        button_pressed(BUTTON_A);
    }

    for (i = 0; i < HANDLED_BUTTON; i++) {
        if (keypad_pressed[i] > 0) {
            keypad_pressed[i]--;
            if (keypad_pressed[i] == 0) {
                button_released(i);
            }
        }
    }


    for (i = 0; i < RETROK_LAST; i++) {
        int scanCode = KeySymToCPCKey[i];

        if (scanCode != LAST_BUTTON) {
            int state = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, i);

            if (state != Core_Key_Sate[i]) {
                Core_Key_Sate[i] = state;

                // if ((scanCode == BUTTON_F) && (state == 1) && (1 == 0)) {
                //     if (fullscreen) {
                //         fullScreenLeave();
                //     } else {
                //         fullScreenEnter();
                //     }
                // } else
                if (scanCode == BUTTON_D) {
                    if (state == 1) {
                        fprintf(stderr,  " Load binary\n");
                        fprintf(stderr,  " Flag %d\n", saturn.power_ctrl);

                        int retour = oldLoadObject("/Users/miguelvanhove/Downloads/diam20a/DIAMONDS");

                        // int retour = oldLoadObject("/Users/miguelvanhove/Downloads/xennon/XENNONGX.LIB");


                    }



                    scanCode = HANDLED_BUTTON;
                } else if ((scanCode == BUTTON_9) && (state == 1)) {
                    fprintf(stderr,  " Btn 9 \n");

                    saturn.PC = 0x0000;
                    // on_event();
                    scanCode = HANDLED_BUTTON;
                } else if ((scanCode == BUTTON_8) && (state == 1)) {
                    fprintf(stderr,  " Btn 8 \n");
                    button_pressed(BUTTON_EVAL);
                    scanCode = HANDLED_BUTTON;
                } else if ((scanCode == BUTTON_7) && (state == 1)) {
                    fprintf(stderr,  " Btn 7 \n");
                    button_released(BUTTON_EVAL);
                    scanCode = HANDLED_BUTTON;
                }
                // else if ((scanCode == BUTTON_F) && (state == 1)) {
//                     fprintf(stderr,  " Btn f \n");
//                     button_released(BUTTON_EVAL);
//                 }
// // }

                if (scanCode != HANDLED_BUTTON) {
                    if (state == 1) {
                        fprintf(stderr,  " input_state_cb %d\n", scanCode);

                        button_pressed(scanCode);
                    } else {
                        fprintf(stderr,  " input_state_cb %d\n", scanCode);

                        button_released(scanCode);
                    }
                }

            }
        }
    } /* retro_run */



    if (mouse_x) {
        // fprintf(stderr, "Mouse X: %d\n", mouse_x);
    }
    if (mouse_y) {
        // fprintf(stderr, "Mouse Y: %d\n", mouse_y);
    }

// fprintf(stderr, "Mouse left: %d,%d (%d)\n", mouse_x, mouse_y, mouse_l);

    int pointer_x = -1, pointer_y = -1;

    if (mouse_l != old_mouse_l) {
        old_mouse_l = mouse_l;


        if (mouse_l) {      // touch down
            if (fullscreen) {
                // fprintf(stderr, "Leave fullscreen\n");

                fullScreenLeave();

                fullscreen = 0;
            } else {
                // fullscreen=1;

                fprintf(stderr,  "Pointer: (%04X, %04X).\n", pointer_x, pointer_y);

                pointer_x = PXtoPOS(p_x);
                pointer_y =  PYtoPOS(p_y);

                fprintf(stderr,  "         (%6d, %6d) (%d,%d).\n", pointer_x, pointer_y, winW, winH);

                if ((pointer_x >= 20) && (pointer_x <= 281) && (pointer_y >= 67) && (pointer_y <= 207)) {
                    fullScreenEnter();
                } else {
                    int n;

                    for (n = 0; n < LAST_BUTTON + 1; n++) {
                        if ( (pointer_x >= keyScrs[n].x) && (pointer_y >= keyScrs[n].y) && (pointer_x < keyScrs[n].x + keyScrs[n].w) && (pointer_y < keyScrs[n].y + keyScrs[n].h) ) {
                            button_pressed(n);
                            last_press = n;
                        }
                    }
                }

            }
        } else { // touch up
            button_released(last_press);
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

            case 300:
                fprintf(stderr, "Load %s\n", file_path);
                int retour = LoadObject(object_size, object);

                fprintf(stderr, "loadfile 240 - Retour: %d\n", retour);
                break;
            case 160:
                button_pressed(BUTTON_ENTER);
                break;
            case 150:
                button_released(BUTTON_ENTER);
                break;
            case 80:
                fprintf(stderr, "loadFile_flag 80\n");

                button_pressed(BUTTON_EVAL);
                break;
            case 10:
                fprintf(stderr, "loadFile_flag 10\n");

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


    if (!fullscreen) {

        pointer_x = PXtoPOS(p_x);
        pointer_y =  PYtoPOS(p_y);

        if (px != -1) {
            restore_background_cross(pixels, px, py);
        }
    }


    if (zoom > 1) {

        if (1 == 1) {
            int x, y;
            int spos = 0;
            int npos = 0;

            // int spos0 = 20 + winW * 65;

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
                spos = y * winW * 2 + 67 * winW + 18;

                for (x = 0; x < NIBBLES_PER_ROW; x++) {
                    u8 nibble = data[npos];

                    int z;
                    int tz = (x == NIBBLES_PER_ROW - 1) ? 3 : 0;

                    for (z = tz; z < 4; z++) {
                        pixels[spos + (3 - z) * 2] = ((nibble & 1) == 1) ? RGB15(0, 0, 0) : bgcolor;
                        pixels[spos + 1 + (3 - z) * 2] = ((nibble & 1) == 1) ? RGB15(0, 0, 0) : bgcolor;

                        pixels[spos + (3 - z) * 2 + winW] = ((nibble & 1) == 1) ? RGB15(0, 0, 0) : bgcolor;
                        pixels[spos + 1 + (3 - z) * 2 + winW] = ((nibble & 1) == 1) ? RGB15(0, 0, 0) : bgcolor;


                        nibble = nibble / 4;
                    }
                    spos += 8;

                    npos++;
                }

            }
        } else {

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

    if (!fullscreen) {


        if ((px != pointer_x) || (py != pointer_y)) {
            fprintf(stderr,  "Pointer: (%d, %d) (%x-%x) - %d.\n", pointer_x, pointer_y, p_x, p_y, 0);
            fprintf(stderr,  "Mouse: (%d, %d) - %d,%d.\n", mouse_x, mouse_y, width, winW);
        }

        px = pointer_x;
        py = pointer_y;

        if ((pointer_x >= 20) && (pointer_x <= 281) && (pointer_y >= 67) && (pointer_y <= 207)) {
            draw_zoomcursor(pixels, px, py);
        } else {
            draw_cross(pixels, px, py);
        }


    }

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
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Pause"},
        {0},
    };

    log_cb(RETRO_LOG_INFO, "retro_load_game\n");

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    // Init pixel format

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        log_cb(RETRO_LOG_INFO, "begin of load games 2\n");
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "XRGG565 is not supported.\n");
        return 0;
    }



    if (info != NULL) {
        log_cb(RETRO_LOG_INFO, "begin of load gamer s\n");

        strcpy(file_path, info->path);
        loadGame();

        log_cb(RETRO_LOG_INFO, "end of load gamer s\n");
    }



    return true;
} /* retro_load_game */

char load84PH(long dsk_size, u8 *dsk)
{
    log_cb(RETRO_LOG_INFO, "load84PH\n");

    snapshotLength = (u32)dsk_size;

    snapshot = (u8 *)malloc(snapshotLength);
    memcpy(snapshot, dsk, snapshotLength);

    if (snapshot != NULL) {

        if (snapshot != NULL) {
            LireSnapshotMem(snapshot);
        }
        return true;
    }

    return false;
}


char loadHPHP48(long dsk_size, u8 *dsk)
{
    log_cb(RETRO_LOG_INFO, "loadHPHP48\n");

    fprintf(stderr, "loadFile_flag 600: %s\n", file_path);

    object = (char *)malloc(dsk_size);
    memcpy(object, dsk, dsk_size);
    object_size = dsk_size;

    loadFile_flag = 700;

    log_cb(RETRO_LOG_INFO, "send file %s to Stack\n", file_path);

    return true;
}


bool loadGame(void)
{
    log_cb(RETRO_LOG_INFO, "loadGame\n");

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
        return load84PH(dsk_size, dsk);
    }

    if (!memcmp(header, "HPHP48", 6)) {
        return loadHPHP48(dsk_size, dsk);
    }

    if (!memcmp(header, "PK", 2)) {
        log_cb(RETRO_LOG_INFO, "loadGame zip\n");

        mz_zip_archive zip_archive;
        memset(&zip_archive, 0, sizeof(zip_archive));

        if (mz_zip_reader_init_mem(&zip_archive, dsk, dsk_size, 0) == MZ_TRUE) {
            int i;
            // char isKcr = 0;

            for (i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++) {
                mz_zip_archive_file_stat file_stat;
                if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
                    mz_zip_reader_end(&zip_archive);
                    break;
                }

                if (!strcasecmp(file_stat.m_filename, "settings.ini")) {
                    // isKcr = 1;
                }

                log_cb(RETRO_LOG_INFO, "dir: %s\n", file_stat.m_filename);

                log_cb(RETRO_LOG_INFO, "usefile");

                unsigned char *undsk = (unsigned char *)malloc(file_stat.m_uncomp_size);
                mz_zip_reader_extract_to_mem(&zip_archive, 0, undsk, (unsigned int)file_stat.m_uncomp_size, 0);

                log_cb(RETRO_LOG_INFO, "header: %s\n", undsk);


                if (!memcmp(undsk, "84PH", 4)) {
                    return load84PH(file_stat.m_uncomp_size, undsk);
                }

                if (!memcmp(undsk, "HPHP48", 6)) {
                    return loadHPHP48(file_stat.m_uncomp_size, undsk);
                }


                // ddlog(core, 2, "Filename: \"%s\", Comment: \"%s\", Uncompressed size: %u, Compressed size: %u, Is Dir: %u\n", file_stat.m_filename, file_stat.m_comment, (unsigned int)file_stat.m_uncomp_size, (unsigned int)file_stat.m_comp_size, mz_zip_reader_is_file_a_directory(&zip_archive, i));
            }
        }
    }

// Send to Stack



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


static const char *cross[20] = {
    "X                               ",
    "XX                              ",
    "X.X                             ",
    "X..X                            ",
    "X...X                           ",
    "X....X                          ",
    "X.....X                         ",
    "X......X                        ",
    "X.......X                       ",
    "X........X                      ",
    "X.....XXXXX                     ",
    "X..X..X                         ",
    "X.X X..X                        ",
    "XX  X..X                        ",
    "X    X..X                       ",
    "     X..X                       ",
    "      X..X                      ",
    "      X..X                      ",
    "       XX                       ",
    "                                ",
};

static const char *zoomcursor[20] = {
    "XXXXXX      XXXXXX              ",
    "X....X      X....X              ",
    "X.XXXX      XXXX.X              ",
    "X.X            X.X              ",
    "X.X            X.X              ",
    "X.X            X.X              ",
    "XXX            XXX              ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "XXX            XXX              ",
    "X.X            X.X              ",
    "X.X            X.X              ",
    "X.X            X.X              ",
    "X.XXXX      XXXX.X              ",
    "X....X      X....X              ",
    "XXXXXX      XXXXXX              ",
};

void DrawPointBmpRestore(unsigned short int *buffer, int x, int y, int rwidth, int rheight)
{
    int idx;

    idx = x + y * rwidth;
    if ((idx >= 0) && (x < rwidth) && (y < rheight))
        buffer[idx] = background[idx];
}

void DrawPointBmp(unsigned short int *buffer, int x, int y, unsigned short int color, int rwidth, int rheight)
{
    int idx;

    idx = x + y * rwidth;
    if ((idx >= 0) && (x < rwidth) && (y < rheight))
        buffer[idx] = color;
}

void draw_zoomcursor(unsigned short int *surface, int x, int y)
{

    int i, j, idx;
    int dx = 32, dy = 20;

    unsigned short int col = 0xffff;

    int w = winW;
    int h = winH;

    for (j = y; j < y + dy; j++) {
        idx = 0;
        for (i = x; i < x + dx; i++) {

            if (zoomcursor[j - y][idx] == '.') DrawPointBmp(surface, i, j, col, w, h);
            else if (zoomcursor[j - y][idx] == 'X') DrawPointBmp(surface, i, j, 0, w, h);
            idx++;
        }
    }
}

void draw_cross(unsigned short int *surface, int x, int y)
{

    int i, j, idx;
    int dx = 32, dy = 20;

    unsigned short int col = 0xffff;

    int w = winW;
    int h = winH;

    for (j = y; j < y + dy; j++) {
        idx = 0;
        for (i = x; i < x + dx; i++) {

            if (cross[j - y][idx] == '.') DrawPointBmp(surface, i, j, col, w, h);
            else if (cross[j - y][idx] == 'X') DrawPointBmp(surface, i, j, 0, w, h);
            idx++;
        }
    }
}

void restore_background_cross(unsigned short int *surface, int x, int y)
{
    int i, j;
    int dx = 32, dy = 20;

    for (j = y; j < y + dy; j++) {
        for (i = x; i < x + dx; i++) {
            DrawPointBmpRestore(surface, i, j, winW, winH);
        }
    }
}