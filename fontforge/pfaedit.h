/* Copyright (C) 2000-2006 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _PFAEDIT_H_
#define _PFAEDIT_H_

#include "configure-pfaedit.h"
#if defined( FONTFORGE_CONFIG_GTK )
# include "gtkbasics.h"
# include <gtk/gtk.h>
# include <gwwv.h>
#else /*if defined( FONTFORGE_CONFIG_GDRAW )*/
# include <basics.h>
# include <stdio.h>
# include <string.h>
# if defined( FONTFORGE_CONFIG_GDRAW )
#  include <gprogress.h>
# endif
#endif
#include "splinefont.h"

static const int unicode4_size = 17*65536;
    /* Unicode goes up to 0x10ffff */


extern void IError(const char *fmt,...);
extern int ErrorWindowExists(void);
extern void ShowErrorWindow(void);
extern void LogError(const char *format,...);

extern void CheckIsScript(int argc, char *argv[]);

extern char *AdobeStandardEncoding[256];
extern int psunicodenames_cnt;
extern const char *psunicodenames[];
extern int32 unicode_from_adobestd[256];
extern struct psaltnames {
    char *name;
    int unicode;
    int provenance;		/* 1=> Adobe PUA, 2=>AMS PUA, 3=>TeX */
} psaltuninames[];
extern int psaltuninames_cnt;

struct unicode_nameannot {
    const char *name, *annot;
};
extern const struct unicode_nameannot * const *const *_UnicodeNameAnnot;

extern int default_fv_font_size;
extern int default_fv_antialias;
extern int default_fv_bbsized;
extern Encoding *default_encoding, custom;
extern int adjustwidth;
extern int adjustlbearing;
extern int autohint_before_rasterize;
extern int autohint_before_generate;
extern int seperate_hint_controls;
extern int ItalicConstrained;
extern int no_windowing_ui;
extern uint32 default_background;
extern int use_utf8_in_script;

extern int new_em_size;
extern int new_fonts_are_order2;
extern int loaded_fonts_same_as_new;

extern char *BDFFoundry, *TTFFoundry;
extern char *xuid;

extern int pagewidth, pageheight, printtype;	/* Printer defaults */
extern char *printcommand, *printlazyprinter;

extern Encoding *enclist;

#define RECENT_MAX	4
extern char *RecentFiles[RECENT_MAX];

#define SCRIPT_MENU_MAX	10
extern unichar_t *script_menu_names[SCRIPT_MENU_MAX];
extern char *script_filenames[SCRIPT_MENU_MAX];

extern MacFeat *default_mac_feature_map;


#endif
