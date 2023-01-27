
#ifndef PLATEFORM_T
#define PLATEFORM_T

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "hp48.h"
#include "resources.h"
#include "device.h"

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;


typedef struct disp_image_t {
	int bytes_per_line;
	char data[32768]; // ??? Need to adjust value

	int width, height;

} disp_image_t;


typedef struct menu_image_t {
	int bytes_per_line;
	char data[32768]; // ??? Need to adjust value

	int width, height;

} menu_image_t;


typedef struct disp_t {
	int display_update;
	char disp_info;
	disp_image_t        *disp_image;
	menu_image_t          *menu_image;

	int lines;
	int offset;

		char mapped;


} disp_t;


void	refresh_display();
int GetEvent(void);
void	refresh_icon();

void init_plateform();
int button_pressed(int b);
int button_released(int b);

// From emulate

void emulate_start(void);
void emulate_frame(void);
// From init

int getSnapshotSize(void);
char *getSnapshot(u32 *len);
void LireSnapshotMem(u8 *snap);

//

extern disp_t disp;
// extern saturn_t saturn;


#define UPDATE_MENU 1
#define UPDATE_DISP     2


#define BUTTON_A        0
#define BUTTON_B        1
#define BUTTON_C        2
#define BUTTON_D        3
#define BUTTON_E        4
#define BUTTON_F        5

#define BUTTON_MTH      6
#define BUTTON_PRG      7
#define BUTTON_CST      8
#define BUTTON_VAR      9
#define BUTTON_UP       10
#define BUTTON_NXT      11

#define BUTTON_COLON    12
#define BUTTON_STO      13
#define BUTTON_EVAL     14
#define BUTTON_LEFT     15
#define BUTTON_DOWN     16
#define BUTTON_RIGHT    17

#define BUTTON_SIN      18
#define BUTTON_COS      19
#define BUTTON_TAN      20
#define BUTTON_SQRT     21
#define BUTTON_POWER    22
#define BUTTON_INV      23

#define BUTTON_ENTER    24
#define BUTTON_NEG      25
#define BUTTON_EEX      26
#define BUTTON_DEL      27
#define BUTTON_BS       28

#define BUTTON_ALPHA    29
#define BUTTON_7        30
#define BUTTON_8        31
#define BUTTON_9        32
#define BUTTON_DIV      33

#define BUTTON_SHL      34
#define BUTTON_4        35
#define BUTTON_5        36
#define BUTTON_6        37
#define BUTTON_MUL      38

#define BUTTON_SHR      39
#define BUTTON_1        40
#define BUTTON_2        41
#define BUTTON_3        42
#define BUTTON_MINUS    43

#define BUTTON_ON       44
#define BUTTON_0        45
#define BUTTON_PERIOD   46
#define BUTTON_SPC      47
#define BUTTON_PLUS     48

#define LAST_BUTTON     48

#define HANDLED_BUTTON  49



void on_event(void);

#endif
