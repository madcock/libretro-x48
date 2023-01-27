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

/* $Log: resources.c,v $
 * Revision 1.3  1995/01/11  18:20:01  ecd
 * major update to support HP48 G/GX
 *
 * Revision 1.2  1994/12/07  20:20:50  ecd
 * more resource get functions
 *
 * Revision 1.2  1994/12/07  20:20:50  ecd
 * more resource get functions
 *
 * Revision 1.1  1994/12/07  10:15:47  ecd
 * Initial revision
 *
 *
 * $Id: resources.c,v 1.3 1995/01/11 18:20:01 ecd Exp ecd $
 */

/* xscreensaver, Copyright (c) 1992 Jamie Zawinski <jwz@lucid.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include "global.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "resources.h"
#include "disasm.h"
#include "errors.h"

int	verbose;
int	quiet;
int     useTerminal;
int     useSerial;
char   *serialLine;
int     useXShm;
int     useDebugger;
int	netbook;
int	throttle;
int     initialize;
int     resetOnStartup;
char   *romFileName;
char   *homeDirectory;

void get_resources(void)
{
  if (get_boolean_resource("printVersion", "PrintVersion"))
    show_version();
  if (get_boolean_resource("printCopyright", "PrintCopyright"))
    show_copyright();
  if (get_boolean_resource("printWarranty", "PrintWarranty"))
    show_warranty();

  verbose = get_boolean_resource("verbose", "Verbose");
  quiet = get_boolean_resource("quiet", "Quiet");

  useXShm = get_boolean_resource("useXShm", "UseXShm");

  useTerminal = get_boolean_resource("useTerminal", "UseTerminal");
  useSerial = get_boolean_resource("useSerial", "UseSerial");
  serialLine = get_string_resource("serialLine", "SerialLine");

  initialize = get_boolean_resource("completeInitialize",
                                    "CompleteInitialize");
  resetOnStartup = get_boolean_resource("resetOnStartup",
                                        "ResetOnStartup");
  romFileName = get_string_resource("romFileName", "RomFileName");
  homeDirectory = get_string_resource("homeDirectory", "HomeDirectory");

  useDebugger = get_boolean_resource("useDebugger", "UseDebugger");
  disassembler_mode = get_mnemonic_resource("disassemblerMnemonics",
                                            "DisassemblerMnemonics");

  netbook = get_boolean_resource("netbook", "Netbook");

  throttle = get_boolean_resource("throttle", "Throttle");
}


char *get_string_resource (char *name, char *class)
{
  return NULL; // get_string_resource_from_db(rdb, name, class);
}

int get_mnemonic_resource (char *name, char *class)
{
  char *tmp, buf [100];
  char *s = get_string_resource (name, class);
  char *os = s;

  if (! s) return CLASS_MNEMONICS;
  for (tmp = buf; *s; s++)
    *tmp++ = isupper (*s) ? _tolower (*s) : *s;
  *tmp = 0;
  free (os);

  if (!strcmp (buf, "hp"))
    return HP_MNEMONICS;
  if (!strcmp (buf, "class"))
    return CLASS_MNEMONICS;
  fprintf (stderr, "%s: %s must be one of \'HP\' or \'class\', not %s.\n",
	   progname, name, buf);
  return CLASS_MNEMONICS;
}

int
#ifdef __FunctionProto__
get_boolean_resource (char *name, char *class)
#else
get_boolean_resource (name, class)
char *name;
char *class;
#endif
{
  char *tmp, buf [100];
  char *s = get_string_resource (name, class);
  char *os = s;
  if (! s) return 0;
  for (tmp = buf; *s; s++)
    *tmp++ = isupper (*s) ? _tolower (*s) : *s;
  *tmp = 0;
  free (os);

  if (!strcmp (buf, "on") || !strcmp (buf, "true") || !strcmp (buf, "yes"))
    return 1;
  if (!strcmp (buf, "off") || !strcmp (buf, "false") || !strcmp (buf, "no"))
    return 0;
  fprintf (stderr, "%s: %s must be boolean, not %s.\n",
	   progname, name, buf);
  return 0;
}

int
#ifdef __FunctionProto__
get_integer_resource (char *name, char *class)
#else
get_integer_resource (name, class)
char *name;
char *class;
#endif
{
  int val;
  char c, *s = get_string_resource (name, class);
  if (!s) return 0;
  if (1 == sscanf (s, " %d %c", &val, &c))
    {
      free (s);
      return val;
    }
  fprintf (stderr, "%s: %s must be an integer, not %s.\n",
	   progname, name, s);
  free (s);
  return 0;
}



