#include <signal.h>

#include "hp48_emu.h"
#include "plateform.h"

disp_t disp;

char *progname;
char *res_name;
char *res_class;

int saved_argc;
char **saved_argv;

int mx,my;

saturn_t saturn;


typedef struct button_t {

    char *name;
    short pressed;
    short extra;

    int code;


} button_t;

#define BUTTON_A      0
#define BUTTON_B      1
#define BUTTON_C      2
#define BUTTON_D      3
#define BUTTON_E      4
#define BUTTON_F      5

#define BUTTON_MTH    6
#define BUTTON_PRG    7
#define BUTTON_CST    8
#define BUTTON_VAR    9
#define BUTTON_UP     10
#define BUTTON_NXT    11

#define BUTTON_COLON  12
#define BUTTON_STO    13
#define BUTTON_EVAL   14
#define BUTTON_LEFT   15
#define BUTTON_DOWN   16
#define BUTTON_RIGHT  17

#define BUTTON_SIN    18
#define BUTTON_COS    19
#define BUTTON_TAN    20
#define BUTTON_SQRT   21
#define BUTTON_POWER  22
#define BUTTON_INV    23

#define BUTTON_ENTER  24
#define BUTTON_NEG    25
#define BUTTON_EEX    26
#define BUTTON_DEL    27
#define BUTTON_BS     28

#define BUTTON_ALPHA  29
#define BUTTON_7      30
#define BUTTON_8      31
#define BUTTON_9      32
#define BUTTON_DIV    33

#define BUTTON_SHL    34
#define BUTTON_4      35
#define BUTTON_5      36
#define BUTTON_6      37
#define BUTTON_MUL    38

#define BUTTON_SHR    39
#define BUTTON_1      40
#define BUTTON_2      41
#define BUTTON_3      42
#define BUTTON_MINUS  43

#define BUTTON_ON     44
#define BUTTON_0      45
#define BUTTON_PERIOD 46
#define BUTTON_SPC    47
#define BUTTON_PLUS   48

#define LAST_BUTTON   48

button_t buttons[] = {
    {"A", 0, 0, 0x14},
    {"B", 0, 0, 0x84},
    {"C", 0, 0, 0x83},
    {"D", 0, 0, 0x82},
    {"E", 0, 0, 0x81},
    {"F", 0, 0, 0x80},

    {"MTH", 0, 0, 0x24},
    {"PRG", 0, 0, 0x74},
    {"CST", 0, 0, 0x73},
    {"VAR", 0, 0, 0x72},
    {"UP", 0, 0, 0x71},
    {"NXT", 0, 0, 0x70},

    {"COLON", 0, 0, 0x04},
    {"STO", 0, 0, 0x64},
    {"EVAL", 0, 0, 0x63},
    {"LEFT", 0, 0, 0x62},
    {"DOWN", 0, 0, 0x61},
    {"RIGHT", 0, 0, 0x60},

    {"SIN", 0, 0, 0x34},
    {"COS", 0, 0, 0x54},
    {"TAN", 0, 0, 0x53},
    {"SQRT", 0, 0, 0x52},
    {"POWER", 0, 0, 0x51},
    {"INV", 0, 0, 0x50},

    {"ENTER", 0, 0, 0x44},
    {"NEG", 0, 0, 0x43},
    {"EEX", 0, 0, 0x42},
    {"DEL", 0, 0, 0x41},
    {"BS", 0, 0, 0x40},

    {"ALPHA", 0, 0, 0x35},
    {"7", 0, 0, 0x33},
    {"8", 0, 0, 0x32},
    {"9", 0, 0, 0x31},
    {"DIV", 0, 0, 0x30},

    {"SHL", 0, 0, 0x25},
    {"4", 0, 0, 0x23},
    {"5", 0, 0, 0x22},
    {"6", 0, 0, 0x21},
    {"MUL", 0, 0, 0x20},

    {"SHR", 0, 0, 0x15},
    {"1", 0, 0, 0x13},
    {"2", 0, 0, 0x12},
    {"3", 0, 0, 0x11},
    {"MINUS", 0, 0, 0x10},

    {"ON", 0, 0, 0x8000},
    {"0", 0, 0, 0x03},
    {"PERIOD", 0, 0, 0x02},
    {"SPC", 0, 0, 0x010},
    {"PLUS", 0, 0, 0x00},

    {0}
};






int GetEvent(void)
{


    // button_pressed(1);
    // button_released(1);


    return 0;
}

void refresh_display(void)
{

    // int n;

    // for(n=0; n<1000; n++) {
    //  printf("%d ", disp.disp_image->data[n] );

    //  if ( disp.disp_image->data[n]!=0) {
    //      printf("--------------\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    //      // pause();
    //  }

    // }

    // printf("\n");

}

void refresh_icon(void)
{
}


void signal_handler(int sig)
{
    // fprintf(stderr, "signal_handler %d\n", sig);

    switch ( sig ) {
        case SIGALRM:
            got_alarm = 1;
            break;
        default:
            break;
    }
}

void init_plateform(void)
{
#if !defined(SF2000)
    fprintf(stderr, "init_plateform\n");
    int i;
    char *res, *buf, *buf2;
    struct passwd *pwd;
    sigset_t set;
    struct sigaction sa;
    struct itimerval it;

    /*********************************
    * Install a handler for SIGALRM *
    *********************************/
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sa.sa_handler = signal_handler;
    sa.sa_mask = set;
#ifdef SA_RESTART
    sa.sa_flags = SA_RESTART;
#endif
    sigaction(SIGALRM, &sa, (struct sigaction *)0);

    /*************************************
    * Set the real time interval  timer *
    *************************************/
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 20000;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 20000;
    setitimer(ITIMER_REAL, &it, (struct itimerval *)0);
#endif
} /* init_plateform */


int button_pressed(int b)
{
    int code;
    int i, r, c;

    code = buttons[b].code;
    buttons[b].pressed = 1;
    if (code == 0x8000) {
        for (i = 0; i < 9; i++) {
            saturn.keybuf.rows[i] |= 0x8000;
        }
        do_kbd_int();
    } else {
        r = code >> 4;
        c = 1 << (code & 0xf);
        if ((saturn.keybuf.rows[r] & c) == 0) {
            if (saturn.kbd_ien) {
                do_kbd_int();
            }
            if ((saturn.keybuf.rows[r] & c)) {
                fprintf(stderr, "bug\n");
            }
            saturn.keybuf.rows[r] |= c;
        }
    }

    saturn.kbd_ien = 1;

    fprintf(stderr, "Button pressed  %d (%s) - %d\n", buttons[b].code, buttons[b].name, b);
    return 0;
} /* button_pressed */

int button_released(int b)
{
    int code;

    code = buttons[b].code;
    buttons[b].pressed = 0;
    if (code == 0x8000) {
        int i;
        for (i = 0; i < 9; i++) {
            saturn.keybuf.rows[i] &= ~0x8000;
        }
    } else {
        int r, c;
        r = code >> 4;
        c = 1 << (code & 0xf);
        saturn.keybuf.rows[r] &= ~c;
    }
    fprintf(stderr, "Button released  %d (%s)\n",  buttons[b].code, buttons[b].name);

    return 0;
}

void on_event(void)
{
    int i;

    button_pressed(BUTTON_ON);

    for ( i = 0; i <= 5000; i++) {
        step_instruction();
        schedule();
    }

    button_released(BUTTON_ON);

    while ( read_nibbles(saturn.PC, 3) != 0x807) {
        step_instruction();
        schedule();
    }

}
