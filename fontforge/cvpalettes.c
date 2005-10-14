/* Copyright (C) 2000-2005 by George Williams */
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

int palettes_docked=0;
int rectelipse=0, polystar=0, regular_star=1;
int center_out[2] = { false, true };
float rr_radius=0;
int ps_pointcnt=6;
float star_percent=1.7320508;	/* Regular 6 pointed star */

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <gkeysym.h>
#include <math.h>
#include "splinefont.h"
#include <ustring.h>
#include <utype.h>
#include <gresource.h>

extern void *font_cvt(char *val, void *def);
extern GDevEventMask input_em[];
extern const int input_em_cnt;

int cvvisible[2] = { 1, 1}, bvvisible[3]= { 1,1,1 };
static GWindow cvlayers, cvtools, bvlayers, bvtools, bvshades;
static GWindow cvlayers2=NULL;
static GPoint cvtoolsoff = { -9999 }, cvlayersoff = { -9999 }, bvlayersoff = { -9999 }, bvtoolsoff = { -9999 }, bvshadesoff = { -9999 };
static int palettesmoved=0;
int palettes_fixed=1;
static GCursor tools[cvt_max+1] = { ct_pointer };

static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a',',','c','a','l','i','b','a','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
static GFont *font;

#define CV_TOOLS_WIDTH		53
#define CV_TOOLS_HEIGHT		(242+4*12+2)
#define CV_LAYERS_WIDTH		104
#define CV_LAYERS_HEIGHT	196
#define CV_LAYERS2_WIDTH	120
#define CV_LAYERS2_LINE_HEIGHT	25
#define CV_LAYERS2_HEADER_HEIGHT	20
#define CV_LAYERS2_VISLAYERS	( (CV_LAYERS_HEIGHT-CV_LAYERS2_HEADER_HEIGHT-2*CV_LAYERS2_LINE_HEIGHT)/CV_LAYERS2_LINE_HEIGHT )
#define BV_TOOLS_WIDTH		53
#define BV_TOOLS_HEIGHT		80
#define BV_LAYERS_HEIGHT	73
#define BV_LAYERS_WIDTH		73
#define BV_SHADES_HEIGHT	(8+9*16)

static void ReparentFixup(GWindow child,GWindow parent, int x, int y, int width, int height ) {
    /* This is so nice */
    /* KDE does not honor my request for a border for top level windows */
    /* KDE does not honor my request for size (for narrow) top level windows */
    /* Gnome gets very confused by reparenting */
	/* If we've got a top level window, then reparenting it removes gnome's */
	/* decoration window, but sets the new parent to root (rather than what */
	/* we asked for */
	/* I have tried reparenting it twice, unmapping & reparenting. Nothing works */

    GWidgetReparentWindow(child,parent,x,y);
    if ( width!=0 )
	GDrawResize(child,width,height);
    GDrawSetWindowBorder(child,1,GDrawGetDefaultForeground(NULL));
}

static GWindow CreatePalette(GWindow w, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs, GWindow v) {
    GWindow gw;
    GPoint pt, base;
    GRect newpos;
    GWindow root;
    GRect ownerpos, screensize;

    pt.x = pos->x; pt.y = pos->y;
    if ( !palettes_fixed ) {
	root = GDrawGetRoot(NULL);
	GDrawGetSize(w,&ownerpos);
	GDrawGetSize(root,&screensize);
	GDrawTranslateCoordinates(w,root,&pt);
	base.x = base.y = 0;
	GDrawTranslateCoordinates(w,root,&base);
	if ( pt.x<0 ) {
	    if ( base.x+ownerpos.width+20+pos->width+20 > screensize.width )
		pt.x=0;
	    else
		pt.x = base.x+ownerpos.width+20;
	}
	if ( pt.y<0 ) pt.y=0;
	if ( pt.x+pos->width>screensize.width )
	    pt.x = screensize.width-pos->width;
	if ( pt.y+pos->height>screensize.height )
	    pt.y = screensize.height-pos->height;
    }
    wattrs->mask |= wam_bordcol|wam_bordwidth;
    wattrs->border_width = 1;
    wattrs->border_color = GDrawGetDefaultForeground(NULL);

    newpos.x = pt.x; newpos.y = pt.y; newpos.width = pos->width; newpos.height = pos->height;
    wattrs->mask |= wam_positioned;
    wattrs->positioned = true;
    gw = GDrawCreateTopWindow(NULL,&newpos,eh,user_data,wattrs);
    if ( palettes_docked )
	ReparentFixup(gw,v,0,pos->y,pos->width,pos->height);
return( gw );
}

static void SaveOffsets(GWindow main, GWindow palette, GPoint *off) {
    if ( !palettes_docked && !palettes_fixed && GDrawIsVisible(palette)) {
	GRect mr, pr;
	GWindow root, temp;
	root = GDrawGetRoot(NULL);
	while ( (temp=GDrawGetParentWindow(main))!=root )
	    main = temp;
	GDrawGetSize(main,&mr);
	GDrawGetSize(palette,&pr);
	off->x = pr.x-mr.x;
	off->y = pr.y-mr.y;
#if 0
 printf( "%s is offset (%d,%d)\n", palette==cvtools?"CVTools":
     palette==cvlayers?"CVLayers":palette==bvtools?"BVTools":
     palette==bvlayers?"BVLayers":"BVShades", off->x, off->y );
#endif
    }
}

static void RestoreOffsets(GWindow main, GWindow palette, GPoint *off) {
    GPoint pt;
    GWindow root,temp;
    GRect screensize, pos;

    if ( palettes_fixed )
return;
    pt = *off;
    root = GDrawGetRoot(NULL);
    GDrawGetSize(root,&screensize);
    GDrawGetSize(palette,&pos);
    while ( (temp=GDrawGetParentWindow(main))!=root )
	main = temp;
    GDrawTranslateCoordinates(main,root,&pt);
    if ( pt.x<0 ) pt.x=0;
    if ( pt.y<0 ) pt.y=0;
    if ( pt.x+pos.width>screensize.width )
	pt.x = screensize.width-pos.width;
    if ( pt.y+pos.height>screensize.height )
	pt.y = screensize.height-pos.height;
    palettesmoved = true;
    GDrawTrueMove(palette,pt.x,pt.y);
    GDrawRaise(palette);
}

/* Note: If you change this ordering, change enum cvtools */
static char *popupsres[] = { N_("Pointer"), N_("Magnify (Minify with alt)"),
				    N_("Draw a freehand curve"), N_("Scroll by hand"),
				    N_("Add a curve point"), N_("Add a corner point"),
			            N_("Add a tangent point"), N_("Add a point, then drag out its control points"),
			            N_("Cut splines in two"), N_("Measure distance, angle between points"),
			            N_("Scale the selection"), N_("Flip the selection"),
			            N_("Rotate the selection"), N_("Skew the selection"),
			            N_("Rotate the selection in 3D and project back to plain"), N_("Perform a perspective transformation on the selection"),
			            N_("Rectangle or Ellipse"), N_("Polygon or Star"),
			            N_("Rectangle or Ellipse"), N_("Polygon or Star")};
static char *editablelayers[] = { N_("F_ore"), N_("_Back"), N_("_Guide") };
static real raddiam_x = 20, raddiam_y = 20, rotate_by=0;
static StrokeInfo expand = { 25, lj_round, lc_butt, si_centerline,
	    /* toobigwarn */  true,
	    /* removeexternal */ false,
	    /* removeinternal */ false,
	    /* removeoverlapif*/ false,
	    /* gottoobig */	 false,
	    /* gottoobiglocal */ false,
	3.1415926535897932/4, .2, 50 };

real CVRoundRectRadius(void) {
return( rr_radius );
}

int CVRectElipseCenter(void) {
return( center_out[rectelipse] );
}

int CVPolyStarPoints(void) {
return( ps_pointcnt );
}

real CVStarRatio(void) {
    if ( regular_star )
return( sin(3.1415926535897932/ps_pointcnt)*tan(2*3.1415926535897932/ps_pointcnt)+cos(3.1415926535897932/ps_pointcnt) );
	
return( star_percent );
}

StrokeInfo *CVFreeHandInfo(void) {
return( &expand );
}
    
struct ask_info {
    GWindow gw;
    int done;
    int ret;
    float *val;
    int *co;
    GGadget *rb1;
    GGadget *reg;
    GGadget *pts;
    int ispolystar;
    int haspos;
    char *lab;
    CharView *cv;
};
#define CID_ValText		1001
#define CID_PointPercent	1002
#define CID_CentCornLab		1003
#define CID_CentCornX		1004
#define CID_CentCornY		1005
#define CID_RadDiamLab		1006
#define CID_RadDiamX		1007
#define CID_RadDiamY		1008
#define CID_Angle		1009

static void FakeShapeEvents(CharView *cv) {
    GEvent event;
    real trans[6];

    cv->active_tool = rectelipse ? cvt_elipse : cvt_rect;
    GDrawSetCursor(cv->v,tools[cv->active_tool]);
    GDrawSetCursor(cvtools,tools[cv->active_tool]);
    cv->showing_tool = cv->active_tool;

    memset(&event,0,sizeof(event));
    event.type = et_mousedown;
    CVMouseDownShape(cv,&event);
    cv->info.x += raddiam_x;
    cv->info.y += raddiam_y;
    CVMouseMoveShape(cv);
    CVMouseUpShape(cv);
    if ( raddiam_x!=0 && raddiam_y!=0 && rotate_by!=0 ) {
	trans[0] = trans[3] = cos ( rotate_by*3.1415926535897932/180. );
	trans[1] = sin( rotate_by*3.1415926535897932/180. );
	trans[2] = -trans[1];
	trans[4] = -cv->p.x*trans[0] - cv->p.y*trans[2] + cv->p.x;
	trans[5] = -cv->p.x*trans[1] - cv->p.y*trans[3] + cv->p.y;
	SplinePointListTransform(cv->layerheads[cv->drawmode]->splines,trans,false);
	SCUpdateAll(cv->sc);
    }
    cv->active_tool = cvt_none;
}

static int TA_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	real val, val2=0;
	int err=0;
	int re = !GGadgetIsChecked(d->rb1);
	if ( d->ispolystar ) {
	    val = GetInt8(d->gw,CID_ValText,d->lab,&err);
	    if ( !(regular_star = GGadgetIsChecked(d->reg)))
		val2 = GetReal8(d->gw,CID_PointPercent,_("Size of Points"),&err);
	} else {
	    val = GetReal8(d->gw,CID_ValText,d->lab,&err);
	    d->co[re] = !GGadgetIsChecked(d->reg);
	}
	if ( err )
return( true );
	if ( d->haspos ) {
	    real x,y, radx,rady, ang;
	    x = GetInt8(d->gw,CID_CentCornX,_("_X"),&err);
	    y = GetInt8(d->gw,CID_CentCornY,_("_Y"),&err);
	    radx = GetInt8(d->gw,CID_RadDiamX,_("Radius:   "),&err);
	    rady = GetInt8(d->gw,CID_RadDiamY,_("Radius:   "),&err);
	    ang = GetInt8(d->gw,CID_Angle,_("Angle:"),&err);
	    if ( err )
return( true );
	    d->cv->p.x = d->cv->info.x = x;
	    d->cv->p.y = d->cv->info.y = y;
	    raddiam_x = radx; raddiam_y = rady;
	    rotate_by = ang;
	    rectelipse = re;
	    *d->val = val;
	    FakeShapeEvents(d->cv);
	}
	*d->val = val;
	d->ret = re;
	d->done = true;
	if ( !regular_star && d->ispolystar )
	    star_percent = val2/100;
	SavePrefs();
    }
return( true );
}

static int TA_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int TA_CenRadChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	int is_bb = GGadgetIsChecked(d->reg);
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_CentCornLab),
		is_bb ? _("Corner") : _("C_enter"));
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_RadDiamLab),
		is_bb ? _("Diameter:") : _("Radius:   "));
    }
return( true );
}

static int TA_RadChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	int is_ellipse = !GGadgetIsChecked(d->rb1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ValText), !is_ellipse );
	GGadgetSetChecked(d->reg,!center_out[is_ellipse]);
	GGadgetSetChecked(d->pts,center_out[is_ellipse]);
	if ( d->haspos )
	    TA_CenRadChange(g,e);
    }
return( true );
}

static int toolask_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct ask_info *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( event->type!=et_char );
}

static int Ask(char *rb1, char *rb2, int rb, char *lab, float *val, int *co,
	int ispolystar, CharView *cv ) {
    struct ask_info d;
    char buffer[20], buf[20];
    char cenx[20], ceny[20], radx[20], rady[20], angle[20];
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[19];
    GTextInfo label[19];
    int off = ((ispolystar&1)?15:0) + ((ispolystar&2)?84:0);
    int haspos = (ispolystar&2)?1:0;

    ispolystar &= 1;

    d.done = false;
    d.ret = rb;
    d.val = val;
    d.co = co;
    d.ispolystar = ispolystar;
    d.haspos = haspos;
    d.lab = lab;
    d.cv = cv;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Shape Type");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,190));
	pos.height = GDrawPointsToPixels(NULL,120+off);
	d.gw = GDrawCreateTopWindow(NULL,&pos,toolask_e_h,&d,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) rb1;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible | (rb==0?gg_cb_on:0);
	gcd[0].creator = GRadioCreate;

	label[1].text = (unichar_t *) rb2;
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = ispolystar?65:75; gcd[1].gd.pos.y = 5; 
	gcd[1].gd.flags = gg_enabled|gg_visible | (rb==1?gg_cb_on:0);
	gcd[1].creator = GRadioCreate;

	label[2].text = (unichar_t *) lab;
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = 25; 
	gcd[2].gd.flags = gg_enabled|gg_visible ;
	gcd[2].creator = GLabelCreate;

	sprintf( buffer, "%g", *val );
	label[3].text = (unichar_t *) buffer;
	label[3].text_is_1byte = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 40; 
	gcd[3].gd.flags = gg_enabled|gg_visible ;
	gcd[3].gd.cid = CID_ValText;
	gcd[3].creator = GTextFieldCreate;

	gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 85+off;
	gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
	gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[4].text = (unichar_t *) _("_OK");
	label[4].text_is_1byte = true;
	label[4].text_in_resource = true;
	gcd[4].gd.mnemonic = 'O';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.handle_controlevent = TA_OK;
	gcd[4].creator = GButtonCreate;

	gcd[5].gd.pos.x = -20; gcd[5].gd.pos.y = 85+3+off;
	gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
	gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[5].text = (unichar_t *) _("_Cancel");
	label[5].text_is_1byte = true;
	label[5].text_in_resource = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.mnemonic = 'C';
	gcd[5].gd.handle_controlevent = TA_Cancel;
	gcd[5].creator = GButtonCreate;

	if ( ispolystar ) {
	    label[6].text = (unichar_t *) _("Regular");
	    label[6].text_is_1byte = true;
	    gcd[6].gd.label = &label[6];
	    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = 70; 
	    gcd[6].gd.flags = gg_enabled|gg_visible | (rb==0?gg_cb_on:0);
	    gcd[6].creator = GRadioCreate;

	    label[7].text = (unichar_t *) _("Points:");
	    label[7].text_is_1byte = true;
	    gcd[7].gd.label = &label[7];
	    gcd[7].gd.pos.x = 65; gcd[7].gd.pos.y = 70; 
	    gcd[7].gd.flags = gg_enabled|gg_visible | (rb==1?gg_cb_on:0);
	    gcd[7].creator = GRadioCreate;

	    sprintf( buf, "%4g", star_percent*100 );
	    label[8].text = (unichar_t *) buf;
	    label[8].text_is_1byte = true;
	    gcd[8].gd.label = &label[8];
	    gcd[8].gd.pos.x = 125; gcd[8].gd.pos.y = 70;  gcd[8].gd.pos.width=50;
	    gcd[8].gd.flags = gg_enabled|gg_visible ;
	    gcd[8].gd.cid = CID_PointPercent;
	    gcd[8].creator = GTextFieldCreate;

	    label[9].text = (unichar_t *) "%";
	    label[9].text_is_1byte = true;
	    gcd[9].gd.label = &label[9];
	    gcd[9].gd.pos.x = 180; gcd[9].gd.pos.y = 70; 
	    gcd[9].gd.flags = gg_enabled|gg_visible ;
	    gcd[9].creator = GLabelCreate;
	} else {
	    label[6].text = (unichar_t *) _("Bounding Box");
	    label[6].text_is_1byte = true;
	    gcd[6].gd.label = &label[6];
	    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = 65; 
	    gcd[6].gd.flags = gg_enabled|gg_visible | (co[rb]==0?gg_cb_on:0);
	    gcd[6].creator = GRadioCreate;

	    label[7].text = (unichar_t *) _("Center Out");
	    label[7].text_is_1byte = true;
	    gcd[7].gd.label = &label[7];
	    gcd[7].gd.pos.x = 90; gcd[7].gd.pos.y = 65; 
	    gcd[7].gd.flags = gg_enabled|gg_visible | (co[rb]==1?gg_cb_on:0);
	    gcd[7].creator = GRadioCreate;

	    if ( rb )
		gcd[3].gd.flags = gg_visible;
	    gcd[0].gd.handle_controlevent = TA_RadChange;
	    gcd[1].gd.handle_controlevent = TA_RadChange;

	    if ( haspos ) {
		gcd[6].gd.handle_controlevent = TA_CenRadChange;
		gcd[7].gd.handle_controlevent = TA_CenRadChange;

		label[8].text = (unichar_t *) _("_X");
		label[8].text_is_1byte = true;
		label[8].text_in_resource = true;
		gcd[8].gd.label = &label[8];
		gcd[8].gd.pos.x = 70; gcd[8].gd.pos.y = gcd[7].gd.pos.y+15;
		gcd[8].gd.flags = gg_enabled|gg_visible;
		gcd[8].creator = GLabelCreate;

		label[9].text = (unichar_t *) _("_Y");
		label[9].text_is_1byte = true;
		label[9].text_in_resource = true;
		gcd[9].gd.label = &label[9];
		gcd[9].gd.pos.x = 120; gcd[9].gd.pos.y = gcd[8].gd.pos.y;
		gcd[9].gd.flags = gg_enabled|gg_visible;
		gcd[9].creator = GLabelCreate;

		label[10].text = (unichar_t *) (co[rb] ? _("C_enter") : _("C_orner") );
		label[10].text_is_1byte = true;
		label[10].text_in_resource = true;
		gcd[10].gd.label = &label[10];
		gcd[10].gd.pos.x = 5; gcd[10].gd.pos.y = gcd[8].gd.pos.y+17;
		gcd[10].gd.flags = gg_enabled|gg_visible;
		gcd[10].gd.cid = CID_CentCornLab;
		gcd[10].creator = GLabelCreate;

		sprintf( cenx, "%g", cv->info.x );
		label[11].text = (unichar_t *) cenx;
		label[11].text_is_1byte = true;
		gcd[11].gd.label = &label[11];
		gcd[11].gd.pos.x = 60; gcd[11].gd.pos.y = gcd[10].gd.pos.y-4;
		gcd[11].gd.pos.width = 40;
		gcd[11].gd.flags = gg_enabled|gg_visible;
		gcd[11].gd.cid = CID_CentCornX;
		gcd[11].creator = GTextFieldCreate;

		sprintf( ceny, "%g", cv->info.y );
		label[12].text = (unichar_t *) ceny;
		label[12].text_is_1byte = true;
		gcd[12].gd.label = &label[12];
		gcd[12].gd.pos.x = 110; gcd[12].gd.pos.y = gcd[11].gd.pos.y;
		gcd[12].gd.pos.width = gcd[11].gd.pos.width;
		gcd[12].gd.flags = gg_enabled|gg_visible;
		gcd[12].gd.cid = CID_CentCornY;
		gcd[12].creator = GTextFieldCreate;

		label[13].text = (unichar_t *) (co[rb] ? _("Radius:   ") : _("Diameter:") );
		label[13].text_is_1byte = true;
		gcd[13].gd.label = &label[13];
		gcd[13].gd.pos.x = 5; gcd[13].gd.pos.y = gcd[10].gd.pos.y+24;
		gcd[13].gd.flags = gg_enabled|gg_visible;
		gcd[13].gd.cid = CID_RadDiamLab;
		gcd[13].creator = GLabelCreate;

		sprintf( radx, "%g", raddiam_x );
		label[14].text = (unichar_t *) radx;
		label[14].text_is_1byte = true;
		gcd[14].gd.label = &label[14];
		gcd[14].gd.pos.x = gcd[11].gd.pos.x; gcd[14].gd.pos.y = gcd[13].gd.pos.y-4;
		gcd[14].gd.pos.width = gcd[11].gd.pos.width;
		gcd[14].gd.flags = gg_enabled|gg_visible;
		gcd[14].gd.cid = CID_RadDiamX;
		gcd[14].creator = GTextFieldCreate;

		sprintf( rady, "%g", raddiam_y );
		label[15].text = (unichar_t *) rady;
		label[15].text_is_1byte = true;
		gcd[15].gd.label = &label[15];
		gcd[15].gd.pos.x = gcd[12].gd.pos.x; gcd[15].gd.pos.y = gcd[14].gd.pos.y;
		gcd[15].gd.pos.width = gcd[11].gd.pos.width;
		gcd[15].gd.flags = gg_enabled|gg_visible;
		gcd[15].gd.cid = CID_RadDiamY;
		gcd[15].creator = GTextFieldCreate;

		label[16].text = (unichar_t *) _("Angle:");
		label[16].text_is_1byte = true;
		gcd[16].gd.label = &label[16];
		gcd[16].gd.pos.x = 5; gcd[16].gd.pos.y = gcd[13].gd.pos.y+24;
		gcd[16].gd.flags = gg_enabled|gg_visible;
		gcd[16].creator = GLabelCreate;

		sprintf( angle, "%g", rotate_by );
		label[17].text = (unichar_t *) angle;
		label[17].text_is_1byte = true;
		gcd[17].gd.label = &label[17];
		gcd[17].gd.pos.x = 60; gcd[17].gd.pos.y = gcd[16].gd.pos.y-4;
		gcd[17].gd.pos.width = gcd[11].gd.pos.width;
		gcd[17].gd.flags = gg_enabled|gg_visible;
		gcd[17].gd.cid = CID_Angle;
		gcd[17].creator = GTextFieldCreate;
	    }
	}
	GGadgetsCreate(d.gw,gcd);
    d.rb1 = gcd[0].ret;
    d.reg = gcd[6].ret;
    d.pts = gcd[7].ret;

    GWidgetHidePalettes();
    GDrawSetVisible(d.gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(d.gw);
return( d.ret );
}

static void CVRectElipse(CharView *cv) {
    rectelipse = Ask(_("Rectangle"),_("Ellipse"),rectelipse,
	    _("Round Rectangle Radius"),&rr_radius,center_out,false, cv);
    GDrawRequestExpose(cvtools,NULL,false);
}

void CVRectEllipsePosDlg(CharView *cv) {
    rectelipse = Ask(_("Rectangle"),_("Ellipse"),rectelipse,
	    _("Round Rectangle Radius"),&rr_radius,center_out,2, cv);
    GDrawRequestExpose(cvtools,NULL,false);
}

static void CVPolyStar(CharView *cv) {
    float temp = ps_pointcnt;
    int foo[2];
    polystar = Ask(_("Polygon"),_("Star"),polystar,
	    _("Number of star points/Polygon verteces"),&temp,foo,true, cv);
    ps_pointcnt = temp;
}

static void ToolsExpose(GWindow pixmap, CharView *cv, GRect *r) {
    GRect old;
    /* Note: If you change this ordering, change enum cvtools */
    static GImage *buttons[][2] = { { &GIcon_pointer, &GIcon_magnify },
				    { &GIcon_freehand, &GIcon_hand },
				    { &GIcon_curve, &GIcon_corner },
				    { &GIcon_tangent, &GIcon_pen },
			            { &GIcon_knife, &GIcon_ruler },
			            { &GIcon_scale, &GIcon_flip },
			            { &GIcon_rotate, &GIcon_skew },
			            { &GIcon_3drotate, &GIcon_perspective },
			            { &GIcon_rect, &GIcon_poly},
			            { &GIcon_elipse, &GIcon_star}};
    static GImage *smalls[] = { &GIcon_smallpointer, &GIcon_smallmag,
				    &GIcon_smallpencil, &GIcon_smallhand,
				    &GIcon_smallcurve, &GIcon_smallcorner,
				    &GIcon_smalltangent, &GIcon_smallpen,
			            &GIcon_smallknife, &GIcon_smallruler,
			            &GIcon_smallscale, &GIcon_smallflip,
			            &GIcon_smallrotate, &GIcon_smallskew,
			            &GIcon_small3drotate, &GIcon_smallperspective,
			            &GIcon_smallrect, &GIcon_smallpoly,
			            &GIcon_smallelipse, &GIcon_smallstar };
    static const unichar_t _Mouse[][9] = {
	    { 'M', 's', 'e', '1',  '\0' },
	    { '^', 'M', 's', 'e', '1',  '\0' },
	    { 'M', 's', 'e', '2',  '\0' },
	    { '^', 'M', 's', 'e', '2',  '\0' }};
    int i,j,norm, mi;
    int tool = cv->cntrldown?cv->cb1_tool:cv->b1_tool;
    int dither = GDrawSetDither(NULL,false);
    GRect temp;

    GDrawPushClip(pixmap,r,&old);
    GDrawFillRect(pixmap,r,GDrawGetDefaultBackground(NULL));
    GDrawSetLineWidth(pixmap,0);
    for ( i=0; i<sizeof(buttons)/sizeof(buttons[0])-1; ++i ) for ( j=0; j<2; ++j ) {
	mi = i;
	if ( i==(cvt_rect)/2 && ((j==0 && rectelipse) || (j==1 && polystar)) )
	    ++mi;
	if ( cv->sc->parent->order2 && buttons[mi][j]==&GIcon_freehand )
	    GDrawDrawImage(pixmap,&GIcon_greyfree,NULL,j*27+1,i*27+1);
	else
	    GDrawDrawImage(pixmap,buttons[mi][j],NULL,j*27+1,i*27+1);
	norm = (mi*2+j!=tool);
	GDrawDrawLine(pixmap,j*27,i*27,j*27+25,i*27,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27,j*27,i*27+25,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27+25,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
	GDrawDrawLine(pixmap,j*27+25,i*27,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
    }
    GDrawSetFont(pixmap,font);
    temp.x = 52-16; temp.y = i*27; temp.width = 16; temp.height = 4*12;
    GDrawFillRect(pixmap,&temp,GDrawGetDefaultBackground(NULL));
    for ( j=0; j<4; ++j ) {
	GDrawDrawText(pixmap,2,i*27+j*12+10,_Mouse[j],-1,NULL,0x000000);
	if ( (&cv->b1_tool)[j]!=cvt_none )
	    GDrawDrawImage(pixmap,smalls[(&cv->b1_tool)[j]],NULL,52-16,i*27+j*12);
    }
    GDrawPopClip(pixmap,&old);
    GDrawSetDither(NULL,dither);
}

int TrueCharState(GEvent *event) {
    int bit = 0;
    /* X doesn't set the state until after the event. I want the state to */
    /*  reflect whatever key just got depressed/released */
    int keysym = event->u.chr.keysym;

    if ( keysym == GK_Caps_Lock || keysym == GK_Shift_Lock ) {
	static int set_on_last_down = false;
	/* caps lock is sticky and doesn't work like the other modifiers */
	/* but it is even worse. the bit seems to be set on key down, but */
	/* unset on key up. In other words on key up, the bit will always */
	/* set and we have no idea which way it will go. So we guess, and */
	/* if they haven't messed with the key outside ff we should be right */
	if ( event->type == et_char ) {
	    set_on_last_down = (event->u.chr.state ^ ksm_capslock)& ksm_capslock;
return( event->u.chr.state ^ ksm_capslock );
	} else if ( !(event->u.chr.state & ksm_capslock) || set_on_last_down )
return( event->u.chr.state );
	else
return( event->u.chr.state & ~ksm_capslock );
    }

    if ( keysym == GK_Meta_L || keysym == GK_Meta_R ||
	    keysym == GK_Alt_L || keysym == GK_Alt_R )
	bit = ksm_meta;
    else if ( keysym == GK_Shift_L || keysym == GK_Shift_R )
	bit = ksm_shift;
    else if ( keysym == GK_Control_L || keysym == GK_Control_R )
	bit = ksm_control;
    else if ( keysym == GK_Super_L || keysym == GK_Super_L )
	bit = ksm_super;
    else if ( keysym == GK_Hyper_L || keysym == GK_Hyper_L )
	bit = ksm_hyper;
    else
return( event->u.chr.state );

    if ( event->type == et_char )
return( event->u.chr.state | bit );
    else
return( event->u.chr.state & ~bit );
}

void CVToolsSetCursor(CharView *cv, int state, char *device) {
    int shouldshow;
    int cntrl;

    if ( tools[0] == ct_pointer ) {
	tools[cvt_pointer] = ct_mypointer;
	tools[cvt_magnify] = ct_magplus;
	tools[cvt_freehand] = ct_pencil;
	tools[cvt_hand] = ct_myhand;
	tools[cvt_curve] = ct_circle;
	tools[cvt_corner] = ct_square;
	tools[cvt_tangent] = ct_triangle;
	tools[cvt_pen] = ct_pen;
	tools[cvt_knife] = ct_knife;
	tools[cvt_ruler] = ct_ruler;
	tools[cvt_scale] = ct_scale;
	tools[cvt_flip] = ct_flip;
	tools[cvt_rotate] = ct_rotate;
	tools[cvt_skew] = ct_skew;
	tools[cvt_3d_rotate] = ct_3drotate;
	tools[cvt_perspective] = ct_perspective;
	tools[cvt_rect] = ct_rect;
	tools[cvt_poly] = ct_poly;
	tools[cvt_elipse] = ct_elipse;
	tools[cvt_star] = ct_star;
	tools[cvt_minify] = ct_magminus;
    }

    shouldshow = cvt_none;
    if ( cv->active_tool!=cvt_none )
	shouldshow = cv->active_tool;
    else if ( cv->pressed_display!=cvt_none )
	shouldshow = cv->pressed_display;
    else if ( device==NULL || strcmp(device,"Mouse1")==0 ) {
	if ( (state&(ksm_shift|ksm_control)) && (state&ksm_button4))
	    shouldshow = cvt_magnify;
	else if ( (state&(ksm_shift|ksm_control)) && (state&ksm_button5))
	    shouldshow = cvt_minify;
	else if ( (state&ksm_control) && (state&(ksm_button2|ksm_super)) )
	    shouldshow = cv->cb2_tool;
	else if ( (state&(ksm_button2|ksm_super)) )
	    shouldshow = cv->b2_tool;
	else if ( (state&ksm_control) )
	    shouldshow = cv->cb1_tool;
	else
	    shouldshow = cv->b1_tool;
    } else if ( strcmp(device,"eraser")==0 )
	shouldshow = cv->er_tool;
    else if ( strcmp(device,"stylus")==0 ) {
	if ( (state&(ksm_button2|ksm_control|ksm_super)) )
	    shouldshow = cv->s2_tool;
	else
	    shouldshow = cv->s1_tool;
    }
    if ( shouldshow==cvt_magnify && (state&ksm_alt))
	shouldshow = cvt_minify;
    if ( shouldshow!=cv->showing_tool ) {
	GDrawSetCursor(cv->v,tools[shouldshow]);
	if ( cvtools!=NULL )	/* Might happen if window owning docked palette destroyed */
	    GDrawSetCursor(cvtools,tools[shouldshow]);
	cv->showing_tool = shouldshow;
    }

    if ( device==NULL || strcmp(device,"stylus")==0 ) {
	cntrl = (state&ksm_control)?1:0;
	if ( device!=NULL && (state&ksm_button2))
	    cntrl = true;
	if ( cntrl != cv->cntrldown ) {
	    cv->cntrldown = cntrl;
	    GDrawRequestExpose(cvtools,NULL,false);
	}
    }
}

static int CVCurrentTool(CharView *cv, GEvent *event) {
    if ( event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"eraser")==0 )
return( cv->er_tool );
    else if ( event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"stylus")==0 ) {
	if ( event->u.mouse.button==2 )
	    /* Only thing that matters is touch which maps to button 1 */;
	else if ( cv->had_control )
return( cv->s2_tool );
	else
return( cv->s1_tool );
    }
    if ( cv->had_control && event->u.mouse.button==2 )
return( cv->cb2_tool );
    else if ( event->u.mouse.button==2 )
return( cv->b2_tool );
    else if ( cv->had_control ) {
return( cv->cb1_tool );
    } else {
return( cv->b1_tool );
    }
}

static void ToolsMouse(CharView *cv, GEvent *event) {
    int i = (event->u.mouse.y/27), j = (event->u.mouse.x/27), mi=i;
    int pos;
    int isstylus = event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"stylus")==0;
    int styluscntl = isstylus && (event->u.mouse.state&0x200);
    static int settings[2];

    if ( i==(cvt_rect)/2 ) {
	int current = CVCurrentTool(cv,event);
	int changed = false;
	if ( event->type == et_mousedown && event->u.mouse.clicks>=2 ) {
	    rectelipse = settings[0];
	    polystar = settings[1];
	} else if ( event->type == et_mousedown ) {
	    settings[0] = rectelipse; settings[1] = polystar;
	    /* A double click will change the type twice, which leaves it where it was, which is desired */
	    if ( j==0 && ((!rectelipse && current==cvt_rect) || (rectelipse && current==cvt_elipse)) ) {
		rectelipse = !rectelipse;
		changed = true;
	    } else if (j==1 && ((!polystar && current==cvt_poly) || (polystar && current==cvt_star)) ) {
		polystar = !polystar;
		changed = true;
	    }
	    if ( changed ) {
		SavePrefs();
		GDrawRequestExpose(cvtools,NULL,false);
	    }
	}
	if ( (j==0 && rectelipse) || (j==1 && polystar) )
	    ++mi;
    }
    pos = mi*2 + j;
    GGadgetEndPopup();
    /* we have two fewer buttons than commands as two bottons each control two commands */
    if ( pos<0 || pos>=cvt_max )
	pos = cvt_none;
    if ( pos==cvt_freehand && cv->sc->parent->order2 )
return;			/* Not available in order2 spline mode */
    if ( event->type == et_mousedown ) {
	if ( isstylus && event->u.mouse.button==2 )
	    /* Not a real button press, only touch counts. This is a modifier */;
	else {
	    cv->pressed_tool = cv->pressed_display = pos;
	    cv->had_control = ((event->u.mouse.state&ksm_control) || styluscntl)?1:0;
	    event->u.mouse.state |= (1<<(7+event->u.mouse.button));
	}
	if ( event->u.mouse.clicks>=2 &&
		(pos/2 == cvt_scale/2 || pos/2 == cvt_rotate/2 || pos == cvt_3d_rotate ))
	    CVDoTransform(cv,pos);
    } else if ( event->type == et_mousemove ) {
	if ( cv->pressed_tool==cvt_none && pos!=cvt_none )
	    /* Not pressed */
	    GGadgetPreparePopup8(cvtools,_(popupsres[pos]));
	else if ( pos!=cv->pressed_tool || cv->had_control != (((event->u.mouse.state&ksm_control) || styluscntl)?1:0) )
	    cv->pressed_display = cvt_none;
	else
	    cv->pressed_display = cv->pressed_tool;
    } else if ( event->type == et_mouseup ) {
	if ( pos==cvt_freehand && event->u.mouse.clicks==2 ) {
	    FreeHandStrokeDlg(&expand);
	} else if ( i==cvt_rect/2 && event->u.mouse.clicks==2 ) {
	    ((j==0)?CVRectElipse:CVPolyStar)(cv);
	    mi = i;
	    if ( (j==0 && rectelipse) || (j==1 && polystar) )
		++mi;
	    pos = mi*2 + j;
	    cv->pressed_tool = cv->pressed_display = pos;
	}
	if ( pos!=cv->pressed_tool || cv->had_control != (((event->u.mouse.state&ksm_control)||styluscntl)?1:0) )
	    cv->pressed_tool = cv->pressed_display = cvt_none;
	else {
	    if ( event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"eraser")==0 )
		cv->er_tool = pos;
	    else if ( isstylus ) {
	        if ( event->u.mouse.button==2 )
		    /* Only thing that matters is touch which maps to button 1 */;
		else if ( cv->had_control )
		    cv->s2_tool = pos;
		else
		    cv->s1_tool = pos;
	    } else if ( cv->had_control && event->u.mouse.button==2 )
		cv->cb2_tool = pos;
	    else if ( event->u.mouse.button==2 )
		cv->b2_tool = pos;
	    else if ( cv->had_control ) {
		cv->cb1_tool = pos;
	    } else {
		cv->b1_tool = pos;
	    }
	    cv->pressed_tool = cv->pressed_display = cvt_none;
	}
	GDrawRequestExpose(cvtools,NULL,false);
	event->u.chr.state &= ~(1<<(7+event->u.mouse.button));
    }
    CVToolsSetCursor(cv,event->u.mouse.state,event->u.mouse.device);
}

static void PostCharToWindow(GWindow to, GEvent *e) {
    GPoint p;

    p.x = e->u.chr.x; p.y = e->u.chr.y;
    GDrawTranslateCoordinates(e->w,to,&p);
    e->u.chr.x = p.x; e->u.chr.y = p.y;
    e->w = to;
    GDrawPostEvent(e);
}

static int cvtools_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy ) {
	cvtools = NULL;
return( true );
    }

    if ( cv==NULL )
return( true );

    GGadgetPopupExternalEvent(event);
    switch ( event->type ) {
      case et_expose:
	ToolsExpose(gw,cv,&event->u.expose.rect);
      break;
      case et_mousedown:
	ToolsMouse(cv,event);
      break;
      case et_mousemove:
	ToolsMouse(cv,event);
      break;
      case et_mouseup:
	ToolsMouse(cv,event);
      break;
      case et_crossing:
	cv->pressed_display = cvt_none;
	CVToolsSetCursor(cv,event->u.mouse.state,NULL);
      break;
      case et_char: case et_charup:
	if ( cv->had_control != ((event->u.chr.state&ksm_control)?1:0) )
	    cv->pressed_display = cvt_none;
	PostCharToWindow(cv->gw,event);
      break;
      case et_close:
	GDrawSetVisible(gw,false);
      break;
    }
return( true );
}

GWindow CVMakeTools(CharView *cv) {
    GRect r;
    GWindowAttrs wattrs;
    FontRequest rq;

    if ( cvtools!=NULL )
return( cvtools );

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Tools");

    r.width = CV_TOOLS_WIDTH; r.height = CV_TOOLS_HEIGHT;
    if ( cvtoolsoff.x==-9999 ) {
	cvtoolsoff.x = -r.width-6; cvtoolsoff.y = cv->mbh+20;
    }
    r.x = cvtoolsoff.x; r.y = cvtoolsoff.y;
    if ( palettes_docked )
	r.x = r.y = 0;
    cvtools = CreatePalette( cv->gw, &r, cvtools_e_h, NULL, &wattrs, cv->v );

    if ( GDrawRequestDeviceEvents(cvtools,input_em_cnt,input_em)>0 ) {
	/* Success! They've got a wacom tablet */
    }

    memset(&rq,0,sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -10;
    rq.weight = 400;
    font = GDrawInstanciateFont(NULL,&rq);

    if ( cvvisible[1])
	GDrawSetVisible(cvtools,true);
return( cvtools );
}


#define CID_VFore	1001
#define CID_VBack	1002
#define CID_VGrid	1003
#define CID_VHHints	1005
#define CID_VVHints	1006
#define CID_VDHints	1007
#define CID_EFore	1008
#define CID_EBack	1009
#define CID_EGrid	1010
#define CID_VHMetrics	1011
#define CID_VVMetrics	1012
#define CID_VVMetricsLab	1013
#define CID_VBlues	1014
#define CID_VAnchor	1015
#define CID_SB		1016

#ifdef FONTFORGE_CONFIG_TYPE3
struct l2 {
    int active;
    int offtop;
    int current_layers, max_layers;
    BDFChar **layers;
    int sb_start;
    GClut *clut;
    GFont *font;
} layer2 = { 2 };

static BDFChar *BDFCharFromLayer(SplineChar *sc,int layer) {
    SplineChar dummy;
    memset(&dummy,0,sizeof(dummy));
    dummy.layer_cnt = 2;
    dummy.layers = sc->layers+layer-1;
    dummy.parent = sc->parent;
return( SplineCharAntiAlias(&dummy,24,4));
}

static void CVLayers2Set(CharView *cv) {
    int i, top;

    GGadgetSetChecked(GWidgetGetControl(cvlayers2,CID_VFore),cv->showfore);
    GGadgetSetChecked(GWidgetGetControl(cvlayers2,CID_VBack),cv->showback);
    GGadgetSetChecked(GWidgetGetControl(cvlayers2,CID_VGrid),cv->showgrids);

    layer2.offtop = 0;
    for ( i=2; i<layer2.current_layers; ++i ) {
	BDFCharFree(layer2.layers[i]);
	layer2.layers[i]=NULL;
    }
    if ( cv->sc->layer_cnt+1>=layer2.max_layers ) {
	top = cv->sc->layer_cnt+10;
	if ( layer2.layers==NULL )
	    layer2.layers = gcalloc(top,sizeof(BDFChar *));
	else {
	    layer2.layers = grealloc(layer2.layers,top*sizeof(BDFChar *));
	    for ( i=layer2.current_layers; i<top; ++i )
		layer2.layers[i] = NULL;
	}
	layer2.max_layers = top;
    }
    layer2.current_layers = cv->sc->layer_cnt+1;
    for ( i=ly_fore; i<cv->sc->layer_cnt; ++i )
	layer2.layers[i+1] = BDFCharFromLayer(cv->sc,i);
    layer2.active = CVLayer(cv)+1;

    GScrollBarSetBounds(GWidgetGetControl(cvlayers2,CID_SB),0,cv->sc->layer_cnt+1-2,
	    CV_LAYERS2_VISLAYERS);
    if ( layer2.offtop>cv->sc->layer_cnt-1-CV_LAYERS2_VISLAYERS )
	layer2.offtop = cv->sc->layer_cnt-1-CV_LAYERS2_VISLAYERS;
    if ( layer2.offtop<0 ) layer2.offtop = 0;
    GScrollBarSetPos(GWidgetGetControl(cvlayers2,CID_SB),layer2.offtop);

    GDrawRequestExpose(cvlayers2,NULL,false);
}

static void Layers2Expose(CharView *cv,GWindow pixmap,GEvent *event) {
    int i, ll;
    const char *str;
    GRect r;
    struct _GImage base;
    GImage gi;
    int as = (24*cv->sc->parent->ascent)/(cv->sc->parent->ascent+cv->sc->parent->descent);

    if ( event->u.expose.rect.y+event->u.expose.rect.height<CV_LAYERS2_HEADER_HEIGHT )
return;

    r.x = 30; r.width = layer2.sb_start-r.x;
    r.y = CV_LAYERS2_HEADER_HEIGHT;
    r.height = CV_LAYERS2_LINE_HEIGHT-CV_LAYERS2_HEADER_HEIGHT;
    GDrawFillRect(pixmap,&r,GDrawGetDefaultBackground(NULL));

    GDrawSetDither(NULL, false);	/* on 8 bit displays we don't want any dithering */

    memset(&gi,0,sizeof(gi));
    memset(&base,0,sizeof(base));
    gi.u.image = &base;
    base.image_type = it_index;
    base.clut = layer2.clut;
    base.trans = -1;
    GDrawSetFont(pixmap,layer2.font);

    for ( i=(event->u.expose.rect.y-CV_LAYERS2_HEADER_HEIGHT)/CV_LAYERS2_LINE_HEIGHT;
	    i<(event->u.expose.rect.y+event->u.expose.rect.height+CV_LAYERS2_LINE_HEIGHT-1-CV_LAYERS2_HEADER_HEIGHT)/CV_LAYERS2_LINE_HEIGHT;
	    ++i ) {
	ll = i<2 ? i : i+layer2.offtop;
	if ( ll==layer2.active ) {
	    r.x = 30; r.width = layer2.sb_start-r.x;
	    r.y = CV_LAYERS2_HEADER_HEIGHT + i*CV_LAYERS2_LINE_HEIGHT;
	    r.height = CV_LAYERS2_LINE_HEIGHT;
	    GDrawFillRect(pixmap,&r,0x000000);
	}
	GDrawDrawLine(pixmap,r.x,CV_LAYERS2_HEADER_HEIGHT+i*CV_LAYERS2_LINE_HEIGHT,
		r.x+r.width,CV_LAYERS2_HEADER_HEIGHT+i*CV_LAYERS2_LINE_HEIGHT,
		0x808080);
	if ( i==0 || i==1 ) {
	    str = i==0?_("Guide") : _("_Back");
	    GDrawDrawText8(pixmap,r.x+2,CV_LAYERS2_HEADER_HEIGHT + i*CV_LAYERS2_LINE_HEIGHT + (CV_LAYERS2_LINE_HEIGHT-12)/2+12,
		    str,-1,NULL,ll==layer2.active?0xffffff:0x000000);
	} else if ( layer2.offtop+i>=layer2.current_layers ) {
    break;
	} else if ( layer2.layers[layer2.offtop+i]!=NULL ) {
	    BDFChar *bdfc = layer2.layers[layer2.offtop+i];
	    base.data = bdfc->bitmap;
	    base.bytes_per_line = bdfc->bytes_per_line;
	    base.width = bdfc->xmax-bdfc->xmin+1;
	    base.height = bdfc->ymax-bdfc->ymin+1;
	    GDrawDrawImage(pixmap,&gi,NULL,
		    r.x+2+bdfc->xmin,
		    CV_LAYERS2_HEADER_HEIGHT + i*CV_LAYERS2_LINE_HEIGHT+as-bdfc->ymax);
	}
    }
}

#define MID_LayerInfo	1
#define MID_NewLayer	2
#define MID_DelLayer	3
#define MID_First	4
#define MID_Earlier	5
#define MID_Later	6
#define MID_Last	7

static void CVLayer2Invoked(GWindow v, GMenuItem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(v);
    Layer temp;
    int layer = CVLayer(cv);
    SplineChar *sc = cv->sc;
    int i;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_No"); buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif

    switch ( mi->mid ) {
      case MID_LayerInfo:
	if ( !LayerDialog(cv->layerheads[cv->drawmode]))
return;
      break;
      case MID_NewLayer:
	LayerDefault(&temp);
	if ( !LayerDialog(&temp))
return;
	sc->layers = grealloc(sc->layers,(sc->layer_cnt+1)*sizeof(Layer));
	sc->layers[sc->layer_cnt] = temp;
	cv->layerheads[dm_fore] = &sc->layers[sc->layer_cnt];
	cv->layerheads[dm_back] = &sc->layers[ly_back];
	++sc->layer_cnt;
      break;
      case MID_DelLayer:
	if ( sc->layer_cnt==2 )		/* May not delete the last foreground layer */
return;
	if ( gwwv_ask(_("Cannot Be Undone"),(const char **) buts,0,1,_("This operation cannot be undone, do it anyway?"))==1 )
return;
	SplinePointListsFree(sc->layers[layer].splines);
	RefCharsFree(sc->layers[layer].refs);
	ImageListsFree(sc->layers[layer].images);
	UndoesFree(sc->layers[layer].undoes);
	UndoesFree(sc->layers[layer].redoes);
	for ( i=layer+1; i<sc->layer_cnt; ++i )
	    sc->layers[i-1] = sc->layers[i];
	--sc->layer_cnt;
	if ( layer==sc->layer_cnt )
	    cv->layerheads[dm_fore] = &sc->layers[layer-1];
      break;
      case MID_First:
	if ( layer==ly_fore )
return;
	temp = sc->layers[layer];
	for ( i=layer-1; i>=ly_fore; --i )
	    sc->layers[i+1] = sc->layers[i];
	sc->layers[i+1] = temp;
	cv->layerheads[dm_fore] = &sc->layers[ly_fore];
      break;
      case MID_Earlier:
	if ( layer==ly_fore )
return;
	temp = sc->layers[layer];
	sc->layers[layer] = sc->layers[layer-1];
	sc->layers[layer-1] = temp;
	cv->layerheads[dm_fore] = &sc->layers[layer-1];
      break;
      case MID_Later:
	if ( layer==sc->layer_cnt-1 )
return;
	temp = sc->layers[layer];
	sc->layers[layer] = sc->layers[layer+1];
	sc->layers[layer+1] = temp;
	cv->layerheads[dm_fore] = &sc->layers[layer+1];
      break;
      case MID_Last:
	if ( layer==sc->layer_cnt-1 )
return;
	temp = sc->layers[layer];
	for ( i=layer+1; i<sc->layer_cnt; ++i )
	    sc->layers[i-1] = sc->layers[i];
	sc->layers[i-1] = temp;
	cv->layerheads[dm_fore] = &sc->layers[i-1];
      break;
    }
    CVLayers2Set(cv);
    CVCharChangedUpdate(cv);
}

static void Layer2Menu(CharView *cv,GEvent *event, int nolayer) {
    GMenuItem mi[20];
    int i;
    static char *names[] = { N_("Layer Info..."), N_("New Layer..."), N_("Del Layer"), (char *) -1,
	    N_("_First"), N_("_Earlier"), N_("L_ater"), N_("_Last"), NULL };
    static int mids[] = { MID_LayerInfo, MID_NewLayer, MID_DelLayer, -1,
	    MID_First, MID_Earlier, MID_Later, MID_Last, 0 };
    int layer = CVLayer(cv);

    memset(mi,'\0',sizeof(mi));
    for ( i=0; names[i]!=0; ++i ) {
	if ( names[i]!=(char *) -1 ) {
	    mi[i].ti.text = (unichar_t *) _(names[i]);
	    mi[i].ti.text_is_1byte = true;
	} else
	    mi[i].ti.line = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = mids[i];
	mi[i].invoke = CVLayer2Invoked;
	if ( mids[i]!=MID_NewLayer && nolayer )
	    mi[i].ti.disabled = true;
	if (( mids[i]==MID_First || mids[i]==MID_Earlier ) && layer==ly_fore )
	    mi[i].ti.disabled = true;
	if (( mids[i]==MID_Last || mids[i]==MID_Later ) && layer==cv->sc->layer_cnt-1 )
	    mi[i].ti.disabled = true;
	if ( mids[i]==MID_DelLayer && cv->sc->layer_cnt==2 )
	    mi[i].ti.disabled = true;
    }
    GMenuCreatePopupMenu(cvlayers2,event, mi);
}

static void Layer2Scroll(CharView *cv, GEvent *event) {
    int off = 0;
    enum sb sbt = event->u.control.u.sb.type;

    if ( sbt==et_sb_top )
	off = 0;
    else if ( sbt==et_sb_bottom )
	off = cv->sc->layer_cnt-1-CV_LAYERS2_VISLAYERS;
    else if ( sbt==et_sb_up ) {
	off = layer2.offtop-1;
    } else if ( sbt==et_sb_down ) {
	off = layer2.offtop+1;
    } else if ( sbt==et_sb_uppage ) {
	off = layer2.offtop-CV_LAYERS2_VISLAYERS+1;
    } else if ( sbt==et_sb_downpage ) {
	off = layer2.offtop+CV_LAYERS2_VISLAYERS-1;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	off = event->u.control.u.sb.pos;
    }
    if ( off>cv->sc->layer_cnt-1-CV_LAYERS2_VISLAYERS )
	off = cv->sc->layer_cnt-1-CV_LAYERS2_VISLAYERS;
    if ( off<0 ) off=0;
    if ( off==layer2.offtop )
return;
    layer2.offtop = off;
    GScrollBarSetPos(GWidgetGetControl(cvlayers2,CID_SB),off);
    GDrawRequestExpose(cvlayers2,NULL,false);
}

static int cvlayers2_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy ) {
	cvlayers2 = NULL;
return( true );
    }

    if ( cv==NULL )
return( true );

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char: case et_charup:
	PostCharToWindow(cv->gw,event);
      break;
      case et_expose:
	Layers2Expose(cv,gw,event);
      break;
      case et_mousedown: {
	int layer = (event->u.mouse.y-CV_LAYERS2_HEADER_HEIGHT)/CV_LAYERS2_LINE_HEIGHT;
	if ( event->u.mouse.y>CV_LAYERS2_HEADER_HEIGHT ) {
	    if ( layer<2 ) {
		cv->drawmode = layer==0 ? dm_grid : dm_back;
		layer2.active = layer;
	    } else if ( layer-1+layer2.offtop >= cv->sc->layer_cnt ) {
		if ( event->u.mouse.button==3 )
		    Layer2Menu(cv,event,true);
		else
		    GDrawBeep(NULL);
return(true);
	    } else {
		layer2.active = layer+layer2.offtop;
		cv->drawmode = dm_fore;
		cv->layerheads[dm_fore] = &cv->sc->layers[layer-1+layer2.offtop];
	    }
	    GDrawRequestExpose(cvlayers2,NULL,false);
	    GDrawRequestExpose(cv->v,NULL,false);
	    GDrawRequestExpose(cv->gw,NULL,false);	/* the logo (where the scrollbars join) shows what layer we are in */
	    if ( event->u.mouse.button==3 )
		Layer2Menu(cv,event,cv->drawmode!=dm_fore);
	    else if ( event->u.mouse.clicks==2 && cv->drawmode==dm_fore ) {
		if ( LayerDialog(cv->layerheads[cv->drawmode]))
		    CVCharChangedUpdate(cv);
	    }
	}
      } break;
      case et_controlevent:
	if ( event->u.control.subtype == et_radiochanged ) {
	    enum drawmode dm = cv->drawmode;
	    switch(GGadgetGetCid(event->u.control.g)) {
	      case CID_VFore:
		CVShows.showfore = cv->showfore = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VBack:
		CVShows.showback = cv->showback = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VGrid:
		CVShows.showgrids = cv->showgrids = GGadgetIsChecked(event->u.control.g);
	      break;
	    }
	    GDrawRequestExpose(cv->v,NULL,false);
	    if ( dm!=cv->drawmode )
		GDrawRequestExpose(cv->gw,NULL,false);	/* the logo (where the scrollbars join) shows what layer we are in */
	} else
	    Layer2Scroll(cv,event);
      break;
    }
return( true );
}

static void CVMakeLayers2(CharView *cv) {
    GRect r;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[25];
    GTextInfo label[25];
    static GBox radio_box = { bt_none, bs_rect, 0, 0, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    GFont *font;
    FontRequest rq;
    int i;
    extern int _GScrollBar_Width;

    if ( layer2.clut==NULL )
	layer2.clut = _BDFClut(4);
    if ( cvlayers2!=NULL )
return;
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Layers");

    r.width = GGadgetScale(CV_LAYERS2_WIDTH); r.height = CV_LAYERS_HEIGHT;
    if ( cvlayersoff.x==-9999 ) {
	cvlayersoff.x = -r.width-6;
	cvlayersoff.y = cv->mbh+CV_TOOLS_HEIGHT+45/*25*/;	/* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
    }
    r.x = cvlayersoff.x; r.y = cvlayersoff.y;
    if ( palettes_docked ) { r.x = 0; r.y=CV_TOOLS_HEIGHT+2; }
    cvlayers2 = CreatePalette( cv->gw, &r, cvlayers2_e_h, NULL, &wattrs, cv->v );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    memset(&rq,'\0',sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(cvlayers2),&rq);
    for ( i=0; i<sizeof(label)/sizeof(label[0]); ++i )
	label[i].font = font;
    layer2.font = font;

    gcd[0].gd.pos.width = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
    gcd[0].gd.pos.x = CV_LAYERS2_WIDTH-gcd[0].gd.pos.width;
    gcd[0].gd.pos.y = CV_LAYERS2_HEADER_HEIGHT+2*CV_LAYERS2_LINE_HEIGHT;
    gcd[0].gd.pos.height = CV_LAYERS_HEIGHT-gcd[0].gd.pos.y;
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_sb_vert;
    gcd[0].gd.cid = CID_SB;
    gcd[0].creator = GScrollBarCreate;
    layer2.sb_start = gcd[0].gd.pos.x;

    label[1].text = (unichar_t *) _("V");
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 7; gcd[1].gd.pos.y = 5; 
    gcd[1].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[0].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[1].creator = GLabelCreate;

    label[2].text = (unichar_t *) _("Layer");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 30; gcd[2].gd.pos.y = 5; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[2].gd.popup_msg = (unichar_t *) _("Is Layer Editable?");
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = CV_LAYERS2_HEADER_HEIGHT+(CV_LAYERS2_LINE_HEIGHT-12)/2; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[3].gd.cid = CID_VGrid;
    gcd[3].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[3].gd.box = &radio_box;
    gcd[3].creator = GCheckBoxCreate;

    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[3].gd.pos.y+CV_LAYERS2_LINE_HEIGHT; 
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[4].gd.cid = CID_VBack;
    gcd[4].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[4].gd.box = &radio_box;
    gcd[4].creator = GCheckBoxCreate;

    gcd[5].gd.pos.x = 5; gcd[5].gd.pos.y = gcd[4].gd.pos.y+CV_LAYERS2_LINE_HEIGHT; 
    gcd[5].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[5].gd.cid = CID_VFore;
    gcd[5].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[5].gd.box = &radio_box;
    gcd[5].creator = GCheckBoxCreate;

    if ( cv->showgrids ) gcd[3].gd.flags |= gg_cb_on;
    if ( cv->showback ) gcd[4].gd.flags |= gg_cb_on;
    if ( cv->showfore ) gcd[5].gd.flags |= gg_cb_on;

    GGadgetsCreate(cvlayers2,gcd);
    if ( cvvisible[0] )
	GDrawSetVisible(cvlayers2,true);
}

static void LayersSwitch(CharView *cv) {
}

void SCMoreLayers(SplineChar *sc, Layer *old) { /* We've added more layers */
    CharView *curcv, *cv;
    if ( sc->parent==NULL || !sc->parent->multilayer )
return;
    for ( cv=sc->views; cv!=NULL ; cv=cv->next ) {
	cv->layerheads[dm_fore] = &cv->sc->layers[cv->layerheads[dm_fore]-old];
	cv->layerheads[dm_back] = &cv->sc->layers[ly_back];
    }
    if ( cvtools==NULL )
return;
    curcv = GDrawGetUserData(cvtools);
    if ( curcv==NULL || curcv->sc!=sc )
return;
    CVLayers2Set(curcv);
}

void SCLayersChange(SplineChar *sc) { /* many of the foreground layers need to be redrawn */
    CharView *curcv;
    if ( cvtools==NULL || !sc->parent->multilayer )
return;
    curcv = GDrawGetUserData(cvtools);
    if ( curcv==NULL || curcv->sc!=sc )
return;
    CVLayers2Set(curcv);
}

void CVLayerChange(CharView *cv) { /* Current layer needs to be redrawn */
    CharView *curcv;
    int layer;

    if ( cvtools==NULL  || !cv->sc->parent->multilayer )
return;
    curcv = GDrawGetUserData(cvtools);
    if ( curcv!=cv )
return;
    if ( cv->drawmode==dm_grid || cv->drawmode==dm_back )
return;
    layer = CVLayer(cv);
    BDFCharFree(layer2.layers[layer+1]);
    layer2.layers[layer+1] = BDFCharFromLayer(cv->sc,layer);
    GDrawRequestExpose(cvlayers2,NULL,false);
}
#endif

void CVLayersSet(CharView *cv) {

#ifdef FONTFORGE_CONFIG_TYPE3
    if ( cv->sc->parent->multilayer ) {
	CVLayers2Set(cv);
return;
    }
#endif
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VFore),cv->showfore);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VBack),cv->showback);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VGrid),cv->showgrids);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VVHints),cv->showvhints);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VHHints),cv->showhhints);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VDHints),cv->showdhints);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VAnchor),cv->showanchor);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VHMetrics),cv->showhmetrics);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VVMetrics),cv->showvmetrics);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,
		cv->drawmode==dm_fore?CID_EFore:
		cv->drawmode==dm_back?CID_EBack:CID_EGrid ),true);
    GGadgetSetEnabled(GWidgetGetControl(cvlayers,CID_VVMetrics),
	    cv->sc->parent->hasvmetrics);
    GGadgetSetEnabled(GWidgetGetControl(cvlayers,CID_VVMetricsLab),
	    cv->sc->parent->hasvmetrics);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VBlues),cv->showblues);

    GGadgetSetEnabled(GWidgetGetControl(cvlayers,CID_EBack),!cv->show_ft_results);
    GGadgetSetEnabled(GWidgetGetControl(cvlayers,CID_EGrid),!cv->show_ft_results);
}

static int cvlayers_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy ) {
	cvlayers = NULL;
return( true );
    }

    if ( cv==NULL )
return( true );

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char: case et_charup:
	PostCharToWindow(cv->gw,event);
      break;
      case et_controlevent:
	if ( event->u.control.subtype == et_radiochanged ) {
	    enum drawmode dm = cv->drawmode;
	    switch(GGadgetGetCid(event->u.control.g)) {
	      case CID_VFore:
		CVShows.showfore = cv->showfore = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VBack:
		CVShows.showback = cv->showback = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VGrid:
		CVShows.showgrids = cv->showgrids = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VHHints:
		CVShows.showhhints = cv->showhhints =
		    CVShows.showmdy = cv->showmdy = 
			GGadgetIsChecked(event->u.control.g);
		cv->back_img_out_of_date = true;	/* only this cv */
	      break;
	      case CID_VVHints:
		CVShows.showvhints = cv->showvhints =
		    CVShows.showmdx = cv->showmdx = 
			GGadgetIsChecked(event->u.control.g);
		cv->back_img_out_of_date = true;	/* only this cv */
	      break;
	      case CID_VDHints:
		CVShows.showdhints = cv->showdhints =
			GGadgetIsChecked(event->u.control.g);
		cv->back_img_out_of_date = true;	/* only this cv */
	      break;
	      case CID_VAnchor:
		CVShows.showanchor = cv->showanchor = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VHMetrics:
		CVShows.showhmetrics = cv->showhmetrics = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VVMetrics:
		CVShows.showvmetrics = cv->showvmetrics = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VBlues:
		CVShows.showfamilyblues = cv->showfamilyblues =
			CVShows.showblues = cv->showblues = GGadgetIsChecked(event->u.control.g);
		cv->back_img_out_of_date = true;	/* only this cv */
	      break;
	      case CID_EFore:
		cv->drawmode = dm_fore;
		cv->lastselpt = NULL;
	      break;
	      case CID_EBack:
		cv->drawmode = dm_back;
		cv->lastselpt = NULL;
	      break;
	      case CID_EGrid:
		cv->drawmode = dm_grid;
		cv->lastselpt = NULL;
	      break;
	    }
	    GDrawRequestExpose(cv->v,NULL,false);
	    if ( dm!=cv->drawmode )
		GDrawRequestExpose(cv->gw,NULL,false);	/* the logo (where the scrollbars join) shows what layer we are in */
	}
      break;
    }
return( true );
}

int CVPaletteMnemonicCheck(GEvent *event) {
    static struct strmatch { char *str; int cid; } strmatch[] = {
	{ N_("F_ore"), CID_EFore },
	{ N_("_Back"), CID_EBack },
	{ N_("_Guide"), CID_EGrid },
	{ 0 }
    };
    unichar_t mn, mnc;
    int i, ch;
    char *foo;
    GEvent fake;
    GGadget *g;
#ifdef FONTFORGE_CONFIG_TYPE3
    CharView *cv;
#endif

    if ( cvtools==NULL )
return( false );
#ifdef FONTFORGE_CONFIG_TYPE3
    cv = GDrawGetUserData(cvtools);
#endif

    for ( i=0; strmatch[i].str!=0 ; ++i ) {
	for ( foo = _(strmatch[i].str); (ch=utf8_ildb((const char **) &foo))!=0; )
	    if ( ch=='_' )
	break;
	if ( ch=='_' )
	    mnc = utf8_ildb((const char **) &foo);
	else
	    mnc = 0;
	mn = mnc;
	if ( islower(mn)) mnc = toupper(mn);
	else if ( isupper(mn)) mnc = tolower(mn);
	if ( event->u.chr.chars[0]==mn || event->u.chr.chars[0]==mnc ) {
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( cv->sc->parent->multilayer ) {
		fake.type = et_mousedown;
		fake.w = cvlayers;
		fake.u.mouse.x = 40;
		if ( strmatch[i].cid==CID_EGrid ) {
		    fake.u.mouse.y = CV_LAYERS2_HEADER_HEIGHT+12;
		} else if ( strmatch[i].cid==CID_EBack ) {
		    fake.u.mouse.y = CV_LAYERS2_HEADER_HEIGHT+12+CV_LAYERS2_LINE_HEIGHT;
		} else {
		    fake.u.mouse.y = CV_LAYERS2_HEADER_HEIGHT+12+2*CV_LAYERS2_LINE_HEIGHT;
		}
		cvlayers2_e_h(cvlayers2,&fake);
	    } else
#endif
	    {
		g = GWidgetGetControl(cvlayers, strmatch[i].cid);
		if ( !GGadgetIsChecked(g)) {
		    GGadgetSetChecked(g,true);
		    fake.type = et_controlevent;
		    fake.w = cvlayers;
		    fake.u.control.subtype = et_radiochanged;
		    fake.u.control.g = g;
		    cvlayers_e_h(cvlayers,&fake);
		}
	    }
return( true );
	}
    }
return( false );
}

GWindow CVMakeLayers(CharView *cv) {
    GRect r;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[25];
    GTextInfo label[25];
    static GBox radio_box = { bt_none, bs_rect, 0, 0, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    static GFont *radio_font = (GFont *) -1;
    GFont *font = NULL;
    FontRequest rq;
    int i, base;

    if ( radio_font == (GFont *) -1 ) {
	extern FontInstance *_GGadgetInitDefaultBox(char *class,GBox *box, FontInstance *deffont);
	GBox junk;
	memset(&junk,0,sizeof(junk));
	radio_font = _GGadgetInitDefaultBox("GRadio.",&junk,NULL);
    }

    if ( cvlayers!=NULL )
return( cvlayers );
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Layers");

    r.width = GGadgetScale(104); r.height = CV_LAYERS_HEIGHT;
    if ( cvlayersoff.x==-9999 ) {
	cvlayersoff.x = -r.width-6;
	cvlayersoff.y = cv->mbh+CV_TOOLS_HEIGHT+45/*25*/;	/* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
    }
    r.x = cvlayersoff.x; r.y = cvlayersoff.y;
    if ( palettes_docked ) { r.x = 0; r.y=CV_TOOLS_HEIGHT+2; }
    cvlayers = CreatePalette( cv->gw, &r, cvlayers_e_h, NULL, &wattrs, cv->v );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    if ( radio_font != NULL )
	font = radio_font;
    if (font == NULL) {
        memset(&rq,'\0',sizeof(rq));
	rq.family_name = helv;
	rq.point_size = -12;
	rq.weight = 400;
	GDrawInstanciateFont(GDrawGetDisplayOfWindow(cvlayers),&rq);
    }
    for ( i=0; i<sizeof(label)/sizeof(label[0]); ++i )
	label[i].font = font;

    label[0].text = (unichar_t *) _("V");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 7; gcd[0].gd.pos.y = 5; 
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[0].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[0].creator = GLabelCreate;

    label[1].text = (unichar_t *) _("E");
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 30; gcd[1].gd.pos.y = 5; 
    gcd[1].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[1].gd.popup_msg = (unichar_t *) _("Is Layer Editable?");
    gcd[1].creator = GLabelCreate;

    label[2].text = (unichar_t *) _("Layer");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 47; gcd[2].gd.pos.y = 5; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[2].gd.popup_msg = (unichar_t *) _("Is Layer Editable?");
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 21; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[3].gd.cid = CID_VFore;
    gcd[3].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[3].gd.box = &radio_box;
    gcd[3].creator = GCheckBoxCreate;

    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = 38; 
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[4].gd.cid = CID_VBack;
    gcd[4].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[4].gd.box = &radio_box;
    gcd[4].creator = GCheckBoxCreate;

    gcd[5].gd.pos.x = 5; gcd[5].gd.pos.y = 55; 
    gcd[5].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[5].gd.cid = CID_VGrid;
    gcd[5].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[5].gd.box = &radio_box;
    gcd[5].creator = GCheckBoxCreate;

    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = 72; 
    gcd[6].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[6].gd.cid = CID_VHHints;
    gcd[6].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[6].gd.box = &radio_box;
    gcd[6].creator = GCheckBoxCreate;

    gcd[7].gd.pos.x = 5; gcd[7].gd.pos.y = 89; 
    gcd[7].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[7].gd.cid = CID_VVHints;
    gcd[7].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[7].gd.box = &radio_box;
    gcd[7].creator = GCheckBoxCreate;

    gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = 106; 
    gcd[8].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[8].gd.cid = CID_VDHints;
    gcd[8].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[8].gd.box = &radio_box;
    gcd[8].creator = GCheckBoxCreate;

    gcd[9].gd.pos.x = 5; gcd[9].gd.pos.y = 123;
    gcd[9].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[9].gd.cid = CID_VBlues;
    gcd[9].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[9].gd.box = &radio_box;
    gcd[9].creator = GCheckBoxCreate;

    gcd[10].gd.pos.x = 5; gcd[10].gd.pos.y = 140; 
    gcd[10].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[10].gd.cid = CID_VAnchor;
    gcd[10].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[10].gd.box = &radio_box;
    gcd[10].creator = GCheckBoxCreate;

    gcd[11].gd.pos.x = 5; gcd[11].gd.pos.y = 157; 
    gcd[11].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[11].gd.cid = CID_VHMetrics;
    gcd[11].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[11].gd.box = &radio_box;
    gcd[11].creator = GCheckBoxCreate;

    gcd[12].gd.pos.x = 5; gcd[12].gd.pos.y = 174; 
    gcd[12].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[12].gd.cid = CID_VVMetrics;
    gcd[12].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[12].gd.box = &radio_box;
    gcd[12].creator = GCheckBoxCreate;
    base = 13;


    label[base].text = (unichar_t *) _("F_ore");
    label[base].text_is_1byte = true;
    label[base].text_in_resource = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 27; gcd[base].gd.pos.y = 21; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[base].gd.cid = CID_EFore;
    gcd[base].gd.popup_msg = (unichar_t *) _("Is Layer Editable?");
    gcd[base].gd.box = &radio_box;
    gcd[base].creator = GRadioCreate;

    label[base+1].text = (unichar_t *) _("_Back");
    label[base+1].text_is_1byte = true;
    label[base+1].text_in_resource = true;
    gcd[base+1].gd.label = &label[base+1];
    gcd[base+1].gd.pos.x = 27; gcd[base+1].gd.pos.y = 38; 
    gcd[base+1].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[base+1].gd.cid = CID_EBack;
    gcd[base+1].gd.popup_msg = (unichar_t *) _("Is Layer Editable?");
    gcd[base+1].gd.box = &radio_box;
    gcd[base+1].creator = GRadioCreate;

    label[base+2].text = (unichar_t *) _("_Guide");
    label[base+2].text_is_1byte = true;
    label[base+2].text_in_resource = true;
    gcd[base+2].gd.label = &label[base+2];
    gcd[base+2].gd.pos.x = 27; gcd[base+2].gd.pos.y = 55; 
    gcd[base+2].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[base+2].gd.cid = CID_EGrid;
    gcd[base+2].gd.popup_msg = (unichar_t *) _("Is Layer Editable?");
    gcd[base+2].gd.box = &radio_box;
    gcd[base+2].creator = GRadioCreate;

    gcd[base+cv->drawmode].gd.flags |= gg_cb_on;
    base += 3;

    label[base].text = (unichar_t *) _("HHints");
    label[base].text_is_1byte = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 72; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _("VHints");
    label[base].text_is_1byte = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 89; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _("DHints");
    label[base].text_is_1byte = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 106; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _("Blues");
    label[base].text_is_1byte = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 123; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _("Anchors");
    label[base].text_is_1byte = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 140; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _("HMetrics");
    label[base].text_is_1byte = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 157; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _("VMetrics");
    label[base].text_is_1byte = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 174; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base].gd.cid = CID_VVMetricsLab;
    gcd[base++].creator = GLabelCreate;

    gcd[base].gd.pos.x = 1; gcd[base].gd.pos.y = 1;
    gcd[base].gd.pos.width = r.width-2; gcd[base].gd.pos.height = r.height-2;
    gcd[base].gd.flags = gg_enabled | gg_visible|gg_pos_in_pixels;
    gcd[base++].creator = GGroupCreate;

    if ( cv->showfore ) gcd[3].gd.flags |= gg_cb_on;
    if ( cv->showback ) gcd[4].gd.flags |= gg_cb_on;
    if ( cv->showgrids ) gcd[5].gd.flags |= gg_cb_on;
    if ( cv->showhhints ) gcd[6].gd.flags |= gg_cb_on;
    if ( cv->showvhints ) gcd[7].gd.flags |= gg_cb_on;
    if ( cv->showdhints ) gcd[8].gd.flags |= gg_cb_on;
    if ( cv->showblues ) gcd[9].gd.flags |= gg_cb_on;
    if ( cv->showanchor ) gcd[10].gd.flags |= gg_cb_on;
    if ( cv->showhmetrics ) gcd[11].gd.flags |= gg_cb_on;
    if ( cv->showvmetrics ) gcd[12].gd.flags |= gg_cb_on;
    if ( !cv->sc->parent->hasvmetrics ) {
	gcd[12].gd.flags &= ~gg_enabled;
	gcd[base-2].gd.flags &= ~gg_enabled;
    }

    GGadgetsCreate(cvlayers,gcd);
    if ( cvvisible[0] )
	GDrawSetVisible(cvlayers,true);
return( cvlayers );
}

static void CVPopupInvoked(GWindow v, GMenuItem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(v);
    int pos;

    pos = mi->mid;
    if ( (pos==14 && rectelipse) || (pos==15 && polystar ))
	pos += 2;
    if ( cv->had_control ) {
	if ( cv->cb1_tool!=pos ) {
	    cv->cb1_tool = pos;
	    GDrawRequestExpose(cvtools,NULL,false);
	}
    } else {
	if ( cv->b1_tool!=pos ) {
	    cv->b1_tool = pos;
	    GDrawRequestExpose(cvtools,NULL,false);
	}
    }
    CVToolsSetCursor(cv,cv->had_control?ksm_control:0,NULL);
}

static void CVPopupLayerInvoked(GWindow v, GMenuItem *mi, GEvent *e) {
    int cid;
    GGadget *g;
    GEvent fake;

    cid = mi->mid==0 ? CID_EFore : mi->mid==1 ? CID_EBack : CID_EGrid;
    g = GWidgetGetControl(cvlayers,cid);
    if ( !GGadgetIsChecked(g)) {
	GGadgetSetChecked(g,true);
	fake.type = et_controlevent;
	fake.w = cvlayers;
	fake.u.control.subtype = et_radiochanged;
	fake.u.control.g = g;
	cvlayers_e_h(cvlayers,&fake);
    }
}

static void CVPopupSelectInvoked(GWindow v, GMenuItem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(v);

    switch ( mi->mid ) {
      case 0:
	CVPGetInfo(cv);
      break;
      case 1:
	if ( cv->p.ref!=NULL )
	    CharViewCreate(cv->p.ref->sc,cv->fv,-1);
      break;
      case 2:
	CVAddAnchor(cv);
      break;
    }
}

void CVToolsPopup(CharView *cv, GEvent *event) {
    GMenuItem mi[125];
    int i, j, anysel;
    static char *selectables[] = { N_("Get _Info..."), N_("Open Reference"), N_("_Add Anchor"), 0 };

    memset(mi,'\0',sizeof(mi));
    for ( i=0;i<16; ++i ) {
	mi[i].ti.text = (unichar_t *) _(popupsres[i]);
	mi[i].ti.text_in_resource = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = i;
	mi[i].invoke = CVPopupInvoked;
    }

    if ( cvlayers!=NULL && !cv->sc->parent->multilayer ) {
	mi[i].ti.line = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i++].ti.bg = COLOR_DEFAULT;
	for ( j=0;j<3; ++j, ++i ) {
	    mi[i].ti.text = (unichar_t *) editablelayers[j];
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    mi[i].mid = j;
	    mi[i].invoke = CVPopupLayerInvoked;
	}
    }

    anysel = CVTestSelectFromEvent(cv,event);
    mi[i].ti.line = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i++].ti.bg = COLOR_DEFAULT;
    for ( j=0;selectables[j]!=0; ++j, ++i ) {
	mi[i].ti.text = (unichar_t *) _(selectables[j]);
	mi[i].ti.text_is_1byte = true;
	if ( (!anysel && j!=2 ) ||
		( j==0 && cv->p.spline!=NULL ) ||
		( j==1 && cv->p.ref==NULL ))
	    mi[i].ti.disabled = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = j;
	mi[i].invoke = CVPopupSelectInvoked;
    }

    cv->had_control = (event->u.mouse.state&ksm_control)?1:0;
    GMenuCreatePopupMenu(cv->v,event, mi);
}

static void CVPaletteCheck(CharView *cv) {
    if ( cvtools==NULL ) {
	if ( palettes_fixed ) {
	    cvtoolsoff.x = 0; cvtoolsoff.y = 0;
	}
	CVMakeTools(cv);
    }
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( cv->sc->parent->multilayer && cvlayers2==NULL ) {
	if ( palettes_fixed ) {
	    cvlayersoff.x = 0; cvlayersoff.y = CV_TOOLS_HEIGHT+45/*25*/;	/* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
	}
	CVMakeLayers2(cv);
    } else if ( !cv->sc->parent->multilayer && cvlayers==NULL ) {
#else
    if ( cvlayers==NULL ) {
#endif
	if ( palettes_fixed ) {
	    cvlayersoff.x = 0; cvlayersoff.y = CV_TOOLS_HEIGHT+45/*25*/;	/* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
	}
	CVMakeLayers(cv);
    }
}

int CVPaletteIsVisible(CharView *cv,int which) {
    CVPaletteCheck(cv);
    if ( which==1 )
return( cvtools!=NULL && GDrawIsVisible(cvtools) );

#ifdef FONTFORGE_CONFIG_TYPE3
    if ( cv->sc->parent->multilayer )
return( cvlayers2!=NULL && GDrawIsVisible(cvlayers2));
#endif

return( cvlayers!=NULL && GDrawIsVisible(cvlayers) );
}

void CVPaletteSetVisible(CharView *cv,int which,int visible) {
    CVPaletteCheck(cv);
    if ( which==1 && cvtools!=NULL)
	GDrawSetVisible(cvtools,visible );
#ifdef FONTFORGE_CONFIG_TYPE3
    else if ( which==0 && cv->sc->parent->multilayer && cvlayers2!=NULL )
	GDrawSetVisible(cvlayers2,visible );
#endif
    else if ( which==0 && cvlayers!=NULL )
	GDrawSetVisible(cvlayers,visible );
    cvvisible[which] = visible;
    SavePrefs();
}

void CVPalettesRaise(CharView *cv) {
    if ( cvtools!=NULL && GDrawIsVisible(cvtools))
	GDrawRaise(cvtools);
    if ( cvlayers!=NULL && GDrawIsVisible(cvlayers))
	GDrawRaise(cvlayers);
}

void _CVPaletteActivate(CharView *cv,int force) {
    CharView *old;

    CVPaletteCheck(cv);
    if ( (old = GDrawGetUserData(cvtools))!=cv || force) {
	if ( old!=NULL ) {
	    SaveOffsets(old->gw,cvtools,&cvtoolsoff);
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( old->sc->parent->multilayer )
		SaveOffsets(old->gw,cvlayers2,&cvlayersoff);
	    else
#endif
		SaveOffsets(old->gw,cvlayers,&cvlayersoff);
	}
	GDrawSetUserData(cvtools,cv);
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( cv->sc->parent->multilayer ) {
	    LayersSwitch(cv);
	    GDrawSetUserData(cvlayers2,cv);
	} else
#endif
	    GDrawSetUserData(cvlayers,cv);
	if ( palettes_docked ) {
	    ReparentFixup(cvtools,cv->v,0,0,CV_TOOLS_WIDTH,CV_TOOLS_HEIGHT);
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( cv->sc->parent->multilayer )
		ReparentFixup(cvlayers2,cv->v,0,CV_TOOLS_HEIGHT+2,0,0);
	    else
#endif
		ReparentFixup(cvlayers,cv->v,0,CV_TOOLS_HEIGHT+2,0,0);
	} else {
	    if ( cvvisible[0]) {
#ifdef FONTFORGE_CONFIG_TYPE3
		if ( cv->sc->parent->multilayer )
		    RestoreOffsets(cv->gw,cvlayers2,&cvlayersoff);
		else
#endif
		    RestoreOffsets(cv->gw,cvlayers,&cvlayersoff);
	    }
	    if ( cvvisible[1])
		RestoreOffsets(cv->gw,cvtools,&cvtoolsoff);
	}
	GDrawSetVisible(cvtools,cvvisible[1]);
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( cv->sc->parent->multilayer )
	    GDrawSetVisible(cvlayers2,cvvisible[0]);
	else
#endif
	    GDrawSetVisible(cvlayers,cvvisible[0]);
	if ( cvvisible[1]) {
	    cv->showing_tool = cvt_none;
	    CVToolsSetCursor(cv,0,NULL);
	    GDrawRequestExpose(cvtools,NULL,false);
	}
	if ( cvvisible[0])
	    CVLayersSet(cv);
    }
    if ( bvtools!=NULL ) {
	BitmapView *bv = GDrawGetUserData(bvtools);
	if ( bv!=NULL ) {
	    SaveOffsets(bv->gw,bvtools,&bvtoolsoff);
	    SaveOffsets(bv->gw,bvlayers,&bvlayersoff);
	    if ( !bv->shades_hidden )
		SaveOffsets(bv->gw,bvshades,&bvshadesoff);
	    GDrawSetUserData(bvtools,NULL);
	    GDrawSetUserData(bvlayers,NULL);
	    GDrawSetUserData(bvshades,NULL);
	}
	GDrawSetVisible(bvtools,false);
	GDrawSetVisible(bvlayers,false);
	GDrawSetVisible(bvshades,false);
    }
}

void CVPaletteActivate(CharView *cv) {
    _CVPaletteActivate(cv,false);
}

#ifdef FONTFORGE_CONFIG_TYPE3
void SFLayerChange(SplineFont *sf) {
    CharView *old, *cv;
    int i;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];
	for ( cv=sc->views; cv!=NULL; cv=cv->next ) {
	    cv->layerheads[dm_back] = &sc->layers[ly_back];
	    cv->layerheads[dm_fore] = &sc->layers[ly_fore];
	    cv->layerheads[dm_grid] = &sf->grid;
	}
    }

    if ( cvtools==NULL )
return;					/* No charviews open */
    old = GDrawGetUserData(cvtools);
    if ( old==NULL || old->sc->parent!=sf )	/* Irrelevant */
return;
    _CVPaletteActivate(old,true);
}
#endif

void CVPalettesHideIfMine(CharView *cv) {
    if ( cvtools==NULL )
return;
    if ( GDrawGetUserData(cvtools)==cv ) {
	SaveOffsets(cv->gw,cvtools,&cvtoolsoff);
	GDrawSetVisible(cvtools,false);
	GDrawSetUserData(cvtools,NULL);
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( cv->sc->parent->multilayer && cvlayers2!=NULL ) {
	    SaveOffsets(cv->gw,cvlayers2,&cvlayersoff);
	    GDrawSetVisible(cvlayers2,false);
	    GDrawSetUserData(cvlayers2,NULL);
	} else
#endif
	{
	    SaveOffsets(cv->gw,cvlayers,&cvlayersoff);
	    GDrawSetVisible(cvlayers,false);
	    GDrawSetUserData(cvlayers,NULL);
	}
    }
}

int CVPalettesWidth(void) {
#ifdef FONTFORGE_CONFIG_TYPE3
return( GGadgetScale(CV_LAYERS2_WIDTH));
#else
return( GGadgetScale(CV_LAYERS_WIDTH));
#endif
}

/* ************************************************************************** */
/* **************************** Bitmap Palettes ***************************** */
/* ************************************************************************** */

static void BVLayersSet(BitmapView *bv) {
    GGadgetSetChecked(GWidgetGetControl(bvlayers,CID_VFore),bv->showfore);
    GGadgetSetChecked(GWidgetGetControl(bvlayers,CID_VBack),bv->showoutline);
    GGadgetSetChecked(GWidgetGetControl(bvlayers,CID_VGrid),bv->showgrid);
}

static int bvlayers_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy ) {
	bvlayers = NULL;
return( true );
    }

    if ( bv==NULL )
return( true );

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char: case et_charup:
	PostCharToWindow(bv->gw,event);
      break;
      case et_controlevent:
	if ( event->u.control.subtype == et_radiochanged ) {
	    switch(GGadgetGetCid(event->u.control.g)) {
	      case CID_VFore:
		BVShows.showfore = bv->showfore = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VBack:
		BVShows.showoutline = bv->showoutline = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VGrid:
		BVShows.showgrid = bv->showgrid = GGadgetIsChecked(event->u.control.g);
	      break;
	    }
	    GDrawRequestExpose(bv->v,NULL,false);
	}
      break;
    }
return( true );
}

GWindow BVMakeLayers(BitmapView *bv) {
    GRect r;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    static GBox radio_box = { bt_none, bs_rect, 0, 0, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    GFont *font;
    FontRequest rq;
    int i;

    if ( bvlayers!=NULL )
return(bvlayers);
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Layers");

    r.width = GGadgetScale(BV_LAYERS_WIDTH); r.height = BV_LAYERS_HEIGHT;
    r.x = -r.width-6; r.y = bv->mbh+BV_TOOLS_HEIGHT+45/*25*/;	/* 45 is right if there's decor, is in kde, not in twm. Sigh */
    if ( palettes_docked ) {
	r.x = 0; r.y = BV_TOOLS_HEIGHT+4;
    } else if ( palettes_fixed ) {
	r.x = 0; r.y = BV_TOOLS_HEIGHT+45;
    }
    bvlayers = CreatePalette( bv->gw, &r, bvlayers_e_h, bv, &wattrs, bv->v );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    memset(&rq,'\0',sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(bvlayers),&rq);
    for ( i=0; i<sizeof(label)/sizeof(label[0]); ++i )
	label[i].font = font;

    label[0].text = (unichar_t *) _("V");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 7; gcd[0].gd.pos.y = 5; 
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[0].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 1; gcd[1].gd.pos.y = 1;
    gcd[1].gd.pos.width = r.width-2; gcd[1].gd.pos.height = r.height-1;
    gcd[1].gd.flags = gg_enabled | gg_visible|gg_pos_in_pixels;
    gcd[1].creator = GGroupCreate;

    label[2].text = (unichar_t *) "Layer";
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 23; gcd[2].gd.pos.y = 5; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[2].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 21; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[3].gd.cid = CID_VFore;
    gcd[3].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[3].gd.box = &radio_box;
    gcd[3].creator = GCheckBoxCreate;
    label[3].text = (unichar_t *) _("Bitmap");
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];

    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = 37; 
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[4].gd.cid = CID_VBack;
    gcd[4].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[4].gd.box = &radio_box;
    gcd[4].creator = GCheckBoxCreate;
    label[4].text = (unichar_t *) _("Outline");
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];

    gcd[5].gd.pos.x = 5; gcd[5].gd.pos.y = 53; 
    gcd[5].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[5].gd.cid = CID_VGrid;
    gcd[5].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[5].gd.box = &radio_box;
    gcd[5].creator = GCheckBoxCreate;
    label[5].text = (unichar_t *) _("_Guide");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];

    if ( bv->showfore ) gcd[3].gd.flags |= gg_cb_on;
    if ( bv->showoutline ) gcd[4].gd.flags |= gg_cb_on;
    if ( bv->showgrid ) gcd[5].gd.flags |= gg_cb_on;

    GGadgetsCreate(bvlayers,gcd);
    if ( bvvisible[0] )
	GDrawSetVisible(bvlayers,true);
return( bvlayers );
}

struct shades_layout {
    int depth;
    int div;
    int cnt;		/* linear number of squares */
    int size;
};

static void BVShadesDecompose(BitmapView *bv, struct shades_layout *lay) {
    GRect r;
    int temp;

    GDrawGetSize(bvshades,&r);
    lay->depth = BDFDepth(bv->bdf);
    lay->div = 255/((1<<lay->depth)-1);
    lay->cnt = lay->depth==8 ? 16 : lay->depth;
    temp = r.width>r.height ? r.height : r.width;
    lay->size = (temp-8+1)/lay->cnt - 1;
}

static void BVShadesExpose(GWindow pixmap, BitmapView *bv, GRect *r) {
    struct shades_layout lay;
    GRect old;
    int i,j,index;
    GRect block;
    Color bg = default_background;
    int greybg = (3*COLOR_RED(bg)+6*COLOR_GREEN(bg)+COLOR_BLUE(bg))/10;

    GDrawSetLineWidth(pixmap,0);
    BVShadesDecompose(bv,&lay);
    GDrawPushClip(pixmap,r,&old);
    for ( i=0; i<=lay.cnt; ++i ) {
	int p = 3+i*(lay.size+1);
	int m = 8+lay.cnt*(lay.size+1);
	GDrawDrawLine(pixmap,p,0,p,m,bg);
	GDrawDrawLine(pixmap,0,p,m,p,bg);
    }
    block.width = block.height = lay.size;
    for ( i=0; i<lay.cnt; ++i ) {
	block.y = 4 + i*(lay.size+1);
	for ( j=0; j<lay.cnt; ++j ) {
	    block.x = 4 + j*(lay.size+1);
	    index = (i*lay.cnt+j)*lay.div;
	    if (( bv->color >= index - lay.div/2 &&
			bv->color <= index + lay.div/2 ) ||
		 ( bv->color_under_cursor >= index - lay.div/2 &&
		    bv->color_under_cursor <= index + lay.div/2 )) {
		GRect outline;
		outline.x = block.x-1; outline.y = block.y-1;
		outline.width = block.width+1; outline.height = block.height+1;
		GDrawDrawRect(pixmap,&outline,
		    ( bv->color >= index - lay.div/2 &&
			bv->color <= index + lay.div/2 )?0x00ff00:0xffffff);
	    }
	    index = (255-index) * greybg / 255;
	    GDrawFillRect(pixmap,&block,0x010101*index);
	}
    }
}

static void BVShadesMouse(BitmapView *bv, GEvent *event) {
    struct shades_layout lay;
    int i, j;

    GGadgetEndPopup();
    if ( event->type == et_mousemove && !bv->shades_down )
return;
    BVShadesDecompose(bv,&lay);
    if ( event->u.mouse.x<4 || event->u.mouse.y<4 ||
	    event->u.mouse.x>=4+lay.cnt*(lay.size+1) ||
	    event->u.mouse.y>=4+lay.cnt*(lay.size+1) )
return;
    i = (event->u.mouse.y-4)/(lay.size+1);
    j = (event->u.mouse.x-4)/(lay.size+1);
    if ( bv->color != (i*lay.cnt + j)*lay.div ) {
	bv->color = (i*lay.cnt + j)*lay.div;
	GDrawRequestExpose(bvshades,NULL,false);
    }
    if ( event->type == et_mousedown ) bv->shades_down = true;
    else if ( event->type == et_mouseup ) bv->shades_down = false;
    if ( event->type == et_mouseup )
	GDrawRequestExpose(bv->gw,NULL,false);
}

static int bvshades_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy ) {
	bvshades = NULL;
return( true );
    }

    if ( bv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	BVShadesExpose(gw,bv,&event->u.expose.rect);
      break;
      case et_mousemove:
      case et_mouseup:
      case et_mousedown:
	BVShadesMouse(bv,event);
      break;
      break;
      case et_char: case et_charup:
	PostCharToWindow(bv->gw,event);
      break;
      case et_destroy:
      break;
      case et_close:
	GDrawSetVisible(gw,false);
      break;
    }
return( true );
}

static GWindow BVMakeShades(BitmapView *bv) {
    GRect r;
    GWindowAttrs wattrs;

    if ( bvshades!=NULL )
return( bvshades );
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg/*|wam_backcol*/;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_eyedropper;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.background_color = 0xffffff;
    wattrs.utf8_window_title = _("Shades");

    r.width = BV_SHADES_HEIGHT; r.height = r.width;
    r.x = -r.width-6; r.y = bv->mbh+225;
    if ( palettes_docked ) {
	r.x = 0; r.y = BV_TOOLS_HEIGHT+BV_LAYERS_HEIGHT+4;
    } else if ( palettes_fixed ) {
	r.x = 0; r.y = BV_TOOLS_HEIGHT+BV_LAYERS_HEIGHT+90;
    }
    bvshades = CreatePalette( bv->gw, &r, bvshades_e_h, bv, &wattrs, bv->v );
    bv->shades_hidden = BDFDepth(bv->bdf)==1;
    if ( bvvisible[2] && !bv->shades_hidden )
	GDrawSetVisible(bvshades,true);
return( bvshades );
}

static char *bvpopups[] = { N_("Pointer"), N_("Magnify (Minify with alt)"),
				    N_("Set/Clear Pixels"), N_("Draw a Line"),
			            N_("Shift Entire Bitmap"), N_("Scroll Bitmap") };

static void BVToolsExpose(GWindow pixmap, BitmapView *bv, GRect *r) {
    GRect old;
    /* Note: If you change this ordering, change enum bvtools */
    static GImage *buttons[][2] = { { &GIcon_pointer, &GIcon_magnify },
				    { &GIcon_pencil, &GIcon_line },
			            { &GIcon_shift, &GIcon_hand }};
    int i,j,norm;
    int tool = bv->cntrldown?bv->cb1_tool:bv->b1_tool;
    int dither = GDrawSetDither(NULL,false);

    GDrawPushClip(pixmap,r,&old);
    GDrawSetLineWidth(pixmap,0);
    for ( i=0; i<sizeof(buttons)/sizeof(buttons[0]); ++i ) for ( j=0; j<2; ++j ) {
	GDrawDrawImage(pixmap,buttons[i][j],NULL,j*27+1,i*27+1);
	norm = (i*2+j!=tool);
	GDrawDrawLine(pixmap,j*27,i*27,j*27+25,i*27,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27,j*27,i*27+25,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27+25,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
	GDrawDrawLine(pixmap,j*27+25,i*27,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
    }
    GDrawPopClip(pixmap,&old);
    GDrawSetDither(NULL,dither);
}

void BVToolsSetCursor(BitmapView *bv, int state,char *device) {
    int shouldshow;
    static enum bvtools tools[bvt_max2+1] = { bvt_none };
    int cntrl;

    if ( tools[0] == bvt_none ) {
	tools[bvt_pointer] = ct_mypointer;
	tools[bvt_magnify] = ct_magplus;
	tools[bvt_pencil] = ct_pencil;
	tools[bvt_line] = ct_line;
	tools[bvt_shift] = ct_shift;
	tools[bvt_hand] = ct_myhand;
	tools[bvt_minify] = ct_magminus;
	tools[bvt_eyedropper] = ct_eyedropper;
	tools[bvt_setwidth] = ct_setwidth;
	tools[bvt_rect] = ct_rect;
	tools[bvt_filledrect] = ct_filledrect;
	tools[bvt_elipse] = ct_elipse;
	tools[bvt_filledelipse] = ct_filledelipse;
    }

    shouldshow = bvt_none;
    if ( bv->active_tool!=bvt_none )
	shouldshow = bv->active_tool;
    else if ( bv->pressed_display!=bvt_none )
	shouldshow = bv->pressed_display;
    else if ( device==NULL || strcmp(device,"Mouse1")==0 ) {
	if ( (state&(ksm_shift|ksm_control)) && (state&ksm_button4))
	    shouldshow = bvt_magnify;
	else if ( (state&(ksm_shift|ksm_control)) && (state&ksm_button5))
	    shouldshow = bvt_minify;
	else if ( (state&ksm_control) && (state&(ksm_button2|ksm_super)) )
	    shouldshow = bv->cb2_tool;
	else if ( (state&(ksm_button2|ksm_super)) )
	    shouldshow = bv->b2_tool;
	else if ( (state&ksm_control) )
	    shouldshow = bv->cb1_tool;
	else
	    shouldshow = bv->b1_tool;
    } else if ( strcmp(device,"eraser")==0 )
	shouldshow = bv->er_tool;
    else if ( strcmp(device,"stylus")==0 ) {
	if ( (state&(ksm_button2|ksm_control|ksm_super)) )
	    shouldshow = bv->s2_tool;
	else
	    shouldshow = bv->s1_tool;
    }
    
    if ( shouldshow==bvt_magnify && (state&ksm_alt))
	shouldshow = bvt_minify;
    if ( (shouldshow==bvt_pencil || shouldshow==bvt_line) && (state&ksm_alt) && bv->bdf->clut!=NULL )
	shouldshow = bvt_eyedropper;
    if ( shouldshow!=bv->showing_tool ) {
	GDrawSetCursor(bv->v,tools[shouldshow]);
	if ( bvtools != NULL )
	    GDrawSetCursor(bvtools,tools[shouldshow]);
	bv->showing_tool = shouldshow;
    }

    if ( device==NULL || strcmp(device,"stylus")==0 ) {
	cntrl = (state&ksm_control)?1:0;
	if ( device!=NULL && (state&ksm_button2))
	    cntrl = true;
	if ( cntrl != bv->cntrldown ) {
	    bv->cntrldown = cntrl;
	    GDrawRequestExpose(bvtools,NULL,false);
	}
    }
}

static void BVToolsMouse(BitmapView *bv, GEvent *event) {
    int i = (event->u.mouse.y/27), j = (event->u.mouse.x/27);
    int pos;
    int isstylus = event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"stylus")==0;
    int styluscntl = isstylus && (event->u.mouse.state&0x200);

    pos = i*2 + j;
    GGadgetEndPopup();
    if ( pos<0 || pos>=bvt_max )
	pos = bvt_none;
    if ( event->type == et_mousedown ) {
	if ( isstylus && event->u.mouse.button==2 )
	    /* Not a real button press, only touch counts. This is a modifier */;
	else {
	    bv->pressed_tool = bv->pressed_display = pos;
	    bv->had_control = ((event->u.mouse.state&ksm_control) || styluscntl)?1:0;
	    event->u.chr.state |= (1<<(7+event->u.mouse.button));
	}
    } else if ( event->type == et_mousemove ) {
	if ( bv->pressed_tool==bvt_none && pos!=bvt_none ) {
	    /* Not pressed */
	    if ( !bv->shades_hidden && strcmp(bvpopups[pos],"Set/Clear Pixels")==0 )
		GGadgetPreparePopup8(bvtools,_("Set/Clear Pixels\n(Eyedropper with alt)"));
	    else
		GGadgetPreparePopup8(bvtools,_(bvpopups[pos]));
	} else if ( pos!=bv->pressed_tool || bv->had_control != (((event->u.mouse.state&ksm_control)||styluscntl)?1:0) )
	    bv->pressed_display = bvt_none;
	else
	    bv->pressed_display = bv->pressed_tool;
    } else if ( event->type == et_mouseup ) {
	if ( pos!=bv->pressed_tool || bv->had_control != (((event->u.mouse.state&ksm_control)||styluscntl)?1:0) )
	    bv->pressed_tool = bv->pressed_display = bvt_none;
	else {
	    if ( event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"eraser")==0 )
		bv->er_tool = pos;
	    else if ( isstylus ) {
	        if ( event->u.mouse.button==2 )
		    /* Only thing that matters is touch which maps to button 1 */;
		else if ( bv->had_control )
		    bv->s2_tool = pos;
		else
		    bv->s1_tool = pos;
	    } else if ( bv->had_control && event->u.mouse.button==2 )
		bv->cb2_tool = pos;
	    else if ( event->u.mouse.button==2 )
		bv->b2_tool = pos;
	    else if ( bv->had_control ) {
		if ( bv->cb1_tool!=pos ) {
		    bv->cb1_tool = pos;
		    GDrawRequestExpose(bvtools,NULL,false);
		}
	    } else {
		if ( bv->b1_tool!=pos ) {
		    bv->b1_tool = pos;
		    GDrawRequestExpose(bvtools,NULL,false);
		}
	    }
	    bv->pressed_tool = bv->pressed_display = bvt_none;
	}
	event->u.mouse.state &= ~(1<<(7+event->u.mouse.button));
    }
    BVToolsSetCursor(bv,event->u.mouse.state,event->u.mouse.device);
}

static int bvtools_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy ) {
	bvtools = NULL;
return( true );
    }

    if ( bv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	BVToolsExpose(gw,bv,&event->u.expose.rect);
      break;
      case et_mousedown:
	BVToolsMouse(bv,event);
      break;
      case et_mousemove:
	BVToolsMouse(bv,event);
      break;
      case et_mouseup:
	BVToolsMouse(bv,event);
      break;
      case et_crossing:
	bv->pressed_display = bvt_none;
	BVToolsSetCursor(bv,event->u.mouse.state,event->u.mouse.device);
      break;
      case et_char: case et_charup:
	if ( bv->had_control != ((event->u.chr.state&ksm_control)?1:0) )
	    bv->pressed_display = bvt_none;
	PostCharToWindow(bv->gw,event);
      break;
      case et_close:
	GDrawSetVisible(gw,false);
      break;
    }
return( true );
}

GWindow BVMakeTools(BitmapView *bv) {
    GRect r;
    GWindowAttrs wattrs;

    if ( bvtools!=NULL )
return( bvtools );
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Tools");

    r.width = BV_TOOLS_WIDTH; r.height = BV_TOOLS_HEIGHT;
    r.x = -r.width-6; r.y = bv->mbh+20;
    if ( palettes_fixed || palettes_docked ) {
	r.x = 0; r.y = 0;
    }
    bvtools = CreatePalette( bv->gw, &r, bvtools_e_h, bv, &wattrs, bv->v );
    if ( bvvisible[1] )
	GDrawSetVisible(bvtools,true);
return( bvtools );
}

static void BVPopupInvoked(GWindow v, GMenuItem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(v);
    int pos;

    pos = mi->mid;
    if ( bv->had_control ) {
	if ( bv->cb1_tool!=pos ) {
	    bv->cb1_tool = pos;
	    GDrawRequestExpose(bvtools,NULL,false);
	}
    } else {
	if ( bv->b1_tool!=pos ) {
	    bv->b1_tool = pos;
	    GDrawRequestExpose(bvtools,NULL,false);
	}
    }
    BVToolsSetCursor(bv,bv->had_control?ksm_control:0,NULL);
}

void BVToolsPopup(BitmapView *bv, GEvent *event) {
    GMenuItem mi[21];
    int i, j;

    memset(mi,'\0',sizeof(mi));
    for ( i=0;i<6; ++i ) {
	mi[i].ti.text = (unichar_t *) _(bvpopups[i]);
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = i;
	mi[i].invoke = BVPopupInvoked;
    }

    mi[i].ti.text = (unichar_t *) _("Rectangle");
    mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_rect;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) _("Filled Rectangle"); mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_filledrect;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) _("Ellipse"); mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_elipse;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) _("Filled Ellipse"); mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_filledelipse;
    mi[i++].invoke = BVPopupInvoked;

    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i++].ti.line = true;
    for ( j=0; j<6; ++j, ++i ) {
	mi[i].ti.text = (unichar_t *) BVFlipNames[j];
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = j;
	mi[i].invoke = BVMenuRotateInvoked;
    }
    if ( bv->fv->sf->onlybitmaps ) {
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i++].ti.line = true;
	mi[i].ti.text = (unichar_t *) _("Set _Width...");
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.text_in_resource = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = bvt_setwidth;
	mi[i].invoke = BVPopupInvoked;
    }
    bv->had_control = (event->u.mouse.state&ksm_control)?1:0;
    GMenuCreatePopupMenu(bv->v,event, mi);
}

static void BVPaletteCheck(BitmapView *bv) {
    if ( bvtools==NULL ) {
	BVMakeTools(bv);
	BVMakeLayers(bv);
	BVMakeShades(bv);
    }
}

int BVPaletteIsVisible(BitmapView *bv,int which) {
    BVPaletteCheck(bv);
    if ( which==1 )
return( bvtools!=NULL && GDrawIsVisible(bvtools) );
    if ( which==2 )
return( bvshades!=NULL && GDrawIsVisible(bvshades) );

return( bvlayers!=NULL && GDrawIsVisible(bvlayers) );
}

void BVPaletteSetVisible(BitmapView *bv,int which,int visible) {
    BVPaletteCheck(bv);
    if ( which==1 && bvtools!=NULL)
	GDrawSetVisible(bvtools,visible );
    else if ( which==2 && bvshades!=NULL)
	GDrawSetVisible(bvshades,visible );
    else if ( which==0 && bvlayers!=NULL )
	GDrawSetVisible(bvlayers,visible );
    bvvisible[which] = visible;
    SavePrefs();
}

void BVPaletteActivate(BitmapView *bv) {
    BitmapView *old;

    BVPaletteCheck(bv);
    if ( (old = GDrawGetUserData(bvtools))!=bv ) {
	if ( old!=NULL ) {
	    SaveOffsets(old->gw,bvtools,&bvtoolsoff);
	    SaveOffsets(old->gw,bvlayers,&bvlayersoff);
	    SaveOffsets(old->gw,bvshades,&bvshadesoff);
	}
	GDrawSetUserData(bvtools,bv);
	GDrawSetUserData(bvlayers,bv);
	GDrawSetUserData(bvshades,bv);
	if ( palettes_docked ) {
	    ReparentFixup(bvtools,bv->v,0,0,BV_TOOLS_WIDTH,BV_TOOLS_HEIGHT);
	    ReparentFixup(bvlayers,bv->v,0,BV_TOOLS_HEIGHT+2,0,0);
	    ReparentFixup(bvshades,bv->v,0,BV_TOOLS_HEIGHT+BV_TOOLS_HEIGHT+4,0,0);
	} else {
	    if ( bvvisible[0])
		RestoreOffsets(bv->gw,bvlayers,&bvlayersoff);
	    if ( bvvisible[1])
		RestoreOffsets(bv->gw,bvtools,&bvtoolsoff);
	    if ( bvvisible[2] && !bv->shades_hidden )
		RestoreOffsets(bv->gw,bvshades,&bvshadesoff);
	}
	GDrawSetVisible(bvtools,bvvisible[1]);
	GDrawSetVisible(bvlayers,bvvisible[0]);
	GDrawSetVisible(bvshades,bvvisible[2] && bv->bdf->clut!=NULL);
	if ( bvvisible[1]) {
	    bv->showing_tool = bvt_none;
	    BVToolsSetCursor(bv,0,NULL);
	    GDrawRequestExpose(bvtools,NULL,false);
	}
	if ( bvvisible[0])
	    BVLayersSet(bv);
	if ( bvvisible[2] && !bv->shades_hidden )
	    GDrawRequestExpose(bvtools,NULL,false);
    }
    if ( cvtools!=NULL ) {
	CharView *cv = GDrawGetUserData(cvtools);
	if ( cv!=NULL ) {
	    SaveOffsets(cv->gw,cvtools,&cvtoolsoff);
	    SaveOffsets(cv->gw,cvlayers,&cvlayersoff);
	    GDrawSetUserData(cvtools,NULL);
	    if ( cvlayers!=NULL )
		GDrawSetUserData(cvlayers,NULL);
	    if ( cvlayers2!=NULL )
		GDrawSetUserData(cvlayers2,NULL);
	}
	GDrawSetVisible(cvtools,false);
	if ( cvlayers!=NULL )
	    GDrawSetVisible(cvlayers,false);
	if ( cvlayers2!=NULL )
	    GDrawSetVisible(cvlayers2,false);
    }
}

void BVPalettesHideIfMine(BitmapView *bv) {
    if ( bvtools==NULL )
return;
    if ( GDrawGetUserData(bvtools)==bv ) {
	SaveOffsets(bv->gw,bvtools,&bvtoolsoff);
	SaveOffsets(bv->gw,bvlayers,&bvlayersoff);
	SaveOffsets(bv->gw,bvshades,&bvshadesoff);
	GDrawSetVisible(bvtools,false);
	GDrawSetVisible(bvlayers,false);
	GDrawSetVisible(bvshades,false);
	GDrawSetUserData(bvtools,NULL);
	GDrawSetUserData(bvlayers,NULL);
	GDrawSetUserData(bvshades,NULL);
    }
}

void CVPaletteDeactivate(void) {
    if ( cvtools!=NULL ) {
	CharView *cv = GDrawGetUserData(cvtools);
	if ( cv!=NULL ) {
	    SaveOffsets(cv->gw,cvtools,&cvtoolsoff);
	    GDrawSetUserData(cvtools,NULL);
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( cv->sc->parent->multilayer && cvlayers2!=NULL ) {
		SaveOffsets(cv->gw,cvlayers2,&cvlayersoff);
		GDrawSetUserData(cvlayers2,NULL);
	    } else
#endif
	    if ( cvlayers!=NULL ) {
		SaveOffsets(cv->gw,cvlayers,&cvlayersoff);
		GDrawSetUserData(cvlayers,NULL);
	    }
	}
	GDrawSetVisible(cvtools,false);
	if ( cvlayers!=NULL )
	    GDrawSetVisible(cvlayers,false);
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( cvlayers2!=NULL )
	    GDrawSetVisible(cvlayers2,false);
#endif
    }
    if ( bvtools!=NULL ) {
	BitmapView *bv = GDrawGetUserData(bvtools);
	if ( bv!=NULL ) {
	    SaveOffsets(bv->gw,bvtools,&bvtoolsoff);
	    SaveOffsets(bv->gw,bvlayers,&bvlayersoff);
	    SaveOffsets(bv->gw,bvshades,&bvshadesoff);
	    GDrawSetUserData(bvtools,NULL);
	    GDrawSetUserData(bvlayers,NULL);
	    GDrawSetUserData(bvshades,NULL);
	}
	GDrawSetVisible(bvtools,false);
	GDrawSetVisible(bvlayers,false);
	GDrawSetVisible(bvshades,false);
    }
}

void BVPaletteColorChange(BitmapView *bv) {
    if ( bvshades!=NULL )
	GDrawRequestExpose(bvshades,NULL,false);
    GDrawRequestExpose(bv->gw,NULL,false);
}

void BVPaletteColorUnderChange(BitmapView *bv,int color_under) {
    if ( bvshades!=NULL && color_under!=bv->color_under_cursor ) {
	bv->color_under_cursor = color_under;
	GDrawRequestExpose(bvshades,NULL,false);
    }
}

void BVPaletteChangedChar(BitmapView *bv) {
    if ( bvshades!=NULL && bvvisible[2]) {
	int hidden = bv->bdf->clut==NULL;
	if ( hidden!=bv->shades_hidden ) {
	    GDrawSetVisible(bvshades,!hidden);
	    bv->shades_hidden = hidden;
	    GDrawRequestExpose(bv->gw,NULL,false);
	} else
	    GDrawRequestExpose(bvshades,NULL,false);
    }
}

void PalettesChangeDocking(void) {

    palettes_docked = !palettes_docked;
    if ( palettes_docked ) {
	if ( cvtools!=NULL ) {
	    CharView *cv = GDrawGetUserData(cvtools);
	    if ( cv!=NULL ) {
		ReparentFixup(cvtools,cv->v,0,0,CV_TOOLS_WIDTH,CV_TOOLS_HEIGHT);
		if ( cvlayers!=NULL )
		    ReparentFixup(cvlayers,cv->v,0,CV_TOOLS_HEIGHT+2,0,0);
#ifdef FONTFORGE_CONFIG_TYPE3
		if ( cvlayers2!=NULL )
		    ReparentFixup(cvlayers2,cv->v,0,CV_TOOLS_HEIGHT+2,0,0);
#endif
	    }
	}
	if ( bvtools!=NULL ) {
	    BitmapView *bv = GDrawGetUserData(bvtools);
	    if ( bv!=NULL ) {
		ReparentFixup(bvtools,bv->v,0,0,BV_TOOLS_WIDTH,BV_TOOLS_HEIGHT);
		ReparentFixup(bvlayers,bv->v,0,BV_TOOLS_HEIGHT+2,0,0);
		ReparentFixup(bvshades,bv->v,0,BV_TOOLS_HEIGHT+BV_LAYERS_HEIGHT+4,0,0);
	    }
	}
    } else {
	if ( cvtools!=NULL ) {
	    GDrawReparentWindow(cvtools,GDrawGetRoot(NULL),0,0);
	    if ( cvlayers!=NULL )
		GDrawReparentWindow(cvlayers,GDrawGetRoot(NULL),0,CV_TOOLS_HEIGHT+2+45);
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( cvlayers2!=NULL )
		GDrawReparentWindow(cvlayers2,GDrawGetRoot(NULL),0,CV_TOOLS_HEIGHT+2+45);
#endif
	}
	if ( bvtools!=NULL ) {
	    GDrawReparentWindow(bvtools,GDrawGetRoot(NULL),0,0);
	    GDrawReparentWindow(bvlayers,GDrawGetRoot(NULL),0,BV_TOOLS_HEIGHT+2+45);
	    GDrawReparentWindow(bvshades,GDrawGetRoot(NULL),0,BV_TOOLS_HEIGHT+BV_LAYERS_HEIGHT+4+90);
	}
    }
    SavePrefs();
}

int BVPalettesWidth(void) {
return( GGadgetScale(BV_LAYERS_WIDTH));
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
