/* Copyright (C) 2000-2004 by George Williams */
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
#include "pfaeditui.h"
#include <math.h>
#include <locale.h>
#include <string.h>
#include "gfile.h"
#include <time.h>
#include <pwd.h>
#if defined(FONTFORGE_CONFIG_GDRAW)
#include "ustring.h"
#include "gio.h"
#include "gicons.h"
#include <utype.h>
#endif

int _ExportEPS(FILE *eps,SplineChar *sc) {
    DBounds b;
    time_t now;
    struct tm *tm;
    int ret;
    char *oldloc;
    const char *author = GetAuthor();

    oldloc = setlocale(LC_NUMERIC,"C");

    fprintf( eps, "%%!PS-Adobe-3.0 EPSF-3.0\n" );
    SplineCharFindBounds(sc,&b);
    fprintf( eps, "%%%%BoundingBox: %g %g %g %g\n", b.minx, b.miny, b.maxx, b.maxy );
    fprintf( eps, "%%%%Pages: 0\n" );
    fprintf( eps, "%%%%Title: %s from %s\n", sc->name, sc->parent->fontname );
    fprintf( eps, "%%%%Creator: FontForge\n" );
    if ( author!=NULL )
	fprintf( eps, "%%%%Author: %s\n", author);
    time(&now);
    tm = localtime(&now);
    fprintf( eps, "%%%%CreationDate: %d:%02d %d-%d-%d\n", tm->tm_hour, tm->tm_min,
	    tm->tm_mday, tm->tm_mon+1, 1900+tm->tm_year );
    fprintf( eps, "%%%%EndComments\n" );
    fprintf( eps, "%%%%EndProlog\n" );
    fprintf( eps, "%%%%Page \"%s\" 1\n", sc->name );

    fprintf( eps, "gsave newpath\n" );
    SC_PSDump((void (*)(int,void *)) fputc,eps,sc,true,false);
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( sc->parent->multilayer )
	fprintf( eps, "grestore\n" );
    else
#endif
	fprintf( eps, "fill grestore\n" );
    fprintf( eps, "%%%%EOF\n" );
    ret = !ferror(eps);
    setlocale(LC_NUMERIC,oldloc);
return( ret );
}

static int ExportEPS(char *filename,SplineChar *sc) {
    FILE *eps;
    int ret;

    eps = fopen(filename,"w");
    if ( eps==NULL ) {
return(0);
    }
    ret = _ExportEPS(eps,sc);
    fclose(eps);
return( ret );
}

int _ExportPDF(FILE *pdf,SplineChar *sc) {
    DBounds b;
    time_t now;
    struct tm *tm;
    int ret;
    char *oldloc;
    uint32 objlocs[8], xrefloc, streamstart, streamlength;
    const char *author = GetAuthor();
    int i;

    oldloc = setlocale(LC_NUMERIC,"C");

    fprintf( pdf, "%%PDF-1.4\n%%\201�\202\203\n" );	/* Header comment + binary comment */
    /* Every document contains a catalog which points to a page tree, which */
    /*  in our case, points to a single page */
    objlocs[1] = ftell(pdf);
    fprintf( pdf, "1 0 obj\n << /Type /Catalog\n    /Pages 2 0 R\n    /PageMode /UseNone\n >>\nendobj\n" );
    objlocs[2] = ftell(pdf);
    fprintf( pdf, "2 0 obj\n << /Type /Pages\n    /Kids [ 3 0 R ]\n    /Count 1\n >>\nendobj\n" );
    /* And our single page points to its contents */
    objlocs[3] = ftell(pdf);
    fprintf( pdf, "3 0 obj\n" );
    fprintf( pdf, " << /Type /Page\n" );
    fprintf( pdf, "    /Parent 2 0 R\n" );
    fprintf( pdf, "    /Resources << >>\n" );
    SplineCharFindBounds(sc,&b);
    fprintf( pdf, "    /MediaBox [%g %g %g %g]\n", b.minx, b.miny, b.maxx, b.maxy );
    fprintf( pdf, "    /Contents 4 0 R\n" );
    fprintf( pdf, " >>\n" );
    fprintf( pdf, "endobj\n" );
    /* And the contents are the interesting stuff */
    objlocs[4] = ftell(pdf);
    fprintf( pdf, "4 0 obj\n" );
    fprintf( pdf, " << /Length 5 0 R >> \n" );
    fprintf( pdf, " stream \n" );
    streamstart = ftell(pdf);
    SC_PSDump((void (*)(int,void *)) fputc,pdf,sc,true,true);
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( !sc->parent->multilayer )
#endif
	fprintf( pdf, "f\n" );
    streamlength = ftell(pdf)-streamstart;
    fprintf( pdf, " endstream\n" );
    fprintf( pdf, "endobj\n" );
    objlocs[5] = ftell(pdf);
    fprintf( pdf, "5 0 obj\n" );
    fprintf( pdf, " %d\n", streamlength );
    fprintf( pdf, "endobj\n" );

    /* Optional Info dict */
    objlocs[6] = ftell(pdf);
    fprintf( pdf, "6 0 obj\n" );
    fprintf( pdf, " <<\n" );
    fprintf( pdf, "    /Creator (FontForge)\n" );
    time(&now);
    tm = localtime(&now);
    tzset();
    fprintf( pdf, "    /CreationDate (D:%04d%02d%02d%02d%2d%02d",
	    1900+tm->tm_year, tm->tm_mon+1, tm->tm_mday,
	    tm->tm_hour, tm->tm_min, tm->tm_sec );
    if ( timezone==0 )
	fprintf( pdf, "Z)\n" );
    else 
	fprintf( pdf, "%+02d')\n", (int) timezone/3600 );	/* doesn't handle half-hour zones */
    fprintf( pdf, "    /Title (%s from %s)\n", sc->name, sc->parent->fontname );
    if ( author!=NULL )
	fprintf( pdf, "    /Author (%s)\n", author );
    fprintf( pdf, " >>\n" );
    fprintf( pdf, "endobj\n" );

    xrefloc = ftell(pdf);
    fprintf( pdf, "xref\n" );
    fprintf( pdf, " 0 7\n" );
    fprintf( pdf, "0000000000 65535 f \n" );
    for ( i=1; i<7; ++i )
	fprintf( pdf, "%010d %05d n \n", objlocs[i], 0 );
    fprintf( pdf, "trailer\n" );
    fprintf( pdf, " <<\n" );
    fprintf( pdf, "    /Size 7\n" );
    fprintf( pdf, "    /Root 1 0 R\n" );
    fprintf( pdf, "    /Info 6 0 R\n" );
    fprintf( pdf, " >>\n" );
    fprintf( pdf, "startxref\n" );
    fprintf( pdf, "%d\n",xrefloc );
    fprintf( pdf, "%%%%EOF\n" );

    ret = !ferror(pdf);
    setlocale(LC_NUMERIC,oldloc);
return( ret );
}

static int ExportPDF(char *filename,SplineChar *sc) {
    FILE *eps;
    int ret;

    eps = fopen(filename,"w");
    if ( eps==NULL ) {
return(0);
    }
    ret = _ExportPDF(eps,sc);
    fclose(eps);
return( ret );
}

static int ExportSVG(char *filename,SplineChar *sc) {
    FILE *svg;
    int ret;

    svg = fopen(filename,"w");
    if ( svg==NULL ) {
return(0);
    }
    ret = _ExportSVG(svg,sc);
    fclose(svg);
return( ret );
}

static void FigDumpPt(FILE *fig,BasePoint *me,real scale,real ascent) {
    fprintf( fig, "%d %d ", (int) rint(me->x*scale), (int) rint(ascent-me->y*scale));
}

static void FigSplineSet(FILE *fig,SplineSet *spl,int spmax, int asc) {
    SplinePoint *sp;
    int cnt;
    real scale = 7*1200.0/spmax;
    real ascent = 11*1200*asc/spmax;

    while ( spl!=NULL ) {
	/* type=3, SPline; sub_type=3, closed interpreted; linestyle=0(solid); thickness=1*/
	/*  colors (pen,fill)=0, black; depth=0(stacking order); pen_style(unused)*/
	/*  area_fill=-1 (no fill) style_val (0.0 no dashes), capstyle=0 (don't care) */
	/*  forward arrow=0 (none) backarrow (none); point count */
	cnt=0;
	sp = spl->first;
	while ( 1 ) {
	    ++cnt;
	    if ( !sp->noprevcp && sp->prev!=NULL ) ++cnt;
	    if ( !sp->nonextcp && sp->next!=NULL ) ++cnt;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( spl->first->prev!=NULL ) {
	    /* Must end with the start point if it's closed */
	    ++cnt;
	}
	fprintf( fig, "3 %d 0 1 0 0 0 0 -1 0.0 0 0 0 %d\n",
		spl->first->prev==NULL?4:5, cnt );
	/* line of coordinates pairs */
	sp = spl->first;
	putc('\t',fig);
	while ( 1 ) {
	    if ( !sp->noprevcp && sp->prev!=NULL && sp!=spl->first )
		FigDumpPt(fig,&sp->prevcp,scale,ascent);
	    FigDumpPt(fig,&sp->me,scale,ascent);
	    if ( !sp->nonextcp && sp->next!=NULL )
		FigDumpPt(fig,&sp->nextcp,scale,ascent);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( spl->first->prev!=NULL ) {
	    /* Must end with the start point if it's closed */
	    if ( !sp->noprevcp && sp->prev!=NULL )
		FigDumpPt(fig,&sp->prevcp,scale,ascent);
	    FigDumpPt(fig,&sp->me,scale,ascent);
	}
	/* line of "shape factors", 0=> corner */
	putc('\n',fig);
	sp = spl->first;
	putc('\t',fig);
	while ( 1 ) {
	    if ( !sp->noprevcp && sp->prev!=NULL && sp!=spl->first )
		fprintf(fig,"1 ");
	    if (( sp->noprevcp && sp->nonextcp ) || sp->pointtype==pt_corner )
		fprintf(fig,"0 ");
	    else
		fprintf(fig,"-1 ");
	    if ( !sp->nonextcp && sp->next!=NULL )
		fprintf(fig,"1 ");
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( spl->first->prev!=NULL ) {
	    /* Must end with the start point if it's closed */
	    if ( !sp->noprevcp && sp->prev!=NULL )
		fprintf(fig,"1 ");
	    if (( sp->noprevcp && sp->nonextcp ) || sp->pointtype==pt_corner )
		fprintf(fig,"0 ");
	    else
		fprintf(fig,"-1 ");
	}
	putc('\n',fig);
	spl=spl->next;
    }
}

static int ExportFig(char *filename,SplineChar *sc) {
    FILE *fig;
    RefChar *rf;
    int ret;
    int spmax = sc->parent->ascent+sc->parent->descent;
    /* This is by no means perfect. but it is a reasonable approximation */

    fig = fopen(filename,"w");
    if ( fig==NULL ) {
return(0);
    }

    fprintf( fig, "#FIG 3.2\n" );
    fprintf( fig, "Portrait\n" );
    fprintf( fig, "Center\n" );
    fprintf( fig, "Inches\n" );
    fprintf( fig, "Letter\n" );
    fprintf( fig, "100.00\n" );
    fprintf( fig, "Single\n" );
    fprintf( fig, "-2\n" );
    fprintf( fig, "1200 2\n" );
    FigSplineSet(fig,sc->layers[ly_fore].splines,spmax,sc->parent->ascent);
    for ( rf=sc->layers[ly_fore].refs; rf!=NULL; rf=rf->next )
	FigSplineSet(fig,rf->layers[0].splines, spmax, sc->parent->ascent );
    ret = !ferror(fig);
    fclose(fig);
return( ret );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static unichar_t def[] = { '1', '0', '0',  '\0' };
static unichar_t def_bits[] = { '1',  '\0' };
static unichar_t *last = NULL;
static unichar_t *last_bits = NULL;

struct sizebits {
    GWindow gw;
    int *pixels, *bits;
    unsigned int done: 1;
    unsigned int good: 1;
};

#define CID_Size	1000
#define CID_Bits	1001

static int SB_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct sizebits *d = GDrawGetUserData(GGadgetGetWindow(g));
	int err=0;
	*d->pixels = GetIntR(d->gw,CID_Size,_STR_PixelSize,&err);
	*d->bits   = GetIntR(d->gw,CID_Bits,_STR_BitsPerPixel,&err);
	if ( err )
return( true );
	if ( *d->bits!=1 && *d->bits!=2 && *d->bits!=4 && *d->bits!=8 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_InvalidBits,_STR_InvalidBits);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("The only valid values for bits/pixel are 1, 2, 4 or 8"),_("The only valid values for bits/pixel are 1, 2, 4 or 8"));
#endif
return( true );
	}
	free( last ); free( last_bits );
	last = GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Size));
	last_bits = GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Bits));
	d->done = true;
	d->good = true;
    }
return( true );
}

static int SB_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct sizebits *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int sb_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct sizebits *d = GDrawGetUserData(gw);
	d->done = true;
    }
return( event->type!=et_char );
}

static int AskSizeBits(int *pixelsize,int *bitsperpixel) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct sizebits sb;

    memset(&sb,'\0',sizeof(sb));
    sb.pixels = pixelsize; sb.bits = bitsperpixel;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(_STR_PixelSizeQ,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = _("Pixel size?");
#endif
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,140));
    pos.height = GDrawPointsToPixels(NULL,100);
    sb.gw = gw = GDrawCreateTopWindow(NULL,&pos,sb_e_h,&sb,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_PixelSize;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 8; gcd[0].gd.pos.y = 8+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 70; gcd[1].gd.pos.y = 8;  gcd[1].gd.pos.width = 65;
    label[1].text = last==NULL ? def : last;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Size;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _STR_BitsPerPixel;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 8; gcd[2].gd.pos.y = 38+6; 
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 70; gcd[3].gd.pos.y = 38;  gcd[3].gd.pos.width = 65;
    label[3].text = last_bits==NULL? def_bits : last_bits;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Bits;
    gcd[3].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = 10-3; gcd[4].gd.pos.y = 38+30-3;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _STR_OK;
    label[4].text_in_resource = true;
    gcd[4].gd.mnemonic = 'O';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = SB_OK;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = -10; gcd[5].gd.pos.y = 38+30;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _STR_Cancel;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.mnemonic = 'C';
    gcd[5].gd.handle_controlevent = SB_Cancel;
    gcd[5].creator = GButtonCreate;

    gcd[6].gd.pos.x = 2; gcd[6].gd.pos.y = 2;
    gcd[6].gd.pos.width = pos.width-4; gcd[22].gd.pos.height = pos.height-2;
    gcd[6].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[6].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Size));
    GTextFieldSelect(GWidgetGetControl(gw,CID_Size),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !sb.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return( sb.good );
}

static int ExportXBM(char *filename,SplineChar *sc, int format) {
    struct _GImage base;
    GImage gi;
    GClut clut;
    BDFChar *bdfc;
    int pixelsize, ret;
    unichar_t *ans;
    int tot, bitsperpixel, i;
    uint8 *pt, *end;
    int scale;
    void *freetypecontext;

    if ( format==0 ) {
	ans = GWidgetAskStringR(_STR_PixelSizeQ,def,_STR_PixelSizeQ);
	if ( ans==NULL )
return( 0 );
	if ( (pixelsize=u_strtol(ans,NULL,10))<=0 )
return( 0 );
	free(last);
	last = ans;
	bitsperpixel=1;
    } else {
	if ( !AskSizeBits(&pixelsize,&bitsperpixel) )
return( 0 );
    }
    if ( autohint_before_rasterize && sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,true);

    memset(&gi,'\0', sizeof(gi));
    memset(&base,'\0', sizeof(base));
    memset(&clut,'\0', sizeof(clut));
    gi.u.image = &base;

    if ( bitsperpixel==1 ) {
	if ( (freetypecontext = FreeTypeFontContext(sc->parent,sc,NULL))==NULL )
	    bdfc = SplineCharRasterize(sc,pixelsize);
	else {
	    bdfc = SplineCharFreeTypeRasterize(freetypecontext,sc->enc,pixelsize,1);
	    FreeTypeFreeContext(freetypecontext);
	}
	BCRegularizeBitmap(bdfc);
	/* Sigh. Bitmaps use a different defn of set than images do. make it consistant */
	tot = bdfc->bytes_per_line*(bdfc->ymax-bdfc->ymin+1);
	for ( pt = bdfc->bitmap, end = pt+tot; pt<end; *pt++ ^= 0xff );

	base.image_type = it_mono;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	base.trans = -1;
	if ( format==0 )
	    ret = GImageWriteXbm(&gi,filename);
#ifndef _NO_LIBPNG
	else if ( format==2 )
	    ret = GImageWritePng(&gi,filename,false);
#endif
	else
	    ret = GImageWriteBmp(&gi,filename);
	BDFCharFree(bdfc);
    } else {
	if ( (freetypecontext = FreeTypeFontContext(sc->parent,sc,NULL))==NULL )
	    bdfc = SplineCharAntiAlias(sc,pixelsize,(1<<(bitsperpixel/2)));
	else {
	    bdfc = SplineCharFreeTypeRasterize(freetypecontext,sc->enc,pixelsize,bitsperpixel);
	    FreeTypeFreeContext(freetypecontext);
	}
	BCRegularizeGreymap(bdfc);
	base.image_type = it_index;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	base.clut = &clut;
	base.trans = -1;
	clut.clut_len = 1<<bitsperpixel;
	clut.is_grey = true;
	clut.trans_index = -1;
	scale = 255/((1<<bitsperpixel)-1);
	scale = COLOR_CREATE(scale,scale,scale);
	for ( i=0; i< 1<<bitsperpixel; ++i )
	    clut.clut[(1<<bitsperpixel)-1 - i] = i*scale;
#ifndef _NO_LIBPNG
	if ( format==2 )
	    ret = GImageWritePng(&gi,filename,false);
	else
#endif
	    ret = GImageWriteBmp(&gi,filename);
	BDFCharFree(bdfc);
    }
return( ret );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static int BCExportXBM(char *filename,BDFChar *bdfc, int format) {
    struct _GImage base;
    GImage gi;
    GClut clut;
    int ret;
    int tot;
    int scale, i;
    uint8 *pt, *end;

    memset(&gi,'\0', sizeof(gi));
    memset(&base,'\0', sizeof(base));
    gi.u.image = &base;

    if ( !bdfc->byte_data ) {
	BCRegularizeBitmap(bdfc);
	/* Sigh. Bitmaps use a different defn of set than images do. make it consistant */
	tot = bdfc->bytes_per_line*(bdfc->ymax-bdfc->ymin+1);
	for ( pt = bdfc->bitmap, end = pt+tot; pt<end; *pt++ ^= 0xff );

	base.image_type = it_mono;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	base.trans = -1;
	if ( format==0 )
	    ret = GImageWriteXbm(&gi,filename);
#ifndef _NO_LIBPNG
	else if ( format==2 )
	    ret = GImageWritePng(&gi,filename,false);
#endif
	else
	    ret = GImageWriteBmp(&gi,filename);
	/* And back to normal */
	for ( pt = bdfc->bitmap, end = pt+tot; pt<end; *pt++ ^= 0xff );
    } else {
	BCRegularizeGreymap(bdfc);
	base.image_type = it_index;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	base.clut = &clut;
	clut.clut_len = 1<<bdfc->depth;
	clut.is_grey = true;
	clut.trans_index = base.trans = -1;
	scale = 255/((1<<bdfc->depth)-1);
	scale = COLOR_CREATE(scale,scale,scale);
	for ( i=0; i< 1<<bdfc->depth; ++i )
	    clut.clut[(1<<bdfc->depth)-1 - i] = i*scale;
#ifndef _NO_LIBPNG
	if ( format==2 )
	    ret = GImageWritePng(&gi,filename,false);
	else
#endif
	    ret = GImageWriteBmp(&gi,filename);
    }
return( ret );
}

void ScriptExport(SplineFont *sf, BDFFont *bdf, int format, int enc) {
    char *ext = format==0?"eps":format==1?"fig":
		format==2?"svg":format==3?"pdf":
		format==4?"xbm":format==5?"bmp":"png";
    char buffer[100];
    SplineChar *sc = sf->chars[enc];
    BDFChar *bc = bdf!=NULL ? bdf->chars[enc] : NULL;
    int good=true;

    if ( sc==NULL )
return;

#if defined( __CygWin ) || defined(__Mac)
    /* Windows file systems are not case conscious */
    /*  nor is the default mac filesystem */
    { char *pt, *bpt, *end;
    bpt = buffer; end = buffer+40;
    for ( pt=sc->name; *pt!='\0' && bpt<end; ) {
	if ( isupper( *pt ))
	    *bpt++ = '$';
	*bpt++ = *pt++;
    }
    sprintf( bpt, "_%.40s.%s", sc->parent->fontname, ext);
    }
#else
    sprintf( buffer, "%.40s_%.40s.%s", sc->name, sc->parent->fontname, ext);
#endif
    if ( format==0 )
	good = ExportEPS(buffer,sc);
    else if ( format==1 )
	good = ExportFig(buffer,sc);
    else if ( format==2 )
	good = ExportSVG(buffer,sc);
    else if ( format==3 )
	good = ExportPDF(buffer,sc);
    else if ( bc!=NULL )
	good = BCExportXBM(buffer,bc,format-3);
    if ( !good )
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Save Failed"),_("Save Failed"));
#else
	GWidgetErrorR(_STR_Savefailedtitle,_STR_Savefailedtitle);
#endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
struct gfc_data {
    int done;
    int ret;
    GGadget *gfc;
    GGadget *format;
    SplineChar *sc;
    BDFChar *bc;
};


static GTextInfo bcformats[] = {
    { (unichar_t *) "X Bitmap", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "BMP", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
#ifndef _NO_LIBPNG
    { (unichar_t *) "png", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
#endif
    { NULL }};

static GTextInfo formats[] = {
    { (unichar_t *) "EPS", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 1, 0, 1 },
    { (unichar_t *) "XFig", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "SVG", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PDF", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "X Bitmap", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "BMP", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
#ifndef _NO_LIBPNG
    { (unichar_t *) "png", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
#endif
    { NULL }};
static int last_format = 0;

static void DoExport(struct gfc_data *d,unichar_t *path) {
    char *temp;
    int format, good;

    temp = cu_copy(path);
    last_format = format = GGadgetGetFirstListSelectedItem(d->format);
    if ( d->bc )
	last_format += 4;
    if ( d->bc!=NULL )
	good = BCExportXBM(temp,d->bc,format);
    else if ( format==0 )
	good = ExportEPS(temp,d->sc);
    else if ( format==1 )
	good = ExportFig(temp,d->sc);
    else if ( format==2 )
	good = ExportSVG(temp,d->sc);
    else if ( format==3 )
	good = ExportPDF(temp,d->sc);
    else
	good = ExportXBM(temp,d->sc,format-4);
    if ( !good )
#if defined(FONTFORGE_CONFIG_GDRAW)
	GWidgetErrorR(_STR_Savefailedtitle,_STR_Savefailedtitle);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Save Failed"),_("Save Failed"));
#endif
    free(temp);
    d->done = good;
    d->ret = good;
}

static void GFD_doesnt(GIOControl *gio) {
    /* The filename the user chose doesn't exist, so everything is happy */
    struct gfc_data *d = gio->userdata;
    DoExport(d,gio->path);
    GFileChooserReplaceIO(d->gfc,NULL);
}

static void GFD_exists(GIOControl *gio) {
    /* The filename the user chose exists, ask user if s/he wants to overwrite */
    struct gfc_data *d = gio->userdata;
    unichar_t buffer[200];
    const unichar_t *rcb[3]; unichar_t rcmn[2];

    rcb[2]=NULL;
#if defined(FONTFORGE_CONFIG_GDRAW)
    rcb[0] = GStringGetResource( _STR_Replace, &rcmn[0]);
#elif defined(FONTFORGE_CONFIG_GTK)
    rcb[0] =  _("Replace");
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
    rcb[1] = GStringGetResource( _STR_Cancel, &rcmn[1]);
#elif defined(FONTFORGE_CONFIG_GTK)
    rcb[1] =  _("Cancel");
#endif

#if defined(FONTFORGE_CONFIG_GDRAW)
    u_strcpy(buffer, GStringGetResource(_STR_Fileexistspre,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
    u_strcpy(buffer, _("File, "));
#endif
    u_strcat(buffer, u_GFileNameTail(gio->path));
#if defined(FONTFORGE_CONFIG_GDRAW)
    u_strcat(buffer, GStringGetResource(_STR_Fileexistspost,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
    u_strcat(buffer, _(", exists. Replace it?"));
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
    if ( GWidgetAsk(GStringGetResource(_STR_Fileexists,NULL),rcb,rcmn,0,1,buffer)==0 ) {
#elif defined(FONTFORGE_CONFIG_GTK)
    if ( GWidgetAsk(_("File Exists"),rcb,rcmn,0,1,buffer)==0 ) {
#endif
	DoExport(d,gio->path);
    }
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_SaveOk(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *tf;
	GFileChooserGetChildren(d->gfc,NULL,NULL,&tf);
	if ( *_GGadgetGetTitle(tf)!='\0' ) {
	    unichar_t *ret = GGadgetGetTitle(d->gfc);

	    GIOfileExists(GFileChooserReplaceIO(d->gfc,
		    GIOCreate(ret,d,GFD_exists,GFD_doesnt)));
	    free(ret);
	}
    }
return( true );
}

static int GFD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
	d->ret = false;
    }
return( true );
}

static int GFD_Format(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *pt, *file, *f2;
	int format = GGadgetGetFirstListSelectedItem(g);
	file = GGadgetGetTitle(d->gfc);
	f2 = galloc(sizeof(unichar_t) * (u_strlen(file)+5));
	u_strcpy(f2,file);
	free(file);
	pt = u_strrchr(f2,'.');
	if ( pt==NULL )
	    pt = f2+u_strlen(f2);
	if ( d->bc!=NULL )
	    uc_strcpy(pt,format==0?".xbm":format==1?".bmp":".png");
	else
	    uc_strcpy(pt,format==0?".eps":
			 format==1?".fig":
			 format==2?".svg":
			 format==3?".pdf":
			 format==4?".xbm":
			 format==5?".bmp":
				   ".png");
	GGadgetSetTitle(d->gfc,f2);
	free(f2);
    }
return( true );
}

static void GFD_dircreated(GIOControl *gio) {
    struct gfc_data *d = gio->userdata;
    unichar_t *dir = u_copy(gio->path);

    GFileChooserReplaceIO(d->gfc,NULL);
    GFileChooserSetDir(d->gfc,dir);
    free(dir);
}

static void GFD_dircreatefailed(GIOControl *gio) {
    /* We couldn't create the directory */
    struct gfc_data *d = gio->userdata;
    unichar_t buffer[500];

#if defined(FONTFORGE_CONFIG_GDRAW)
    u_strcpy(buffer, GStringGetResource(_STR_Couldntcreatedir,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
    u_strcpy(buffer, _("Couldn't create directory"));
#endif
    uc_strcat(buffer,": ");
    u_strcat(buffer, u_GFileNameTail(gio->path));
    uc_strcat(buffer, ".\n");
    if ( gio->error!=NULL ) {
	u_strcat(buffer,gio->error);
	uc_strcat(buffer, "\n");
    }
    if ( gio->status[0]!='\0' )
	u_strcat(buffer,gio->status);
#if defined(FONTFORGE_CONFIG_GDRAW)
    GWidgetPostNotice(GStringGetResource(_STR_Couldntcreatedir,NULL),buffer);
#elif defined(FONTFORGE_CONFIG_GTK)
    GWidgetPostNotice(_("Couldn't create directory"),buffer);
#endif
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_NewDir(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *newdir;
	newdir = GWidgetAskStringR(_STR_Createdir,NULL,_STR_Dirname);
	if ( newdir==NULL )
return( true );
	if ( !u_GFileIsAbsolute(newdir)) {
	    unichar_t *temp = u_GFileAppendFile(GFileChooserGetDir(d->gfc),newdir,false);
	    free(newdir);
	    newdir = temp;
	}
	GIOmkDir(GFileChooserReplaceIO(d->gfc,
		GIOCreate(newdir,d,GFD_dircreated,GFD_dircreatefailed)));
	free(newdir);
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfc_data *d = GDrawGetUserData(gw);
	d->done = true;
	d->ret = false;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_map ) {
	GDrawRaise(gw);
    } else if ( event->type == et_mousemove ||
	    (event->type==et_mousedown && event->u.mouse.button==3 )) {
	struct gfc_data *d = GDrawGetUserData(gw);
	GFileChooserPopupCheck(d->gfc,event);
    } else if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
	struct gfc_data *d = GDrawGetUserData(gw);
return( GGadgetDispatchEvent((GGadget *) (d->gfc),event));
    }
return( true );
}

static int _Export(SplineChar *sc,BDFChar *bc) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[7];
    struct gfc_data d;
    GGadget *pulldown, *files, *tf;
    char buffer[100]; unichar_t ubuf[100];
    char *ext;
    int _format, i;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(_STR_Export,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = _("Export...");
#endif
    pos.x = pos.y = 0;
    totwid = GGadgetScale(223);
    bsbigger = 3*bs+4*14>totwid; totwid = bsbigger?3*bs+4*12:totwid;
    pos.width = GDrawPointsToPixels(NULL,totwid);
    pos.height = GDrawPointsToPixels(NULL,255);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = totwid*100/GIntGetResource(_NUM_ScaleFactor)-24; gcd[0].gd.pos.height = 182;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GFileChooserCreate;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 224-3; gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _STR_Save;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_SaveOk;
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = (totwid-bs)*100/GIntGetResource(_NUM_ScaleFactor)/2; gcd[2].gd.pos.y = 224; gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _STR_Filter;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = -gcd[1].gd.pos.x; gcd[3].gd.pos.y = 224; gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _STR_Cancel;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = gcd[3].gd.pos.x; gcd[4].gd.pos.y = 194; gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) _STR_New;
    label[4].text_in_resource = true;
    label[4].image = &_GIcon_dir;
    label[4].image_precedes = false;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = GFD_NewDir;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = 12; gcd[5].gd.pos.y = 200; gcd[5].gd.pos.width = 0; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled;
    label[5].text = (unichar_t *) _STR_Format;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].creator = GLabelCreate;

    _format = last_format;
    if ( bc!=NULL ) {
	_format-=2;
	if ( _format<0 || _format>2 ) _format = 0;
    }
    gcd[6].gd.pos.x = 55; gcd[6].gd.pos.y = 194; 
    gcd[6].gd.flags = gg_visible | gg_enabled ;
    gcd[6].gd.u.list = bc!=NULL?bcformats:formats;
    if ( bc!=NULL ) {
	bcformats[0].disabled = bc->byte_data;
	if ( _format==0 ) _format=1;
    }
    gcd[6].gd.label = &gcd[6].gd.u.list[_format];
    gcd[6].gd.u.list[0].selected = true;
    gcd[6].gd.handle_controlevent = GFD_Format;
    gcd[6].creator = GListButtonCreate;
    for ( i=0; gcd[6].gd.u.list[i].text!=NULL; ++i )
	gcd[6].gd.u.list[i].selected =false;
    gcd[6].gd.u.list[_format].selected = true;

    GGadgetsCreate(gw,gcd);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    if ( bc!=NULL )
	ext = _format==0 ? "xbm" : _format==1 ? "bmp" : "png";
    else
	ext = _format==0?"eps":_format==1?"fig":_format==2?"svg":
		_format==3?"pdf":_format==4?"xbm":_format==5?"bmp":"png";
#if defined( __CygWin ) || defined(__Mac)
    /* Windows file systems are not case conscious */
    { char *pt, *bpt, *end;
    bpt = buffer; end = buffer+40;
    for ( pt=sc->name; *pt!='\0' && bpt<end; ) {
	if ( isupper( *pt ))
	    *bpt++ = '$';
	*bpt++ = *pt++;
    }
    sprintf( bpt, "_%.40s.%s", sc->parent->fontname, ext);
    }
#else
    sprintf( buffer, "%.40s_%.40s.%s", sc->name, sc->parent->fontname, ext);
#endif
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(gcd[0].ret,ubuf);
    GFileChooserGetChildren(gcd[0].ret,&pulldown,&files,&tf);
    GWidgetIndicateFocusGadget(tf);

    memset(&d,'\0',sizeof(d));
    d.sc = sc;
    d.bc = bc;
    d.gfc = gcd[0].ret;
    d.format = gcd[6].ret;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return(d.ret);
}

int CVExport(CharView *cv) {
return( _Export(cv->sc,NULL));
}

int BVExport(BitmapView *bv) {
return( _Export(bv->bc->sc,bv->bc));
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
