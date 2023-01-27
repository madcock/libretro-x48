/*
 *  This file is part of x48, an emulator of the HP-48sx Calculator.
 *  Copyright (C) 1994  Eddie C. Dost  (ecd@dressler.de)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $Log: rpl.c,v $
 * Revision 1.3  1995/01/11  18:20:01  ecd
 * major update to support HP48 G/GX
 *
 * Revision 1.2  1994/12/07  20:20:50  ecd
 * lots of more functions
 *
 * Revision 1.2  1994/12/07  20:20:50  ecd
 * lots of more functions
 *
 * Revision 1.1  1994/12/07  10:15:47  ecd
 * Initial revision
 *
 *
 * $Id: rpl.c,v 1.3 1995/01/11 18:20:01 ecd Exp ecd $
 */

#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "hp48.h"
#include "hp48_emu.h"
#include "rpl.h"
#include "append.h"
#include "disasm.h"
#include "romio.h"
#include "plateform.h"

#define DEFINE_TRANS_TABLE 1
#include "hp48char.h"
#undef DEFINE_TRANS_TABLE

struct objfunc {
    char *name;
    short length;
    word_20 prolog;
    char *(*func)__ProtoType__((word_20 * addr, char *string));
} objects[] = {
    {"System Binary", 0,               DOBINT,                  dec_bin_int},
    {"Real",          0,               DOREAL,                  dec_real},
    {"Long Real",     0,               DOEREL,                  dec_long_real},
    {"Complex",       0,               DOCMP,                   dec_complex},
    {"Long Complex",  0,               DOECMP,                  dec_long_complex},
    {"Character",     0,               DOCHAR,                  dec_char},
    {"Array",         0,               DOARRY,                  dec_array},
    {"Linked Array",  0,               DOLNKARRY,               dec_lnk_array},
    {"String",        2,               DOCSTR,                  dec_string},
    {"Hex String",    1,               DOHSTR,                  dec_hex_string},
    {"List",          0,               DOLIST,                  dec_list},
    {"Directory",     0,               DORRP,                   skip_ob},
    {"Symbolic",      0,               DOSYMB,                  dec_symb},
    {"Unit",          0,               DOEXT,                   dec_unit},
    {"Tagged",        0,               DOTAG,                   skip_ob},
    {"Graphic",       0,               DOGROB,                  skip_ob},
    {"Library",       0,               DOLIB,                   dec_library},
    {"Backup",        0,               DOBAK,                   skip_ob},
    {"Library Data",  0,               DOEXT0,                  dec_library_data},
    {"ACPTR",         0,               DOACPTR,                 dec_acptr},
    {"External 2",    0,               DOEXT2,                  skip_ob},
    {"External 3",    0,               DOEXT3,                  skip_ob},
    {"External 4",    0,               DOEXT4,                  skip_ob},
    {"Program",       0,               DOCOL,                   dec_prog},
    {"Code",          1,               DOCODE,                  dec_code},
    {"Global Ident",  0,               DOIDNT,                  dec_global_ident},
    {"Local Ident",   0,               DOLAM,                   dec_local_ident},
    {"XLib Name",     0,               DOROMP,                  dec_xlib_name},
    {"*",             0,               UM_MUL,                  dec_unit_op},
    {"/",             0,               UM_DIV,                  dec_unit_op},
    {"^",             0,               UM_POW,                  dec_unit_op},
    {" ",             0,               UM_PRE,                  dec_unit_op},
    {"_",             0,               UM_END,                  dec_unit_op},
    {0,               0,               0}
};

char *
#ifdef __FunctionProto__
skip_ob(word_20 *addr, char *string)
#else
skip_ob(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 size, type;
    char *p = string;
    struct objfunc *op;

    type = read_nibbles(*addr - 5, 5);
    for (op = objects; op->prolog != 0; op++) {
        if (op->prolog == type)
            break;
    }

    if (op->prolog) {
        sprintf(p, "%s", op->name);
        p += strlen(p);
    }

    size = read_nibbles(*addr, 5);
    *addr += size;

    *p = '\0';
    return p;
}

long
#ifdef __FunctionProto__
hxs2real(long hxs)
#else
hxs2real(hxs)
long hxs;
#endif
{
    int n = 0, c = 1;

    while (hxs) {
        n += (hxs & 0xf) * c;
        c *= 10;
        hxs >>= 4;
    }
    return n;
}

char *
#ifdef __FunctionProto__
dec_bin_int(word_20 *addr, char *string)
#else
dec_bin_int(addr, string)
word_20 *addr;
char *string;
#endif
{
    char *p = string;
    word_20 n = 0;

    n = read_nibbles(*addr, 5);
    *addr += 5;
    sprintf(p, "<%lXh>", (long)n);
    p += strlen(p);
    return p;
}

char *
#ifdef __FunctionProto__
real_number(word_20 *addr, char *string, int ml, int xl)
#else
real_number(addr, string, ml, xl)
word_20 *addr;
char *string;
int ml;
int xl;
#endif
{
    hp_real r;
    long re, xs;
    int i;
    char fmt[20];
    char m[16];
    char *p = string;

    /*
     * Read the number
     */
    r.x = read_nibbles(*addr, xl);
    *addr += xl;
    r.ml = read_nibbles(*addr, ml - 8);
    *addr += ml - 8;
    r.mh = read_nibbles(*addr, 8);
    *addr += 8;
    r.m = read_nibbles(*addr, 1);
    (*addr)++;
    r.s = read_nibbles(*addr, 1);
    (*addr)++;

    /*
     * Figure out the exponent
     */
    xs = 5;
    while (--xl) xs *= 10;
    re = hxs2real(r.x);
    if (re >= xs)
        re = re - 2 * xs;


    if ((re >= 0) && (re < ml + 1)) {
        if (r.s >= 5)
            *p++ = '-';

        sprintf(fmt, "%%.1X%%.8lX%%.%dlX", ml - 8);
        sprintf(m, fmt, r.m, r.mh, r.ml);

        for (i = 0; i <= re; i++) {
            *p++ = m[i];
        }
        *p++ = '.';
        for (; i < ml + 1; i++) {
            *p++ = m[i];
        }
        p--;
        while (*p == '0') p--;
        if (*p == '.') p--;
        *++p = '\0';

        return p;
    }

    if ((re < 0) && (re >= -ml - 1)) {
        sprintf(fmt, "%%.1X%%.8lX%%.%dlX", ml - 8);
        sprintf(m, fmt, r.m, r.mh, r.ml);

        for (i = ml; m[i] == '0'; i--) {
            ;
        }

        if (-re <= ml - i + 1) {
            if (r.s >= 5)
                *p++ = '-';

            *p++ = '.';

            for (i = 1; i < -re; i++) {
                *p++ = '0';
            }

            for (i = 0; i < ml + 1; i++) {
                *p++ = m[i];
            }
            p--;
            while (*p == '0') p--;
            *++p = '\0';

            return p;
        }
    }

    sprintf(fmt, "%%s%%X.%%.8lX%%.%dlX", ml - 8);
    sprintf(p, fmt, (r.s >= 5) ? "-" : "", r.m, r.mh, r.ml);

    p += strlen(p) - 1;

    while (*p == '0') p--;
    *++p = '\0';

    if (re) {
        sprintf(p, "E%ld", re);
        p += strlen(p);
        *p = '\0';
    }

    return p;
}

char *
#ifdef __FunctionProto__
dec_real(word_20 *addr, char *string)
#else
dec_real(addr, string)
word_20 *addr;
char *string;
#endif
{
    return real_number(addr, string, 11, 3);
}

char *
#ifdef __FunctionProto__
dec_long_real(word_20 *addr, char *string)
#else
dec_long_real(addr, string)
word_20 *addr;
char *string;
#endif
{
    return real_number(addr, string, 14, 5);
}

char *
#ifdef __FunctionProto__
dec_complex(word_20 *addr, char *string)
#else
dec_complex(addr, string)
word_20 *addr;
char *string;
#endif
{
    char *p = string;

    *p++ = '(';
    p = real_number(addr, p, 11, 3);
    *p++ = ',';
    p = real_number(addr, p, 11, 3);
    *p++ = ')';
    *p = '\0';
    return p;
}

char *
#ifdef __FunctionProto__
dec_long_complex(word_20 *addr, char *string)
#else
dec_long_complex(addr, string)
word_20 *addr;
char *string;
#endif
{
    char *p = string;

    *p++ = '(';
    p = real_number(addr, p, 14, 5);
    *p++ = ',';
    p = real_number(addr, p, 14, 5);
    *p++ = ')';
    *p = '\0';
    return p;
}

char *
#ifdef __FunctionProto__
dec_string(word_20 *addr, char *string)
#else
dec_string(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 len;
    unsigned char c;
    char *p = string;
    int i, n;

    len = read_nibbles(*addr, 5);
    *addr += 5;
    len -= 5;
    len /= 2;

    n = len;
    if (len > 1000)
        n = 1000;

    *p++ = '\"';
    for (i = 0; i < n; i++) {
        c = read_nibbles(*addr, 2);
        *addr += 2;
        if (hp48_trans_tbl[c].trans) {
            strcpy(p, hp48_trans_tbl[c].trans);
            p += strlen(p);
        } else
            *p++ = c;
    }

    if (n != len) {
        *p++ = '.';
        *p++ = '.';
        *p++ = '.';
    }

    *p++ = '\"';
    *p = '\0';
    return p;
}

char * dec_hex_string(word_20 *addr, char *string)
{
    int len, lead, i, n;
    static char hex[] = "0123456789ABCDEF";
    char *p = string;

    len = read_nibbles(*addr, 5);
    *addr += 5;
    len -= 5;

    if (len <= 16) {
        *p++ = '#';
        *p++ = ' ';
        lead = 1;
        for (i = len - 1; i >= 0; i--) {
            *p = hex[read_nibble(*addr + i)];
            if (lead) {
                if ((i != 0) && (*p == '0')) {
                    p--;
                } else {
                    lead = 0;
                }
            }
            p++;
        }

        *p++ = 'h';
    } else {
        *p++ = 'C';
        *p++ = '#';
        *p++ = ' ';

        sprintf(p, "%d", len);
        p += strlen(p);

        *p++ = ' ';

        n = len;
        if (len > 1000)
            n = 1000;

        for (i = 0; i < n; i++) {
            *p++ = hex[read_nibble(*addr + i)];
        }

        if (n != len) {
            *p++ = '.';
            *p++ = '.';
            *p++ = '.';
        }
    }

    *addr += len;

    *p = '\0';
    return p;
} /* dec_hex_string */

char *
#ifdef __FunctionProto__
dec_list(word_20 *addr, char *string)
#else
dec_list(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 semi;
    char *p = string;

    *p++ = '{';
    *p++ = ' ';
    semi = read_nibbles(*addr, 5);
    while (semi != SEMI) {
        p = dec_rpl_obj(addr, p);
        semi = read_nibbles(*addr, 5);
        if (semi != SEMI) {
            *p++ = ' ';
            *p = '\0';
        }
    }
    *p++ = ' ';
    *p++ = '}';
    *p = '\0';

    *addr += 5;
    return p;
}

char *
#ifdef __FunctionProto__
dec_symb(word_20 *addr, char *string)
#else
dec_symb(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 semi;
    char *p = string;

    semi = read_nibbles(*addr, 5);
    *p++ = '\'';
    while (semi != SEMI) {
        p = dec_rpl_obj(addr, p);
        semi = read_nibbles(*addr, 5);
        if (semi != SEMI) {
            *p++ = ' ';
            *p = '\0';
        }
    }
    *addr += 5;

    *p++ = '\'';
    *p = '\0';
    return p;
}

char *
#ifdef __FunctionProto__
dec_unit(word_20 *addr, char *string)
#else
dec_unit(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 semi;
    char *p = string;

    semi = read_nibbles(*addr, 5);
    while (semi != SEMI) {
        p = dec_rpl_obj(addr, p);
        semi = read_nibbles(*addr, 5);
        if (semi != SEMI) {
            *p++ = ' ';
            *p = '\0';
        }
    }
    *addr += 5;
    return p;
}

char *
#ifdef __FunctionProto__
dec_unit_op(word_20 *addr, char *string)
#else
dec_unit_op(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 op;
    char *p = string;

    op = read_nibbles(*addr - 5, 5);
    switch (op) {
        case UM_MUL:
            *p++ = '*';
            break;
        case UM_DIV:
            *p++ = '/';
            break;
        case UM_POW:
            *p++ = '^';
            break;
        case UM_END:
            *p++ = '_';
            break;
        case UM_PRE:
            p--;
            break;
        default:
            break;
    }
    *p = '\0';
    return p;
}

char *
#ifdef __FunctionProto__
dec_library(word_20 *addr, char *string)
#else
dec_library(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 libsize, libidsize;
/*
 * word_20        hashoff, mesgoff, linkoff, cfgoff;
 * word_20        mesgloc, cfgloc;
 */
    int i, libnum;
    unsigned char c;
    char *p = string;

    libsize = read_nibbles(*addr, 5);
    libidsize = read_nibbles(*addr + 5, 2);
    libnum = read_nibbles(*addr + 2 * libidsize + 9, 3);

    sprintf(p, "Library %d:  ", libnum);
    p += strlen(p);

    for (i = 0; i < libidsize; i++) {
        c = read_nibbles(*addr + 2 * i + 7, 2);
        if (hp48_trans_tbl[c].trans) {
            strcpy(p, hp48_trans_tbl[c].trans);
            p += strlen(p);
        } else
            *p++ = c;
    }

    *addr += libsize;

    *p = '\0';
    return p;
}

char *
#ifdef __FunctionProto__
dec_library_data(word_20 *addr, char *string)
#else
dec_library_data(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 size;
    char *p = string;

    size = read_nibbles(*addr, 5);

    strcpy(p, "Library Data");
    p += strlen(p);

    *addr += size;

    *p = '\0';
    return p;
}

char *
#ifdef __FunctionProto__
dec_acptr(word_20 *addr, char *string)
#else
dec_acptr(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 size;
    char *p = string;
    int i;
    static char hex[] = "0123456789ABCDEF";

    if (opt_gx) {
        size = 10;
        sprintf(p, "ACPTR ");
        p += strlen(p);
        for (i = 0; i < 5; i++) {
            *p++ = hex[read_nibble(*addr + i)];
        }
        *p++ = ' ';
        for (i = 5; i < 10; i++) {
            *p++ = hex[read_nibble(*addr + i)];
        }
    } else {
        size = read_nibbles(*addr, 5);
        strcpy(p, "Ext 1");
        p += strlen(p);
    }

    *addr += size;

    *p = '\0';
    return p;
}

char *
#ifdef __FunctionProto__
dec_prog(word_20 *addr, char *string)
#else
dec_prog(addr, string)
word_20 *addr;
char *string;
#endif
{
    word_20 semi;
    char *p = string;

    semi = read_nibbles(*addr, 5);
    while (semi != SEMI) {
        p = dec_rpl_obj(addr, p);
        semi = read_nibbles(*addr, 5);
        if (semi != SEMI) {
            *p++ = ' ';
            *p = '\0';
        }
    }
    *addr += 5;
    return p;
}

char *
#ifdef __FunctionProto__
dec_code(word_20 *addr, char *string)
#else
dec_code(addr, string)
word_20 *addr;
char *string;
#endif
{
    char *p = string;
    word_20 n, len;

    len = read_nibbles(*addr, 5);
    sprintf(p, "Code");
    p += strlen(p);

    n = 0;
    while (n < len)
/*
 * addr = disassemble(*addr, p);
 */
        n += len;

    *addr += len;
    return p;
}

char *
#ifdef __FunctionProto__
dec_local_ident(word_20 *addr, char *string)
#else
dec_local_ident(addr, string)
word_20 *addr;
char *string;
#endif
{
    int len, i, n;
    char *p = string;
    unsigned char c;

    len = read_nibbles(*addr, 2);
    *addr += 2;

    n = len;
    if (len > 1000)
        n = 1000;

    for (i = 0; i < n; i++) {
        c = read_nibbles(*addr, 2);
        *addr += 2;
        if (hp48_trans_tbl[c].trans) {
            strcpy(p, hp48_trans_tbl[c].trans);
            p += strlen(p);
        } else
            *p++ = c;
    }

    if (n != len) {
        *p++ = '.';
        *p++ = '.';
        *p++ = '.';
    }

    *p = '\0';
    return p;
}

char *
#ifdef __FunctionProto__
dec_global_ident(word_20 *addr, char *string)
#else
dec_global_ident(addr, string)
word_20 *addr;
char *string;
#endif
{
    int len, i, n;
    char *p = string;
    unsigned char c;

    len = read_nibbles(*addr, 2);
    *addr += 2;

    n = len;
    if (len > 1000)
        n = 1000;

    for (i = 0; i < n; i++) {
        c = read_nibbles(*addr, 2);
        *addr += 2;
        if (hp48_trans_tbl[c].trans) {
            strcpy(p, hp48_trans_tbl[c].trans);
            p += strlen(p);
        } else
            *p++ = c;
    }

    if (n != len) {
        *p++ = '.';
        *p++ = '.';
        *p++ = '.';
    }

    *p = '\0';
    return p;
}

char *
#ifdef __FunctionProto__
xlib_name(int lib, int command, char *string)
#else
xlib_name(lib, command, string)
int lib;
int command;
char *string;
#endif
{
    int n, len;
    int i, lib_n = 0;
    unsigned char c;
    word_20 romptab, acptr;
    word_20 offset, hash_end;
    word_20 lib_addr, name_addr;
    word_20 type, ram_base, ram_mask;
    short present = 0;
    char *p = string;

    /*
     * Configure RAM to address 0x70000
     */
    ram_base = saturn.mem_cntl[1].config[0];
    ram_mask = saturn.mem_cntl[1].config[1];
    if (opt_gx) {
        saturn.mem_cntl[1].config[0] = 0x80000;
        saturn.mem_cntl[1].config[1] = 0xc0000;
        romptab = ROMPTAB_GX;
    } else {
        saturn.mem_cntl[1].config[0] = 0x70000;
        saturn.mem_cntl[1].config[1] = 0xf0000;
        romptab = ROMPTAB_SX;
    }

    /*
     * look up number of installed libs in romptab
     */
    n = read_nibbles(romptab, 3);
    romptab += 3;

    if (n > 0) {
        /*
         * look up lib number in romptab
         */
        while (n--) {
            lib_n = read_nibbles(romptab, 3);
            romptab += 3;
            if (lib_n == lib)
                break;
            romptab += 5;
            if (opt_gx)
                romptab += 8;
        }
        if (lib_n == lib) {
            /*
             * look at hash table pointer
             */
            lib_addr = read_nibbles(romptab, 5);
            if (opt_gx) {
                romptab += 5;
                acptr = read_nibbles(romptab, 5);
                if (acptr != 0x00000) {
                    saturn.mem_cntl[1].config[0] = ram_base;
                    saturn.mem_cntl[1].config[1] = ram_mask;
                    sprintf(p, "XLIB %d %d", lib, command);
                    p += strlen(p);
                    return p;
                }
            }
            lib_addr += 3;
            offset = read_nibbles(lib_addr, 5);
            if (offset > 0) {
                /*
                 * look at the hash table
                 */
                lib_addr += offset;

                /*
                 * check if library is in ROM
                 */
                if (!opt_gx)
                    if (lib_addr < 0x70000)
                        saturn.mem_cntl[1].config[0] = 0xf0000;

                /*
                 * check pointer type
                 */
                type = read_nibbles(lib_addr, 5);
                if (type == DOBINT) {
                    /*
                     * follow pointer to real address
                     */
                    lib_addr += 5;
                    lib_addr = read_nibbles(lib_addr, 5);
                } else if (type == DOACPTR) {
                    /*
                     * follow pointer to real address
                     */
                    lib_addr += 5;
                    acptr = lib_addr + 5;
                    lib_addr = read_nibbles(lib_addr, 5);
                    acptr = read_nibbles(acptr, 5);
                    if (acptr != 0x00000) {
                        saturn.mem_cntl[1].config[0] = ram_base;
                        saturn.mem_cntl[1].config[1] = ram_mask;
                        sprintf(p, "XLIB %d %d", lib, command);
                        p += strlen(p);
                        return p;
                    }
                }

                /*
                 * get length of hash table
                 */
                lib_addr += 5;
                hash_end = read_nibbles(lib_addr, 5);
                hash_end += lib_addr;

                /*
                 * go into real name table
                 */
                lib_addr += 85;
                offset = read_nibbles(lib_addr, 5);
                lib_addr += offset;

                /*
                 * look at library name number 'command'
                 */
                offset = 5 * command;
                lib_addr += offset;
                if (lib_addr < hash_end) {
                    offset = read_nibbles(lib_addr, 5);
                    if (offset > 0) {
                        name_addr = lib_addr - offset;
                        len = read_nibbles(name_addr, 2);
                        name_addr += 2;
                        present = 1;
                        for (i = 0; i < len; i++) {
                            c = read_nibbles(name_addr, 2);
                            name_addr += 2;
                            if (hp48_trans_tbl[c].trans) {
                                strcpy(p, hp48_trans_tbl[c].trans);
                                p += strlen(p);
                            } else
                                *p++ = c;
                        }
                        *p = '\0';
                    }
                }
            }
        }
    }

    /*
     * Reconfigure RAM
     */
    saturn.mem_cntl[1].config[0] = ram_base;
    saturn.mem_cntl[1].config[1] = ram_mask;

    if (!present) {
        sprintf(p, "XLIB %d %d", lib, command);
        p += strlen(p);
    }
    return p;
}

char * dec_xlib_name(word_20 *addr, char *string)
{
    int lib, command;

    lib = read_nibbles(*addr, 3);
    *addr += 3;
    command = read_nibbles(*addr, 3);
    *addr += 3;

    return xlib_name(lib, command, string);
}

char * any_array(word_20 *addr, char *string, short lnk_flag)
{
    word_20 len, type, dim;
    word_20 *dim_lens, *dims;
    word_20 array_addr, elem_addr;
    long elems;
    int d, i;
    char *p = string;
    struct objfunc *op;

    array_addr = *addr;
    len = read_nibbles(*addr, 5);
    *addr += 5;
    type = read_nibbles(*addr, 5);
    *addr += 5;
    dim = read_nibbles(*addr, 5);
    *addr += 5;

    for (op = objects; op->prolog != 0; op++) {
        if (op->prolog == type)
            break;
    }

    dim_lens = (word_20 *)malloc(dim * sizeof(word_20));
    dims = (word_20 *)malloc(dim * sizeof(word_20));
    elems = 1;
    for (i = 0; i < dim; i++) {
        dim_lens[i] = read_nibbles(*addr, 5);
        dims[i] = dim_lens[i];
        elems *= dim_lens[i];
        *addr += 5;
    }

    if (op->prolog == 0) {
        sprintf(p, "of Type %.5lX, Dim %ld, Size ", type, (long)dim);
        p += strlen(p);
        for (i = 0; i < dim; i++) {
            sprintf(p, "%ld", (long)dim_lens[i]);
            p += strlen(p);
            if (i < dim - 1) {
                sprintf(p, " x ");
                p += strlen(p);
            }
        }
        *p = '\0';
        *addr = array_addr + len;
        free(dim_lens);
        free(dims);
        return p;
    }

    d = -1;
    while (elems--) {
        if (d < dim - 1) {
            for (; d < dim - 1; d++) {
                *p++ = '[';
            }
            d = dim - 1;
        }
        if (lnk_flag) {
            elem_addr = read_nibbles(*addr, 5);
            elem_addr += *addr;
            *addr += 5;
            p = (*op->func)(&elem_addr, p);
        } else
            p = (*op->func)(addr, p);
        *p = '\0';
        dims[d]--;
        if (dims[d])
            *p++ = ' ';
        while (dims[d] == 0) {
            dims[d] = dim_lens[d];
            d--;
            dims[d]--;
            *p++ = ']';
        }
    }

    free(dim_lens);
    free(dims);
    *addr = array_addr + len;

    *p = '\0';
    return p;
} /* any_array */

char *
#ifdef __FunctionProto__
dec_array(word_20 *addr, char *string)
#else
dec_array(addr, string)
word_20 *addr;
char *string;
#endif
{
    return any_array(addr, string, 0);
}

char *
#ifdef __FunctionProto__
dec_lnk_array(word_20 *addr, char *string)
#else
dec_lnk_array(addr, string)
word_20 *addr;
char *string;
#endif
{
    return any_array(addr, string, 1);
}

char *
#ifdef __FunctionProto__
dec_char(word_20 *addr, char *string)
#else
dec_char(addr, string)
word_20 *addr;
char *string;
#endif
{
    char *p = string;
    unsigned char c;

    c = read_nibbles(*addr, 2);
    *addr += 2;

    *p++ = '\'';
    if (hp48_trans_tbl[c].trans) {
        strcpy(p, hp48_trans_tbl[c].trans);
        p += strlen(p);
    } else
        *p++ = c;
    *p++ = '\'';

    *p = 0;
    return p;
}

short check_xlib(word_20 addr, char *string)
{
    int n, lib, command;
    word_20 romptab;
    word_20 offset, link_end;
    word_20 acptr;
    word_20 lib_addr;
    word_20 type, ram_base, ram_mask;
    char *p = string;

    /*
     * Configure RAM to address 0x70000
     */
    ram_base = saturn.mem_cntl[1].config[0];
    ram_mask = saturn.mem_cntl[1].config[1];
    if (opt_gx) {
        saturn.mem_cntl[1].config[0] = 0x80000;
        saturn.mem_cntl[1].config[1] = 0xc0000;
        romptab = ROMPTAB_GX;
    } else {
        saturn.mem_cntl[1].config[0] = 0x70000;
        saturn.mem_cntl[1].config[1] = 0xf0000;
        romptab = ROMPTAB_SX;
    }

    /*
     * look up number of installed libs in romptab
     */
    n = read_nibbles(romptab, 3);
    romptab += 3;

/*
 * fprintf(stderr, "Number of Libraries = %d\n", n);
 * fflush(stderr);
 */

    if (n > 0) {
        /*
         * look up lib number in romptab
         */
        while (n--) {
            lib = read_nibbles(romptab, 3);
            romptab += 3;
/*
 * fprintf(stderr, "Library num = %d\n", lib);
 * fflush(stderr);
 */
            /*
             * look at link table pointer
             */
            lib_addr = read_nibbles(romptab, 5);
/*
 * fprintf(stderr, "Library addr = %.5lx\n", lib_addr);
 * fflush(stderr);
 */
            romptab += 5;

            if (opt_gx) {
                acptr = read_nibbles(romptab, 5);
                romptab += 8;
                if (acptr != 0x00000)
                    continue;
            }

            lib_addr += 13;
            offset = read_nibbles(lib_addr, 5);
            if (offset > 0) {
                /*
                 * look at the link table
                 */
                lib_addr += offset;
/*
 * fprintf(stderr, "Link table addr = %.5lx\n", lib_addr);
 * fflush(stderr);
 */
                /*
                 * check if library is in ROM
                 */
                if (!opt_gx)
                    if (lib_addr < 0x70000)
                        saturn.mem_cntl[1].config[0] = 0xf0000;

                /*
                 * check pointer type
                 */
                type = read_nibbles(lib_addr, 5);
                if (type == DOBINT) {
                    /*
                     * follow pointer to real address
                     */
                    lib_addr += 5;
                    lib_addr = read_nibbles(lib_addr, 5);
                }
/*
 * fprintf(stderr, "Link table addr (2) = %.5lx\n", lib_addr);
 * fflush(stderr);
 */
                /*
                 * get length of link table
                 */
                lib_addr += 5;
                link_end = read_nibbles(lib_addr, 5);
                link_end += lib_addr;
/*
 * fprintf(stderr, "Link table end = %.5lx\n", link_end);
 * fflush(stderr);
 */
                /*
                 * look at library commands
                 */
                lib_addr += 5;
                command = 0;
                while (lib_addr < link_end) {
                    offset = read_nibbles(lib_addr, 5);
                    if (offset > 0) {
                        if (addr == ((lib_addr + offset) & 0xfffff)) {
                            p = xlib_name(lib, command, p);
                            saturn.mem_cntl[1].config[0] = ram_base;
                            saturn.mem_cntl[1].config[1] = ram_mask;
                            return 1;
                        }
                    }
                    lib_addr += 5;
                    command++;
                }
                if (opt_gx)
                    saturn.mem_cntl[1].config[0] = 0x80000;
                else
                    saturn.mem_cntl[1].config[0] = 0x70000;
            }
        }
    }

    /*
     * Reconfigure RAM
     */
    saturn.mem_cntl[1].config[0] = ram_base;
    saturn.mem_cntl[1].config[1] = ram_mask;

    return 0;
} /* check_xlib */


char * dec_rpl_obj(word_20 *addr, char *string)
{
    word_20 prolog = 0;
    word_20 prolog_2;
    char *p = string;
    char tmp_str[80];
    struct objfunc *op;

    prolog = read_nibbles(*addr, 5);

    for (op = objects; op->prolog != 0; op++) {
        if (op->prolog == prolog)
            break;
    }

    if (op->prolog == 0) {
        if (check_xlib(prolog, tmp_str)) {
            p = append_str(p, tmp_str);
        } else {
            prolog_2 = read_nibbles(prolog, 5);
            for (op = objects; op->prolog != 0; op++) {
                if (op->prolog == prolog_2)
                    break;
            }
            if (op->prolog)
                p = dec_rpl_obj(&prolog, p);
            else
                p = append_str(p, "External");
        }
        *addr += 5;
        return p;
    }

    *addr += 5;
    p = (*op->func)(addr, p);

    return p;
} /* dec_rpl_obj */

void decode_rpl_obj_2(word_20 addr, char *typ, char *dat)
{
    word_20 prolog = 0;
    int len;
    char tmp_str[80];
    struct objfunc *op;

    typ[0] = '\0';
    dat[0] = '\0';

    prolog = read_nibbles(addr, 5);

    for (op = objects; op->prolog != 0; op++) {
        if (op->prolog == prolog)
            break;
    }

    if (op->prolog == 0) {
        if (addr == SEMI) {
            append_str(typ, "Primitive Code");
            append_str(dat, "SEMI");
        } else if (addr + 5 == prolog) {
            append_str(typ, "Primitive Code");
            sprintf(dat, "at %.5lX", prolog);
        } else {
            append_str(typ, "PTR");
            sprintf(dat, "%.5lX", prolog);
        }
        return;
    }

    if (op->prolog == DOCOL) {
        if (check_xlib(addr, tmp_str)) {
            append_str(typ, "XLib Call");
            append_str(dat, tmp_str);
            return;
        }
    }

    if (op->length) {
        len = (read_nibbles(addr + 5, 5) - 5) / op->length;
        sprintf(typ, "%s %d", op->name, len);
    } else {
        append_str(typ, op->name);
    }

    addr += 5;
    (*op->func)(&addr, dat);

    return;
} /* decode_rpl_obj_2 */

char * decode_rpl_obj(word_20 addr, char *buf)
{
    word_20 prolog = 0;
    int len;
    char *p = buf;
    char tmp_str[80];
    struct objfunc *op;

    prolog = read_nibbles(addr, 5);

    for (op = objects; op->prolog != 0; op++) {
        if (op->prolog == prolog)
            break;
    }

    if (op->prolog == 0) {
        if (addr == SEMI) {
            p = append_str(buf, "Primitive Code");
            p = append_tab_16(buf);
            p = append_str(p, "SEMI");
        } else if (addr + 5 == prolog) {
            p = append_str(buf, "Primitive Code");
            p = append_tab_16(buf);
            sprintf(p, "at %.5lX", prolog);
            p += strlen(p);
            *p = '\0';
        } else {
            p = append_str(buf, "PTR");
            p = append_tab_16(buf);
            sprintf(p, "%.5lX", prolog);
            p += strlen(p);
            *p = '\0';
        }
        return p;
    }

    if (op->prolog == DOCOL) {
        if (check_xlib(addr, tmp_str)) {
            p = append_str(buf, "XLib Call");
            p = append_tab_16(buf);
            p = append_str(p, tmp_str);
            return p;
        }
    }

    p = append_str(buf, op->name);

    if (op->length) {
        len = (read_nibbles(addr + 5, 5) - 5) / op->length;
        sprintf(p, " %d", len);
        p += strlen(p);
    }

    p = append_tab_16(buf);
    addr += 5;
    p = (*op->func)(&addr, p);

    return p;
} /* decode_rpl_obj */


/*********************************************************************
* LoadObject: load an HP48 object onto the stack                    *
*********************************************************************/
int LoadObject(int objectSize, char *object)
{
    long size, i, j;
    word_20 len, addr;
    int byte = 0;
    unsigned char *buf;

    size = objectSize - 8;

    /* save room for file */
    if ( !(buf = (unsigned char *)malloc(size * 2)) ) {
        return( -5 );       /* ERROR: can't save enough room */
    }
    /* copy file into buffer */
    for ( i = 0, j = 0; i < size; i++ ) {
        byte = object[8 + i];

        buf[j++] = (char)byte & 0x0F;
        buf[j++] = (char)(byte >> 4) & 0x0F;
    }

    /* introduce file into HP48 memory */
    len = RPL_ObjectSize_2(buf);
    if ( !(addr = RPL_CreateTemp(len)) ) {
        fprintf(stderr, "Object address = 0x%5lX\n", len);
        free(buf);
        return( -7 );    /* ERROR: HP48 has not enough memory */
    }
    for ( i = 0; i < len; i++ ) {
        write_nibble(addr + i, (int)buf[i]);
    }

    // fprintf(stderr, "Lenght %d,%d\n", len, size);
    // fprintf(stderr, "Addr %d\n", addr);


    RPL_Push(addr);

    free(buf);

    // on_event();
    //
    fprintf(stderr, "LoadObject: push %lx\n", addr);


    return 0;
} /* LoadObject */

/*********************************************************************
* LoadObject: load an HP48 object onto the stack                    *
*********************************************************************/
int oldLoadObject(char *filename)
{

    {u8 *dsk;
     long dsk_size;

     FILE *fic = fopen(filename, "rb");
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

     memcpy(saturn.ram + 0x28a0 + 5, dsk + 8, dsk_size - 8);
     return 0;}



    FILE *f;
    struct stat st;
    long size, i, j;
    word_20 len, addr;
    int byte = 0;
    unsigned char *buf;

    if ( !(f = fopen(filename, "r")) ) {
        return( -1 );      /* ERROR: can't open file */
    }

    if ( stat(filename, &st)  < 0 ) {
        fclose(f);
        return( -2 );      /* ERROR: can't stat file */
    }

    fseek(f, 8, SEEK_SET);
    size = st.st_size - 8;

    /* save room for file */
    if ( !(buf = (unsigned char *)malloc(size * 2)) ) {
        fclose(f);
        return( -5 );       /* ERROR: can't save enough room */
    }
    /* copy file into buffer */
    for ( i = 0, j = 0; i < size; i++ ) {
        if ( fread(&byte, 1, 1, f) != 1 ) {
            fclose(f);
            free(buf);
            return( -6 ); /* ERROR: can't read from file */
        }
        buf[j++] = (char)byte & 0x0F;
        buf[j++] = (char)(byte >> 4) & 0x0F;
    }
    fclose(f);

    /* introduce file into HP48 memory */
    len = RPL_ObjectSize_2(buf);
    if ( !(addr = RPL_CreateTemp(len)) ) {
        fprintf(stderr, "Object address = 0x%5lX\n", len);
        free(buf);
        return( -7 );    /* ERROR: HP48 has not enough memory */
    }
    for ( i = 0; i < len; i++ ) {
        write_nibble(addr + i, (int)buf[i]);
    }

    // fprintf(stderr, "Lenght %d,%d\n", len, size);
    // fprintf(stderr, "Addr %d\n", addr);


    RPL_Push(addr);

    free(buf);

    // on_event();
    //
    fprintf(stderr, "LoadObject: push %lx\n", addr);


    return 0;
} /* LoadObject */

word_20 RPL_CreateTemp(word_20 l)
{
    word_20 a, b, c;
    int i;

    l += 6;   // memory for link field (5) + marker (1) and end
    a = read_nibbles(TEMPTOP, 5);
    b = read_nibbles(RSKTOP, 5);  /* start of avalaible memory */
    c = read_nibbles(DSKTOP, 5);  /* end of avalaible memory */
    if ( (b + l) > c ) {           /* check if enough room */
        return 0x00000;
    }

    write_nibbles(TEMPTOP, a + l, 5); /* adjust end of temporary objs */
    write_nibbles(RSKTOP, b + l, 5); /* adjust start of temporary mem */
    write_nibbles(AVMEM, (c - (b + l)) / 5, 5); /* adjust new free memory */

    for ( i = b - a; i >= 0; i-- ) {
        write_nibble(a + l + i,  read_nibble(a + i));
    }

    write_nibbles(a + l - 5, l, 5); /* set temporary obj length field */
    return( a + 1 );              /* return temporary obj address */
} /* RPL_CreateTemp */

void RPL_Push(word_20 n)
{
    word_20 avmem, stkp;

    if ( !(avmem = read_nibbles(AVMEM, 5)) ) {
        return;
    }

    write_nibbles(AVMEM, --avmem, 5);
    stkp = read_nibbles(DSKTOP, 5);
    stkp -= 5;
    write_nibbles(stkp, n, 5);
    write_nibbles(DSKTOP, stkp, 5);
}

/*********************************************************************
* RPL_ObjectSize: return the size (in nibbles) of the object in 'd' *
*********************************************************************/
word_20 RPL_ObjectSize(word_20 d)
{
    word_20 prolog, l = 0, n;

    prolog = read_nibbles(d, 5);
    switch ( prolog ) {
        case DOBINT:
            l = 10;
            break; // System Binary
        case DOREAL:
            l = 21;
            break;  // Real
        case DOEREL:
            l = 26;
            break; // Long Real
        case DOCMP:
            l = 37;
            break; // Complex
        case DOECMP:
            l = 47;
            break; // Long Complex
        case DOCHAR:
            l =  7;
            break; // Character
        case DOACPTR:
            l = 15;
            break; // Extended Pointer
        case DOROMP:
            l = 11;
            break; // XLIB Name

        case DOLIST: // List
        case DOSYMB: // Algebraic
        case DOEXT: // Unit
        case DOCOL: // Program

            n = 5;
            while ( n ) {
                l += n;
                d += n;
                n = RPL_ObjectSize(d);
            }
            ;
            l += 5;
            break;

        case DOIDNT:
        case DOLAM:
            n = 7 + read_nibbles(d + 5, 2) * 2;
            l = n + RPL_ObjectSize(d + n);
            // l = 7 + read_nibbles(d + 5, 2) * 2;   <-- on emu
            break;

        case DOTAG:
            n = 7 + read_nibbles(d + 5, 2) * 2;
            l = n + RPL_ObjectSize(d + n);
            break;

        case DORRP:
            if ( !(n = read_nibbles(d + 8, 5)) )
                l = 13;
            else {
                l = n + 8;
                n = read_nibbles(d + l, 2) * 2 + 4;
                l += n;
                l += RPL_ObjectSize(d + l);
            }
            break;

        case DOARRY:
        case DOLNKARRY:
        case DOCSTR:
        case DOHSTR:
        case DOGROB:
        case DOLIB:
        case DOBAK:
        case DOEXT0:
        case DOEXT2:
        case DOEXT3:
        case DOEXT4:
        case DOCODE:
            l = 5 + read_nibbles(d + 5, 5);
            break;

        case SEMI:
            l =  0;
            break;

        default:
            l =  5;
            break;
    } /* switch */

    return l;
} /* RPL_ObjectSize */


/*********************************************************************
* RPL_ObjectSize_2: same as RPL_ObjectSize but in computer memory   *
*********************************************************************/
word_20 RPL_ObjectSize_2(unsigned char *d)
{
    word_20 prolog, l = 0, n;

    prolog = read_nibbles_2(d, 5);

    // fprintf(stderr, "Bytes:\n");
    // int oo;
    // for (oo = 0; oo < 32; oo++) {
    //     fprintf(stderr, "%02X ", d[oo]);
    //     if ((d[oo] > 32) && (d[oo] < 128)) {
    //         fprintf(stderr, "(%c) ", d[oo]);
    //     } else {
    //         fprintf(stderr, "(.) ");
    //     }
    //     if (oo == 15) {
    //         fprintf(stderr, "\n");
    //     }
    // }

    // fprintf(stderr, "\nProlog: %ld\n", prolog);


    switch ( prolog ) {
        case DOBINT:
            l = 10;
            break; // System Binary
        case DOREAL:
            l = 21;
            break;  // Real
        case DOEREL:
            l = 26;
            break; // Long Real
        case DOCMP:
            l = 37;
            break; // Complex
        case DOECMP:
            l = 47;
            break; // Long Complex
        case DOCHAR:
            l =  7;
            break; // Character
        case DOACPTR:
            l = 15;
            break; // Extended Pointer
        case DOROMP:
            l = 11;
            break; // XLIB Name

        case DOLIST: // List
        case DOSYMB: // Algebraic
        case DOEXT: // Unit
        case DOCOL: // Program
            n = 5;
            while ( n ) {
                l += n;
                d += n;
                n = RPL_ObjectSize_2(d);
            }
            ;
            l += 5;
            break;

        case DOIDNT:
        case DOLAM:
            l = 7 + read_nibbles_2(d + 5, 2) * 2;
            break;

        case DOTAG:
            n = 7 + read_nibbles_2(d + 5, 2) * 2;
            l = n + RPL_ObjectSize_2(d + n);
            break;

        case DORRP:
            if ( !(n = read_nibbles_2(d + 8, 5)) )
                l = 13;
            else {
                l = n + 8;
                n = read_nibbles_2(d + l, 2) * 2 + 4;
                l += n;
                l += RPL_ObjectSize_2(d + l);
            }
            break;

        case DOARRY:
        case DOLNKARRY:
        case DOCSTR:
        case DOHSTR:
        case DOGROB:
        case DOLIB:
        case DOBAK:
        case DOEXT0:
        case DOEXT2:
        case DOEXT3:
        case DOEXT4:
        case DOCODE:
            l = 5 + read_nibbles_2(d + 5, 5);
            break;

        case SEMI:
            l =  0;
            break;

        default:
            l =  5;
            break;
    } /* switch */

    return l;
} /* RPL_ObjectSize_2 */

/**************************************************************************
* read_nibbles_2: read the nibbles of the object in temp memory (not HP) *
**************************************************************************/
long read_nibbles_2(unsigned char *addr, int len)
{
    long val = 0;

    addr += len;
    while ( len-- > 0 ) {

        // fprintf(stderr, "%02X ", *addr);

        addr--;
        val = (val << 4) | (*addr);
    }

    // fprintf(stderr, "\n");


    return val;
}



int read_bin_file(char *filename)
{
    struct stat st;
    FILE *fp;
    word_20 bin_size = 0;
    unsigned char *bin_buffer = (unsigned char *)0;
    int bBinary;
    long dwAddress;
    long i;

    if (NULL == (fp = fopen(filename, "r"))) {
        return 0;
    }

    if (stat(filename, &st) < 0) {
        fclose(fp);
        return 0;
    }

    bin_size = st.st_size;
    bin_buffer = (unsigned char *)malloc(bin_size * 2);

    if (fread(bin_buffer + bin_size, 1, (size_t)bin_size, fp) != bin_size) {
        free(bin_buffer);
        fclose(fp);
        return 0;
    }
    fclose(fp);

    bBinary =  ((bin_buffer[bin_size + 0] == 'H')
                &&  (bin_buffer[bin_size + 1] == 'P')
                &&  (bin_buffer[bin_size + 2] == 'H')
                &&  (bin_buffer[bin_size + 3] == 'P')
                &&  (bin_buffer[bin_size + 4] == '4')
                &&  (bin_buffer[bin_size + 5] == '8')
                &&  (bin_buffer[bin_size + 6] == '-'));

    for (i = 0; i < bin_size; i++) {
        unsigned char byTwoNibs = bin_buffer[i + bin_size];
        bin_buffer[i * 2  ] = (unsigned char)(byTwoNibs & 0xF);
        bin_buffer[i * 2 + 1] = (unsigned char)(byTwoNibs >> 4);
    }

    fprintf(stderr, "%ld bytes\n", bin_size);


    if (bBinary) { // load as binary
        bin_size = RPL_ObjectSize_2(bin_buffer + 16);
        dwAddress = RPL_CreateTemp(bin_size);
        if (dwAddress == 0) return 0;

        store_n(dwAddress, bin_buffer + 16, bin_size);
    } else { // load as string
        bin_size *= 2;
        dwAddress = RPL_CreateTemp(bin_size + 10);
        if (dwAddress == 0) return 0;
        write_nibbles(dwAddress, 0x02A2C, 5); // String
        write_nibbles(dwAddress + 5, bin_size + 5, 5); // length of String
                                                       //
        store_n(dwAddress + 10, bin_buffer, bin_size); // data
    }
    RPL_Push(dwAddress);

    fprintf(stderr, "read_bin: push %ld (%d - %ld bytes)\n", dwAddress, bBinary, bin_size);


    return 1;
} /* read_bin_file */
