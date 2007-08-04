/* Copyright (C) 2007 by George Williams */
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
#include <stddef.h>
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>

#ifdef FONTFORGE_CONFIG_DEVICETABLES
#define MCD(ui_name,name,msg,np) { ui_name, #name, offsetof(struct MATH,name), -1,msg,np }
#define MCDD(ui_name,name,devtab_name,msg,np) { ui_name, #name, offsetof(struct MATH,name), offsetof(struct MATH,devtab_name),msg,np }
#else
#define MCD(ui_name,name,msg,np) { ui_name, #name, offsetof(struct MATH,name), -1,msg,np }
#define MCDD(ui_name,name,devtab_name,msg,np) { ui_name, #name, offsetof(struct MATH,name), -2,msg,np }
#endif

struct math_constants_descriptor math_constants_descriptor[] = {
    MCD(N_("ScriptPercentScaleDown:"),ScriptPercentScaleDown,N_("Percentage scale down for script level 1"),0),
    MCD(N_("ScriptScriptPercentScaleDown:"),ScriptScriptPercentScaleDown,N_("Percentage scale down for script level 2"),0),
    MCD(N_("DelimitedSubFormulaMinHeight:"),DelimitedSubFormulaMinHeight,N_("Minimum height at which to treat a delimited\nexpression as a subformula"),0),
    MCD(N_("DisplayOperatorMinHeight:"),DisplayOperatorMinHeight,N_("Minimum height of n-ary operators (integration, summation, etc.)"),0),
    MCDD(N_("MathLeading:"),MathLeading,MathLeading_adjust,N_("White space to be left between math formulae\nto ensure proper line spacing."),0),
    MCDD(N_("AxisHeight:"),AxisHeight,AxisHeight_adjust,N_("Axis height of the font"),0),
    MCDD(N_("AccentBaseHeight:"),AccentBaseHeight,AccentBaseHeight_adjust,N_("Maximum (ink) height of accent base that\ndoes not require raising the accents."),0),
    MCDD(N_("FlattenedAccentBaseHeight:"),FlattenedAccentBaseHeight,FlattenedAccentBaseHeight_adjust,N_("Maximum (ink) height of accent base that\ndoes not require flattening the accents."),0),
    MCDD(N_("SubscriptShiftDown:"),SubscriptShiftDown,SubscriptShiftDown_adjust,N_("The standard shift down applied to subscript elements.\nPositive for moving downward."),1),
    MCDD(N_("SubscriptTopMax:"),SubscriptTopMax,SubscriptTopMax_adjust,N_("Maximum height of the (ink) top of subscripts\nthat does not require moving\nubscripts further down."),0),
    MCDD(N_("SubscriptBaselineDropMin:"),SubscriptBaselineDropMin,SubscriptBaselineDropMin_adjust,N_("Maximum allowed drop of the baseline of\nsubscripts realtive to the bottom of the base.\nUsed for bases that are treated as a box\nor extended shape. Positive for subscript\nbaseline dropped below base bottom."),0),
    MCDD(N_("SuperscriptShiftUp:"),SuperscriptShiftUp,SuperscriptShiftUp_adjust,N_("Standard shift up applied to superscript elements."),0),
    MCDD(N_("SuperscriptShiftUpCramped:"),SuperscriptShiftUpCramped,SuperscriptShiftUpCramped_adjust,N_("Standard shift of superscript relative\nto base in cramped mode."),0),
    MCDD(N_("SuperscriptBottomMin:"),SuperscriptBottomMin,SuperscriptBottomMin_adjust,N_("Minimum allowed hieght of the bottom\nof superscripts that does not require moving\nthem further up."),0),
    MCDD(N_("SuperscriptBaselineDropMax:"),SubscriptBaselineDropMin,SubscriptBaselineDropMin_adjust,N_("Maximum allowed drop of the baseline of\nsuperscripts realtive to the top of the base.\nUsed for bases that are treated as a box\nor extended shape. Positive for superscript\nbaseline below base top."),0),
    MCDD(N_("SubSuperscriptGapMin:"),SubSuperscriptGapMin,SubSuperscriptGapMin_adjust,N_("Minimum gap between the supersecript and subscript ink."),0),
    MCDD(N_("SuperscriptBottomMaxWithSubscript:"),SuperscriptBottomMaxWithSubscript,SuperscriptBottomMaxWithSubscript_adjust,N_("The maximum level to which the (ink) bottom\nof superscript can be pushed to increase the\ngap between superscript and subscript, before\nsubscript starts being moved down."),0),
    MCDD(N_("SpaceAfterScript:"),SpaceAfterScript,SpaceAfterScript_adjust,N_("Extra white space to be added after each\nub/superscript."),0),
    MCDD(N_("UpperLimitGapMin:"),UpperLimitGapMin,UpperLimitGapMin_adjust,N_("Minimum gap between the bottom of the\nupper limit, and the top fo the base operator."),1),
    MCDD(N_("UpperLimitBaselineRiseMin:"),UpperLimitBaselineRiseMin,UpperLimitBaselineRiseMin_adjust,N_("Minimum distance between the baseline of an upper\nlimit and the bottom of the base operator."),0),
    MCDD(N_("LowerLimitGapMin:"),LowerLimitGapMin,LowerLimitGapMin_adjust,N_("Standard shift up applied to the top element of\na stack."),0),
    MCDD(N_("LowerLimitBaselineDropMin:"),LowerLimitBaselineDropMin,LowerLimitBaselineDropMin_adjust,N_("Minimum distance between the baseline of the\nlower limit and bottom of the base operator."),0),
    MCDD(N_("StackTopShiftUp:"),StackTopShiftUp,StackTopShiftUp_adjust,N_("Standard shift up applied to the top element of a stack."),1),
    MCDD(N_("StackTopDisplayStyleShiftUp:"),StackTopDisplayStyleShiftUp,StackTopDisplayStyleShiftUp_adjust,N_("Standard shift up applied to the top element of\na stack in display style."),0),
    MCDD(N_("StackBottomShiftDown:"),StackBottomShiftDown,StackBottomShiftDown_adjust,N_("Standard shift down applied to the bottom element of a stack.\nPositive values indicate downward motion."),0),
    MCDD(N_("StackBottomDisplayStyleShiftDown:"),StackBottomDisplayStyleShiftDown,StackBottomDisplayStyleShiftDown_adjust,N_("Standard shift down applied to the bottom\nelement of a stack in display style.\nPositive values indicate downward motion."),0),
    MCDD(N_("StackGapMin:"),StackGapMin,StackGapMin_adjust,N_("Minimum gap between bottom of the top\nelement of a stack, and the top of the bottom element."),0),
    MCDD(N_("StackDisplayStyleGapMin:"),StackDisplayStyleGapMin,StackDisplayStyleGapMin_adjust,N_("Minimum gap between bottom of the top\nelement of a stack and the top of the bottom\nelement in display style."),0),
    MCDD(N_("StretchStackTopShiftUp:"),StretchStackTopShiftUp,StretchStackTopShiftUp_adjust,N_("Standard shift up applied to the top element of the stretch stack."),0),
    MCDD(N_("StretchStackBottomShiftDown:"),StretchStackBottomShiftDown,StretchStackBottomShiftDown_adjust,N_("Standard shift down applied to the bottom\nelement of the stretch stack.\nPositive values indicate downward motion."),0),
    MCDD(N_("StretchStackGapAboveMin:"),StretchStackGapAboveMin,StretchStackGapAboveMin_adjust,N_("Minimum gap between the ink of the stretched\nelement and the ink bottom of the element\nabove.."),0),
    MCDD(N_("StretchStackGapBelowMin:"),StretchStackGapBelowMin,StretchStackGapBelowMin_adjust,N_("Minimum gap between the ink of the stretched\nelement and the ink top of the element below."),0),
    MCDD(N_("FractionNumeratorShiftUp:"),FractionNumeratorShiftUp,FractionNumeratorShiftUp_adjust,N_("Standard shift up applied to the numerator."),1),
    MCDD(N_("FractionNumeratorDisplayStyleShiftUp:"),FractionNumeratorDisplayStyleShiftUp,FractionNumeratorDisplayStyleShiftUp_adjust,N_("Standard shift up applied to the\nnumerator in display style."),0),
    MCDD(N_("FractionDenominatorShiftDown:"),FractionDenominatorShiftDown,FractionDenominatorShiftDown_adjust,N_("Standard shift down applied to the denominator.\nPostive values indicate downward motion."),0),
    MCDD(N_("FractionDenominatorDisplayStyleShiftDown:"),FractionDenominatorDisplayStyleShiftDown,FractionDenominatorDisplayStyleShiftDown_adjust,N_("Standard shift down applied to the\ndenominator in display style.\nPostive values indicate downward motion."),0),
    MCDD(N_("FractionNumeratorGapMin:"),FractionNumeratorGapMin,FractionNumeratorGapMin_adjust,N_("Minimum tolerated gap between the ink\nbottom of the numerator and the ink of the fraction bar."),0),
    MCDD(N_("FractionNumeratorDisplayStyleGapMin:"),FractionNumeratorDisplayStyleGapMin,FractionNumeratorDisplayStyleGapMin_adjust,N_("Minimum tolerated gap between the ink\nbottom of the numerator and the ink of the fraction\nbar in display style."),0),
    MCDD(N_("FractionRuleThickness:"),FractionRuleThickness,FractionRuleThickness_adjust,N_("Thickness of the fraction bar."),0),
    MCDD(N_("FractionDenominatorGapMin:"),FractionDenominatorGapMin,FractionDenominatorGapMin_adjust,N_("Minimum tolerated gap between the ink top of the denominator\nand the ink of the fraction bar.."),0),
    MCDD(N_("FractionDenominatorDisplayStyleGapMin:"),FractionDenominatorDisplayStyleGapMin,FractionDenominatorDisplayStyleGapMin_adjust,N_("Minimum tolerated gap between the ink top of the denominator\nand the ink of the fraction bar in display style."),0),
    MCDD(N_("SkewedFractionHorizontalGap:"),SkewedFractionHorizontalGap,SkewedFractionHorizontalGap_adjust,N_("Horizontal distance between the top\nand bottom elemnts of a skewed fraction."),0),
    MCDD(N_("SkewedFractionVerticalGap:"),SkewedFractionVerticalGap,SkewedFractionVerticalGap_adjust,N_("Vertical distance between the ink of the top and\nbottom elements of a skewed fraction."),0),
    MCDD(N_("OverbarVerticalGap:"),OverbarVerticalGap,OverbarVerticalGap_adjust,N_("Distance between the overbar and\nthe ink top of the base."),1),
    MCDD(N_("OverbarRuleThickness:"),OverbarRuleThickness,OverbarRuleThickness_adjust,N_("Thickness of the overbar."),0),
    MCDD(N_("OverbarExtraAscender:"),OverbarExtraAscender,OverbarExtraAscender_adjust,N_("Extra white space reserved above the overbar."),0),
    MCDD(N_("UnderbarVerticalGap:"),UnderbarVerticalGap,UnderbarVerticalGap_adjust,N_("Distance between underbar and\nthe (ink) bottom of the base."),0),
    MCDD(N_("UnderbarRuleThickness:"),UnderbarRuleThickness,UnderbarRuleThickness_adjust,N_("Thickness of the underbar."),0),
    MCDD(N_("UnderbarExtraDescender:"),UnderbarExtraDescender,UnderbarExtraDescender_adjust,N_("Extra white space resevered below the underbar."),0),
    MCDD(N_("RadicalVerticalGap:"),RadicalVerticalGap,RadicalVerticalGap_adjust,N_("Space between the ink to of the\nexpression and the bar over it."),1),
    MCDD(N_("RadicalDisplayStyleVerticalGap:"),RadicalDisplayStyleVerticalGap,RadicalDisplayStyleVerticalGap_adjust,N_("Space between the ink top of the\nexpression and the bar over it in display\nstyle."),0),
    MCDD(N_("RadicalRuleThickness:"),RadicalRuleThickness,RadicalRuleThickness_adjust,N_("Thickness of the radical rule in\ndesigned or constructed radical\nsigns."),0),
    MCDD(N_("RadicalExtraAscender:"),RadicalExtraAscender,RadicalExtraAscender_adjust,N_("Extra white space reserved above the radical."),0),
    MCDD(N_("RadicalKernBeforeDegree:"),RadicalKernBeforeDegree,RadicalKernBeforeDegree_adjust,N_("Extra horizontal kern before the degree of a\nradical if such be present."),0),
    MCDD(N_("RadicalKernAfterDegree:"),RadicalKernAfterDegree,RadicalKernAfterDegree_adjust,N_("Negative horizontal kern after the degree of a\nradical if such be present."),0),
    MCD(N_("RadicalDegreeBottomRaisePercent:"),RadicalDegreeBottomRaisePercent,N_("Height of the bottom of the radical degree, if\nsuch be present, in proportion to the ascender\nof the radical sign."),0),
    MCD(N_("MinConnectorOverlap:"),MinConnectorOverlap,N_("Minimum overlap of connecting glyphs during\nglyph construction."),1),
    { NULL }
};

static char *aspectnames[] = {
    N_("Constants"),
    N_("Sub/Superscript"),
    N_("Limits"),
    N_("Stacks"),
    N_("Fractions"),
    N_("Over/Underbars"),
    N_("Radicals"),
    N_("Connectors"),
    NULL
};

static char *GlyphConstruction_Dlg(GGadget *g, int r, int c);
static char *MKChange_Dlg(GGadget *g, int r, int c);
static void extpart_finishedit(GGadget *g, int r, int c, int wasnew);
static void italic_finishedit(GGadget *g, int r, int c, int wasnew);
static void topaccent_finishedit(GGadget *g, int r, int c, int wasnew);
static void mathkern_finishedit(GGadget *g, int r, int c, int wasnew);

static GTextInfo truefalse[] = {
    { (unichar_t *) N_("false"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("true"),  NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    NULL };

static struct col_init exten_shape_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_enum, NULL, truefalse, NULL, N_("Is Extension Shape") },
    0
    };

static struct col_init italic_cor_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_int, NULL, NULL, NULL, N_("Italic Correction") },
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
#endif
    0
    };

static struct col_init top_accent_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_int, NULL, NULL, NULL, N_("Top Accent Horiz. Pos") },
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
#endif
    0
    };

static struct col_init glyph_variants_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_string, NULL, NULL, NULL, N_("Pre-Built Larger Variants") },
    0
    };

static struct col_init glyph_construction_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
/* GT: Italic correction */
    { me_int, NULL, NULL, NULL, N_("I.C.") },
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
#endif
    { me_funcedit, GlyphConstruction_Dlg, NULL, NULL, N_("Parts List") },
    0
    };

static struct col_init math_kern_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_button, MKChange_Dlg, NULL, NULL, N_("Height/Kern Data") },
    0
    };

struct matrixinit mis[] = {
    { sizeof(exten_shape_ci)/sizeof(struct col_init)-1, exten_shape_ci, 0, NULL, NULL, NULL, NULL },
    { sizeof(italic_cor_ci)/sizeof(struct col_init)-1, italic_cor_ci, 0, NULL, NULL, NULL, italic_finishedit },
    { sizeof(top_accent_ci)/sizeof(struct col_init)-1, top_accent_ci, 0, NULL, NULL, NULL, topaccent_finishedit },
    { sizeof(math_kern_ci)/sizeof(struct col_init)-1, math_kern_ci, 0, NULL, NULL, NULL, mathkern_finishedit },
    { sizeof(glyph_variants_ci)/sizeof(struct col_init)-1, glyph_variants_ci, 0, NULL, NULL, NULL, NULL },
    { sizeof(glyph_construction_ci)/sizeof(struct col_init)-1, glyph_construction_ci, 0, NULL, NULL, NULL, NULL },
    { sizeof(glyph_variants_ci)/sizeof(struct col_init)-1, glyph_variants_ci, 0, NULL, NULL, NULL, NULL },
    { sizeof(glyph_construction_ci)/sizeof(struct col_init)-1, glyph_construction_ci, 0, NULL, NULL, NULL, NULL },
    { 0 }
};

static struct col_init extensionpart[] = {
    { me_string , NULL, NULL, NULL, N_("Glyph") },
    { me_enum, NULL, truefalse, NULL, N_("Extender") },
/* GT: "Len" is an abreviation for "Length" */
    { me_int, NULL, NULL, NULL, N_("StartLen") },
    { me_int, NULL, NULL, NULL, N_("EndLen") },
    { me_int, NULL, NULL, NULL, N_("FullLen") },
    0
    };
static struct matrixinit mi_extensionpart =
    { sizeof(extensionpart)/sizeof(struct col_init)-1, extensionpart, 0, NULL, NULL, NULL, extpart_finishedit };


#define CID_Exten	1000
#define CID_Italic	1001
#define CID_TopAccent	1002
#define CID_MathKern	1003
#define CID_VGlyphVar	1004
#define CID_VGlyphConst	1005
#define CID_HGlyphVar	1006
#define CID_HGlyphConst	1007

static char *gi_aspectnames[] = {
    N_("Exten Shapes"),
    N_("Italic Correction"),
    N_("Top Accent"),
    N_("Math Kern"),
    N_("Vert. Variants"),
    N_("Vert. Construction"),
    N_("Hor. Variants"),
    N_("Hor. Construction"),
    NULL
};

static char *cornernames[] = {
    N_("Top Right"),
    N_("Top Left"),
    N_("Bottom Right"),
    N_("Bottom Right"),
    NULL
};


void MathInit(void) {
    int i, j;
    static int inited = false;
    static struct col_init *ci[] = { exten_shape_ci, italic_cor_ci,
	    top_accent_ci, glyph_variants_ci, glyph_construction_ci,
	    extensionpart, math_kern_ci, NULL };
    static GTextInfo *tis[] = { truefalse, NULL };
    static char **chars[] = { aspectnames, gi_aspectnames, cornernames, NULL };

    if ( inited )
return;

    for ( j=0; chars[j]!=NULL; ++j )
	for ( i=0; chars[j][i]!=NULL; ++i )
	    chars[j][i] = _(chars[j][i]);
    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i )
	math_constants_descriptor[i].ui_name=_(math_constants_descriptor[i].ui_name);
    for ( j=0; tis[j]!=NULL; ++j )
	for ( i=0; tis[j][i].text!=NULL; ++i )
	    tis[j][i].text = (unichar_t *) _((char *) tis[j][i].text);
    for ( j=0; ci[j]!=NULL; ++j )
	for ( i=0; ci[j][i].title!=NULL; ++i )
	    ci[j][i].title = _(ci[j][i].title);
	    
    inited = true;
}

struct MATH *MathTableNew(SplineFont *sf) {
    struct MATH *math = gcalloc(1,sizeof(struct MATH));	/* Too big for chunkalloc */
    int emsize = sf->ascent+sf->descent;
    DBounds b;
    SplineChar *sc;

    math->ScriptPercentScaleDown	= 80;
    math->ScriptScriptPercentScaleDown	= 60;
    math->DelimitedSubFormulaMinHeight	= emsize*1.5;
    /* No default given for math->DisplayOperatorMinHeight */
    /* No default given for math->AxisHeight */
    sc = SFGetChar(sf,'x',NULL);
    if ( sc!=NULL ) {
	SplineCharFindBounds(sc,&b);
	math->AccentBaseHeight = b.maxy;
    }
    sc = SFGetChar(sf,'I',NULL);
    if ( sc!=NULL ) {
	SplineCharFindBounds(sc,&b);
	math->FlattenedAccentBaseHeight = b.maxy;
    }
    if ( sf->pfminfo.subsuper_set )
	math->SubscriptShiftDown = sf->pfminfo.os2_subyoff;
    math->SubscriptTopMax = math->AccentBaseHeight;		/* X-height */
    /* No default given for math->SubscriptBaselineDropMin */
    if ( sf->pfminfo.subsuper_set )
	math->SuperscriptShiftUp = sf->pfminfo.os2_supyoff;
    /* No default given for math->SuperscriptShiftUpCramped */
    math->SuperscriptBottomMin = math->AccentBaseHeight;	/* X-height */
    /* No default given for math->SuperscriptBaselineDropMax */
    math->SubSuperscriptGapMin = 4*sf->uwidth;			/* 4* default rule thickness */
    math->SuperscriptBottomMaxWithSubscript = math->AccentBaseHeight;	/* X-height */
    math->SpaceAfterScript = emsize/24;				/* .5pt at 12pt */
    math->StackGapMin = 3*sf->uwidth;				/* 3* default rule thickness */
    math->StackDisplayStyleGapMin = 7*sf->uwidth;
    math->StretchStackGapAboveMin = math->UpperLimitGapMin;
    math->StretchStackGapBelowMin = math->LowerLimitGapMin;
    math->FractionNumeratorDisplayStyleShiftUp = math->StackTopDisplayStyleShiftUp;
    math->FractionDenominatorDisplayStyleShiftDown = math->StackBottomDisplayStyleShiftDown;
    math->FractionNumeratorGapMin = sf->uwidth;
    math->FractionNumeratorDisplayStyleGapMin = 3*sf->uwidth;
    math->FractionRuleThickness = sf->uwidth;
    math->FractionDenominatorGapMin = sf->uwidth;
    math->FractionDenominatorDisplayStyleGapMin = 3*sf->uwidth;
    math->OverbarVerticalGap = 3*sf->uwidth;
    math->OverbarRuleThickness = sf->uwidth;
    math->OverbarExtraAscender = sf->uwidth;
    math->UnderbarVerticalGap = 3*sf->uwidth;
    math->UnderbarRuleThickness = sf->uwidth;
    math->UnderbarExtraDescender = sf->uwidth;
    math->RadicalVerticalGap = sf->uwidth;
    math->RadicalExtraAscender = sf->uwidth;
    math->RadicalKernBeforeDegree = 5*emsize/18;
    math->RadicalKernAfterDegree = -10*emsize/18;
    math->RadicalDegreeBottomRaisePercent = 60;

    math->MinConnectorOverlap = emsize/50;
return( math );
}

void MATHFree(struct MATH *math) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    int i;

    if ( math==NULL )
return;

    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	if ( math_constants_descriptor[i].devtab_offset>=0 )
	    DeviceTableFree( *(DeviceTable **) (((char *) math) + math_constants_descriptor[i].devtab_offset ) );
    }
#else
    if ( math==NULL )
return;
#endif
    free(math);
}

static int GV_StringCheck(SplineFont *sf,char *str) {
    char *start, *pt;
    int scnt, pcnt, ch;
    SplineChar *sc;

    pcnt = 0;
    for ( start = str ; ; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
return( pcnt );
	for ( pt=start; *pt!=':' && *pt!=' ' && *pt!='\0' ; ++pt );
	ch = *pt;
	if ( ch==' ' || ch=='\0' )
return( -1 );
	if ( sf!=NULL ) {
	    *pt = '\0';
	    sc = SFGetChar(sf,-1,start);
	    *pt = ch;
	    if ( sc==NULL )
return( -1 );
	}
	scnt = 0;
	while ( *pt!=' ' && *pt!='\0' ) {
	    if ( *pt==':' ) ++scnt;
	    else if ( !isdigit( *pt ))
return( -1 );
	    ++pt;
	}
	if ( scnt!=4 )
return( -1 );
	++pcnt;
	start = pt;
    }
}

static struct glyphvariants *GV_FromString(struct glyphvariants *gv,char *str) {
    int pcnt = GV_StringCheck(NULL,str);
    char *start, *pt;
    int ch, temp;

    if ( pcnt<=0 )
return( gv );
    if ( gv==NULL )
	gv = chunkalloc(sizeof(struct glyphvariants));
    gv->part_cnt = pcnt;
    gv->parts = gcalloc(pcnt,sizeof(struct gv_part));
    pcnt = 0;
    for ( start = str ; ; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
return( gv );
	for ( pt=start; *pt!=':' ; ++pt );
	ch = *pt; *pt = '\0';
	gv->parts[pcnt].component = copy(start);
	*pt = ch;
	sscanf(pt,":%d:%hd:%hd:%hd", &temp,
		&gv->parts[pcnt].startConnectorLength,
		&gv->parts[pcnt].endConnectorLength,
		&gv->parts[pcnt].fullAdvance);
	gv->parts[pcnt].is_extender = temp;
	while ( *pt!=' ' && *pt!='\0' ) ++pt;
	++pcnt;
	start = pt;
    }
}

static char *GV_ToString(struct glyphvariants *gv) {
    int i, len;
    char buffer[80], *str;

    if ( gv==NULL || gv->part_cnt==0 )
return( NULL );
    for ( i=len=0; i<gv->part_cnt; ++i ) {
	len += strlen(gv->parts[i].component);
	sprintf( buffer, ":%d:%d:%d:%d ", gv->parts[i].is_extender,
		gv->parts[i].startConnectorLength,
		gv->parts[i].endConnectorLength,
		gv->parts[i].fullAdvance);
	len += strlen( buffer );
    }
    str = galloc(len+1);
    for ( i=len=0; i<gv->part_cnt; ++i ) {
	strcpy(str+len,gv->parts[i].component);
	len += strlen(gv->parts[i].component);
	sprintf( buffer, ":%d:%d:%d:%d ", gv->parts[i].is_extender,
		gv->parts[i].startConnectorLength,
		gv->parts[i].endConnectorLength,
		gv->parts[i].fullAdvance);
	strcpy(str+len,buffer);
	len += strlen( buffer );
    }
    if ( len!=0 )
	str[len-1] = '\0';
    else
	*str = '\0';
return( str );
}

static int SF_NameListCheck(SplineFont *sf,char *list) {
    char *start, *pt;
    int ch;
    SplineChar *sc;

    if ( list==NULL )
return( true );

    for ( start = list ; ; ) {
	while ( *start== ' ' ) ++start;
	if ( *start=='\0' )
return( true );
	for ( pt=start ; *pt!=' ' && *pt!='\0'; ++pt );
	ch = *pt; *pt = '\0';
	sc = SFGetChar(sf,-1,start);
	*pt = ch;
	start = pt;
	if ( sc==NULL )
return( false );
    }
}

typedef struct mathdlg {
    GWindow gw;
    SplineFont *sf;
    struct MATH *math;
    uint8 done;
    uint8 ok;
    uint16 popup_r;
    GGadget *popup_g;
    /* Used by glyphconstruction_dlg */
    SplineChar *sc;
    int is_horiz;
} MathDlg;

static unichar_t **MATH_GlyphNameCompletion(GGadget *t,int from_tab) {
    MathDlg *math = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = math->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,false));
}

static unichar_t **MATH_GlyphListCompletion(GGadget *t,int from_tab) {
    MathDlg *math = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = math->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,true));
}

static void MATH_Init(MathDlg *math) {
    int i, cnt, ccnt, ta, h;
    char buffer[20];
    GGadget *g;
    struct matrix_data *mds;
    SplineFont *sf = math->sf;
    SplineChar *sc;
    int cols;
    char *change = _("Change");

    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	GGadget *tf = GWidgetGetControl(math->gw,2*i+1);
	int16 *pos = (int16 *) (((char *) (math->math)) + math_constants_descriptor[i].offset );

	sprintf( buffer, "%d", *pos );
	GGadgetSetTitle8(tf,buffer);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
	    GGadget *tf2 = GWidgetGetControl(math->gw,2*i+2);
	    DeviceTable **devtab = (DeviceTable **) (((char *) (math->math)) + math_constants_descriptor[i].devtab_offset );
	    char *str;

	    DevTabToString(&str,*devtab);
	    if ( str!=NULL )
		GGadgetSetTitle8(tf2,str);
	    free(str);
	}
#endif
    }

    /* Extension Shapes */
    for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL )
	if ( sc->is_extended_shape )
	    ++cnt;
    mds = gcalloc(cnt*2,sizeof(struct matrix_data));
    for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL )
	if ( sc->is_extended_shape ) {
	    mds[2*cnt+0].u.md_str = copy(sc->name);
	    mds[2*cnt+1].u.md_ival = true;
	    ++cnt;
	}
    GMatrixEditSet(GWidgetGetControl(math->gw,CID_Exten), mds,cnt,false);

    /* Italic Correction && Top Angle Horizontal Position */
    for ( ta=0; ta<2; ++ta ) {
	g = GWidgetGetControl(math->gw,ta?CID_TopAccent:CID_Italic );
	cols = GMatrixEditGetColCnt(g);
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    if ( (ta==0 && sc->italic_correction!=TEX_UNDEF ) ||
		    (ta==1 && sc->top_accent_horiz!=TEX_UNDEF))
		++cnt;
	}
	mds = gcalloc(cnt*cols,sizeof(struct matrix_data));
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    if ( ta==0 && sc->italic_correction!=TEX_UNDEF ) {
		mds[cols*cnt+0].u.md_str = copy(sc->name);
		mds[cols*cnt+1].u.md_ival = sc->italic_correction;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		DevTabToString(&mds[cols*cnt+2].u.md_str,sc->italic_adjusts);
#endif
		++cnt;
	    } else if ( ta==1 &&sc->top_accent_horiz!=TEX_UNDEF ) {
		mds[cols*cnt+0].u.md_str = copy(sc->name);
		mds[cols*cnt+1].u.md_ival = sc->top_accent_horiz;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		DevTabToString(&mds[cols*cnt+2].u.md_str,sc->top_accent_adjusts);
#endif
		++cnt;
	    }
	}
	GMatrixEditSet(g, mds,cnt,false);
    }

    /* Math Kern */
    g = GWidgetGetControl(math->gw,CID_MathKern);
    cols = GMatrixEditGetColCnt(g);
    for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL )
	if ( sc->mathkern!=NULL )
	    ++cnt;
    mds = gcalloc(cnt*cols,sizeof(struct matrix_data));
    for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL )
	if ( sc->mathkern!=NULL ) {
	    mds[cols*cnt+0].u.md_str = copy(sc->name);
	    mds[cols*cnt+1].u.md_str = copy(change);
	    ++cnt;
	}
    GMatrixEditSet(g, mds,cnt,false);

    /* Horizontal/Vertical Glyph Variants */
    for ( h=0; h<2; ++h ) {
	g = GWidgetGetControl(math->gw,CID_VGlyphVar+2*h);
	cols = GMatrixEditGetColCnt(g);
	for ( i=cnt=ccnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    struct glyphvariants *gv = h ? sc->horiz_variants : sc->vert_variants;
	    if ( gv!=NULL && gv->variants!=NULL )
		++cnt;
	    if ( gv!=NULL && gv->part_cnt!=0 )
		++ccnt;
	}
	mds = gcalloc(cnt*cols,sizeof(struct matrix_data));
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    struct glyphvariants *gv = h ? sc->horiz_variants : sc->vert_variants;
	    if ( gv!=NULL && gv->variants!=NULL ) {
		mds[cols*cnt+0].u.md_str = copy(sc->name);
		mds[cols*cnt+1].u.md_str = copy(gv->variants);
		++cnt;
	    }
	}
	GMatrixEditSet(g, mds,cnt,false);

	/* Glyph Construction */
	g = GWidgetGetControl(math->gw,CID_VGlyphConst+2*h);
	cols = GMatrixEditGetColCnt(g);
	mds = gcalloc(ccnt*cols,sizeof(struct matrix_data));
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    struct glyphvariants *gv = h ? sc->horiz_variants : sc->vert_variants;
	    if ( gv!=NULL && gv->part_cnt!=0 ) {
		mds[cols*cnt+0].u.md_str = copy(sc->name);
		mds[cols*cnt+1].u.md_ival = gv->italic_correction;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		DevTabToString(&mds[cols*cnt+2].u.md_str,gv->italic_adjusts);
#endif
		mds[cols*cnt+cols-1].u.md_str = GV_ToString(gv);
		++cnt;
	    }
	}
	GMatrixEditSet(g, mds,cnt,false);
    }
}

static void MATH_FreeImage(const void *_math, GImage *img) {
    GImageDestroy(img);
}

static GImage *_MATHVar_GetImage(const void *_math) {
    MathDlg *math = (MathDlg *) _math;
    GGadget *varlist = math->popup_g;
    int rows, cols = GMatrixEditGetColCnt(varlist);
    struct matrix_data *old = GMatrixEditGet(varlist,&rows);
    SplineChar *sc = SFGetChar(math->sf,-1, old[cols*math->popup_r].u.md_str);
    static OTLookup dummyl = { NULL, gsub_multiple };
    static struct lookup_subtable dummys = { NULL, NULL, &dummyl };

return( PST_GetImage(varlist,math->sf,&dummys,math->popup_r,sc) );
}

static void MATHVar_PopupPrepare(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);

    if ( c<0 || c>=cols || r<0 || r>=rows || old[cols*r].u.md_str==NULL ||
	SFGetChar(math->sf,-1, old[cols*r+0].u.md_str)==NULL )
return;
    math->popup_r = r;
    math->popup_g = g;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,math,_MATHVar_GetImage,MATH_FreeImage);
}

static GImage *_MATHConst_GetImage(const void *_math) {
    MathDlg *math = (MathDlg *) _math;
    GGadget *varlist = math->popup_g;
    int rows, cols = GMatrixEditGetColCnt(varlist);
    struct matrix_data *old = GMatrixEditGet(varlist,&rows);
    SplineChar *sc = SFGetChar(math->sf,-1, old[cols*math->popup_r].u.md_str);
    struct glyphvariants *gv = GV_FromString(NULL,old[cols*math->popup_r+cols-1].u.md_str);
    GImage *ret;

    ret = GV_GetConstructedImage(sc,gv,GGadgetGetCid(varlist)==CID_HGlyphConst);
    GlyphVariantsFree(gv);
return( ret );
}

static void MATHConst_PopupPrepare(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);

    if ( c<0 || c>=cols || r<0 || r>=rows || old[cols*r].u.md_str==NULL ||
	SFGetChar(math->sf,-1, old[cols*r+0].u.md_str)==NULL )
return;
    math->popup_r = r;
    math->popup_g = g;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,math,_MATHConst_GetImage,MATH_FreeImage);
}

static GImage *_MATHLine_GetImage(const void *_math) {
    MathDlg *math = (MathDlg *) _math;
    GGadget *varlist = math->popup_g;
    int rows, cols = GMatrixEditGetColCnt(varlist);
    struct matrix_data *old = GMatrixEditGet(varlist,&rows);
    SplineChar *sc = SFGetChar(math->sf,-1, old[cols*math->popup_r].u.md_str);

return( SC_GetLinedImage(sc,old[cols*math->popup_r+1].u.md_ival,GGadgetGetCid(varlist)==CID_Italic));
}

static void MATHLine_PopupPrepare(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);

    if ( c<0 || c>=cols || r<0 || r>=rows || old[cols*r].u.md_str==NULL ||
	SFGetChar(math->sf,-1, old[cols*r+0].u.md_str)==NULL )
return;
    math->popup_r = r;
    math->popup_g = g;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,math,_MATHLine_GetImage,MATH_FreeImage);
}

static GImage *_GVC_GetImage(const void *_math) {
    MathDlg *math = (MathDlg *) _math;
    GGadget *varlist = math->popup_g;
    int rows, cols = GMatrixEditGetColCnt(varlist);
    struct matrix_data *old = GMatrixEditGet(varlist,&rows);
    GImage *ret;
    struct glyphvariants *gv;

    gv = GV_ParseConstruction(NULL,old,rows,cols);
    ret = GV_GetConstructedImage(math->sc,gv,math->is_horiz);
    GlyphVariantsFree(gv);
return( ret );
}

static void italic_finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *stuff;
    MathDlg *math;
    int cols;
    DBounds b;
    SplineChar *sc;

    if ( c!=0 )
return;
    if ( !wasnew )
return;
    /* If they added a new glyph to the sequence then set some defaults for it. */
    /*  only the full advance has any likelyhood of being correct */
    math = GDrawGetUserData(GGadgetGetWindow(g));
    stuff = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( stuff[r*cols+0].u.md_str==NULL )
return;
    sc = SFGetChar(math->sf,-1,stuff[r*cols+0].u.md_str);
    if ( sc==NULL )
return;
    SplineCharFindBounds(sc,&b);
    if ( b.maxx>sc->width ) {
	stuff[r*cols+1].u.md_ival = rint((b.maxx-sc->width) +
			(math->sf->ascent+math->sf->descent)/16.0);
	GGadgetRedraw(g);
    }
}

static void topaccent_finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *stuff;
    MathDlg *math;
    int cols;
    DBounds b;
    SplineChar *sc;
    double italic_off;

    if ( c!=0 )
return;
    if ( !wasnew )
return;
    /* If they added a new glyph to the sequence then set some defaults for it. */
    /*  only the full advance has any likelyhood of being correct */
    math = GDrawGetUserData(GGadgetGetWindow(g));
    stuff = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( stuff[r*cols+0].u.md_str==NULL )
return;
    sc = SFGetChar(math->sf,-1,stuff[r*cols+0].u.md_str);
    if ( sc==NULL )
return;
    SplineCharFindBounds(sc,&b);
    italic_off = (b.maxy-b.miny)*tan(-math->sf->italicangle);
    if ( b.maxx-b.minx-italic_off < 0 )
	stuff[r*cols+1].u.md_ival = rint(b.minx + (b.maxx-b.minx)/2);
    else
	stuff[r*cols+1].u.md_ival = rint(b.minx + italic_off + (b.maxx - b.minx - italic_off)/2);
    GGadgetRedraw(g);
}

static void mathkern_finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *stuff;
    MathDlg *math;
    int cols;
    SplineChar *sc;

    if ( c!=0 || !wasnew )
return;
    /* Popup up a dlg if they added a glyph */
    math = GDrawGetUserData(GGadgetGetWindow(g));
    stuff = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( stuff[r*cols+0].u.md_str==NULL )
return;
    sc = SFGetChar(math->sf,-1,stuff[r*cols+0].u.md_str);
    if ( sc==NULL )
return;
    MathKernDialog(sc);
}

static void extpart_finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *stuff;
    MathDlg *math;
    int cols;
    DBounds b;
    double full_advance;
    SplineChar *sc;

    if ( c!=0 )
return;
    if ( !wasnew )
return;
    /* If they added a new glyph to the sequence then set some defaults for it. */
    /*  only the full advance has any likelyhood of being correct */
    math = GDrawGetUserData(GGadgetGetWindow(g));
    stuff = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( stuff[r*cols+0].u.md_str==NULL )
return;
    sc = SFGetChar(math->sf,-1,stuff[r*cols+0].u.md_str);
    if ( sc==NULL )
return;
    SplineCharFindBounds(sc,&b);
    if ( math->is_horiz )
	full_advance = b.maxx - b.minx;
    else
	full_advance = b.maxy - b.miny;
    stuff[r*cols+2].u.md_ival = stuff[r*cols+3].u.md_ival = rint(full_advance/3);
    stuff[r*cols+4].u.md_ival = rint(full_advance);
    GGadgetRedraw(g);
}

static void GVC_PopupPrepare(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));

    math->popup_g = g;
    if ( math->sc==NULL )
return;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,math,_GVC_GetImage,MATH_FreeImage);
}

static int GVC_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
	math->done = true;
	math->ok = true;
    }
return( true );
}

static int MATH_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
	math->done = true;
    }
return( true );
}

static int gc_e_h(GWindow gw, GEvent *event) {
    MathDlg *math = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	math->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("math.html#GlyphConstruction");
return( true );
	}
return( false );
    }
return( true );
}

static char *GlyphConstruction_Dlg(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    MathDlg md;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);
    GGadgetCreateData *harray[7], mgcd[4], *varray[6], mboxes[3];
    GTextInfo mlabel[3];
    struct glyphvariants *gv;
    char *ret;

    memset(&md,0,sizeof(md));
    md.sf = math->sf;
    md.is_horiz = GGadgetGetCid(g)==CID_HGlyphConst;
    md.sc = SFGetChar(md.sf,-1,old[r*cols+0].u.md_str);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Glyph Construction");
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    md.gw = gw = GDrawCreateTopWindow(NULL,&pos,gc_e_h,&md,&wattrs);

    memset(mgcd,0,sizeof(mgcd));
    memset(mlabel,0,sizeof(mlabel));
    memset(mboxes,0,sizeof(mboxes));

    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.u.matrix = &mi_extensionpart;
    mgcd[0].gd.cid = CID_VGlyphConst;
    mgcd[0].creator = GMatrixEditCreate;

    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _("_OK");
    mlabel[1].text_is_1byte = true;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = GVC_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _("_Cancel");
    mlabel[2].text_is_1byte = true;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = MATH_Cancel;
    mgcd[2].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &mgcd[1]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &mgcd[2]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    mboxes[2].gd.flags = gg_enabled|gg_visible;
    mboxes[2].gd.u.boxelements = harray;
    mboxes[2].creator = GHBoxCreate;

    varray[0] = &mgcd[0]; varray[1] = NULL;
    varray[2] = &mboxes[2]; varray[3] = NULL;
    varray[4] = NULL;

    mboxes[0].gd.pos.x = mboxes[0].gd.pos.y = 2;
    mboxes[0].gd.flags = gg_enabled|gg_visible;
    mboxes[0].gd.u.boxelements = varray;
    mboxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,mboxes);
    GHVBoxSetExpandableRow(mboxes[0].ret,0);
    GHVBoxSetExpandableCol(mboxes[2].ret,gb_expandgluesame);
    GMatrixEditSetColumnCompletion(mgcd[0].ret,0,MATH_GlyphNameCompletion);
    GMatrixEditSetMouseMoveReporter(mgcd[0].ret,GVC_PopupPrepare);

    /* If it's unparseable, this will give 'em nothing */
    gv = GV_FromString(NULL,old[r*cols+cols-1].u.md_str);
    GV_ToMD(mgcd[0].ret,gv);
    GlyphVariantsFree(gv);

    GHVBoxFitWindow(mboxes[0].ret);

    GDrawSetVisible(md.gw,true);

    while ( !md.done )
	GDrawProcessOneEvent(NULL);

    if ( md.ok ) {
	int rs, cs = GMatrixEditGetColCnt(mgcd[0].ret);
	struct matrix_data *stuff = GMatrixEditGet(mgcd[0].ret,&rs);
	gv = GV_ParseConstruction(NULL,stuff,rs,cs);
	ret = GV_ToString(gv);
	GlyphVariantsFree(gv);
    } else
	ret = copy( old[r*cols+cols-1].u.md_str );
    GDrawDestroyWindow(md.gw);
return( ret );
}

static char *MKChange_Dlg(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);
    SplineChar *sc;

    if ( old[r*cols+0].u.md_str==NULL )
return( NULL );
    sc = SFGetChar(math->sf,-1,old[r*cols+0].u.md_str);
    if ( sc==NULL )
return( NULL );

    MathKernDialog(sc);
return( NULL );
}

static int MATH_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int cid,i,high,low;
	SplineFont *sf = math->sf;
	SplineChar *sc;

	/* Two passes. First checks that everything is parsable */
	for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	    GetInt8(math->gw,2*i+1,math_constants_descriptor[i].ui_name,&err);
	    if ( err )
return( true );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
		GGadget *tf2 = GWidgetGetControl(math->gw,2*i+2);
		char *str = GGadgetGetTitle8(tf2);
		if ( !DeviceTableOK(str,&low,&high)) {
		    gwwv_post_error(_("Bad device table"), _("Bad device table for %s"),
			    math_constants_descriptor[i].ui_name);
		    free(str);
return( true );
		}
		free(str);
	    }
#endif
	}
	/* Now check that the various glyph lists are parseable */
	for ( cid=CID_Exten; cid<=CID_HGlyphConst; ++cid ) {
	    GGadget *g = GWidgetGetControl(math->gw,cid);
	    int rows, cols = GMatrixEditGetColCnt(g);
	    struct matrix_data *old = GMatrixEditGet(g,&rows);
	    for ( i=0; i<rows; ++i ) {
		if ( SFGetChar(sf,-1,old[i*cols+0].u.md_str)==NULL ) {
		    gwwv_post_error(_("Missing Glyph"), _("There is no glyph named %s (used in %s)"),
			    old[i*cols+0].u.md_str, gi_aspectnames[cid-CID_Exten]);
return( true );
		}
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( cid==CID_Italic || cid==CID_TopAccent ||
			cid == CID_VGlyphConst || cid == CID_HGlyphConst ) {
		    if ( !DeviceTableOK(old[i*cols+2].u.md_str,&low,&high)) {
			gwwv_post_error(_("Bad device table"), _("Bad device table for glyph %s in %s"),
				old[i*cols+0].u.md_str, gi_aspectnames[cid-CID_Exten]);
return( true );
		    }
		}
#endif
		if ( cid == CID_VGlyphConst || cid == CID_HGlyphConst ) {
		    if ( GV_StringCheck(sf,old[i*cols+cols-1].u.md_str)==-1 ) {
			gwwv_post_error(_("Bad Parts List"), _("Bad parts list for glyph %s in %s"),
				old[i*cols+0].u.md_str, gi_aspectnames[cid-CID_Exten]);
return( true );
		    }
		}
		if ( cid == CID_VGlyphConst || cid == CID_HGlyphConst ) {
		    if ( !SF_NameListCheck(sf,old[i*cols+1].u.md_str)) {
			gwwv_post_error(_("Bad Variants List"), _("Bad Variants list for glyph %s in %s"),
				old[i*cols+0].u.md_str, gi_aspectnames[cid-CID_Exten]);
return( true );
		    }
		}
	    }
	}

	/*********************************************/
	/* Ok, if we got this far it should be legal */
	/*********************************************/
	for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	    int16 *pos = (int16 *) (((char *) (math->math)) + math_constants_descriptor[i].offset );
	    *pos = GetInt8(math->gw,2*i+1,math_constants_descriptor[i].ui_name,&err);

#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
		GGadget *tf2 = GWidgetGetControl(math->gw,2*i+2);
		char *str = GGadgetGetTitle8(tf2);
		DeviceTable **devtab = (DeviceTable **) (((char *) (math->math)) + math_constants_descriptor[i].devtab_offset );

		*devtab = DeviceTableParse(*devtab,str);
		free(str);
	    }
#endif
	}
	sf->MATH = math->math;

	/* As for the per-glyph stuff... Well the only way I can insure that */
	/* things which have been removed in the dlg are removed in the font */
	/* is to clear everything now, and start from a blank slate when I   */
	/* parse stuff. (Except for math kerning which I don't support here) */
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    sc->is_extended_shape = false;
	    sc->italic_correction = TEX_UNDEF;
	    sc->top_accent_horiz  = TEX_UNDEF;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    DeviceTableFree(sc->italic_adjusts);
	    DeviceTableFree(sc->top_accent_adjusts);
	    sc->italic_adjusts = sc->top_accent_adjusts = NULL;
#endif
	    GlyphVariantsFree(sc->vert_variants);
	    GlyphVariantsFree(sc->horiz_variants);
	    sc->vert_variants = sc->horiz_variants = NULL;
	    /* MathKernFree(sc->mathkern); sc->mathkern = NULL; */
	}
	/* Then process each table to set whatever it sets */
	for ( cid=CID_Exten; cid<=CID_HGlyphConst; ++cid ) {
	    GGadget *g = GWidgetGetControl(math->gw,cid);
	    int rows, cols = GMatrixEditGetColCnt(g);
	    struct matrix_data *old = GMatrixEditGet(g,&rows);
	    for ( i=0; i<rows; ++i ) {
		sc = SFGetChar(sf,-1,old[i*cols+0].u.md_str);
		if ( cid==CID_Exten )
		    sc->is_extended_shape = old[i*cols+1].u.md_ival;
		else if ( cid==CID_Italic ) {
		    sc->italic_correction = old[i*cols+1].u.md_ival;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    sc->italic_adjusts = DeviceTableParse(NULL,old[i*cols+2].u.md_str);
#endif
		} else if ( cid==CID_TopAccent ) {
		    sc->top_accent_horiz = old[i*cols+1].u.md_ival;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    sc->top_accent_adjusts = DeviceTableParse(NULL,old[i*cols+2].u.md_str);
#endif
		} else if ( cid==CID_VGlyphVar || cid==CID_HGlyphVar ) {
		    struct glyphvariants **gvp = cid == CID_VGlyphVar ?
			    &sc->vert_variants : &sc->horiz_variants;
		    char *str = old[i*cols+1].u.md_str;
		    if ( str!=NULL ) while ( *str==' ' ) ++str;
		    if ( str!=NULL && *str!='\0' ) {
			*gvp = chunkalloc(sizeof(struct glyphvariants));
			(*gvp)->variants = copy( str );
		    }
		} else if ( cid==CID_VGlyphConst || cid==CID_HGlyphConst ) {
		    struct glyphvariants **gvp = cid == CID_VGlyphConst ?
			    &sc->vert_variants : &sc->horiz_variants;
		    *gvp = GV_FromString(*gvp,old[cols*i+cols-1].u.md_str);
		    if ( *gvp!=NULL && (*gvp)->part_cnt!=0 ) {
			(*gvp)->italic_correction = old[i*cols+1].u.md_ival;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			(*gvp)->italic_adjusts = DeviceTableParse(NULL,old[i*cols+2].u.md_str);
#endif
		    }
		}
	    }
	}

	/* Done! */

	math->done = true;
	math->ok = true;
    }
return( true );
}

static int math_e_h(GWindow gw, GEvent *event) {
    MathDlg *math = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	math->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("math.html");
return( true );
	}
return( false );
    }
return( true );
}

#define MAX_PAGE	9
#define MAX_ROW		12

void SFMathDlg(SplineFont *sf) {
    MathDlg md;
    int i, j, page, row, h;
    GGadget *g;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[MAX_PAGE][MAX_ROW][3], boxes[MAX_PAGE][2],
	    *hvarray[MAX_PAGE][MAX_ROW+1][4], *harray[7], mgcd[4],
	    *varray[6], mboxes[3], gi[8][2];
    GTextInfo label[MAX_PAGE][MAX_ROW], mlabel[3];
    GTabInfo aspects[MAX_PAGE+8+1];

    MathInit();

    memset(&md,0,sizeof(md));
    if ( sf->cidmaster ) sf = sf->cidmaster;
    md.sf = sf;
    md.math = sf->MATH;
    if ( md.math==NULL )
	md.math = MathTableNew(sf);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("MATH table");
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    md.gw = gw = GDrawCreateTopWindow(NULL,&pos,math_e_h,&md,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    memset(boxes,0,sizeof(boxes));
    memset(aspects,0,sizeof(aspects));
    memset(gi,0,sizeof(gi));

    page = row = 0;
    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	if ( math_constants_descriptor[i].new_page ) {
	    hvarray[page][row][0] = hvarray[page][row][1] = hvarray[page][row][2] = GCD_Glue;
	    hvarray[page][row][3] = NULL;
	    hvarray[page][row+1][0] = NULL;
	    ++page;
	    if ( page>=MAX_PAGE ) {
		IError( "Too many pages" );
return;
	    }
	    row = 0;
	}

	label[page][row].text = (unichar_t *) math_constants_descriptor[i].ui_name;
	label[page][row].text_is_1byte = true;
	label[page][row].text_in_resource = true;
	gcd[page][row][0].gd.label = &label[page][row];
	gcd[page][row][0].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	gcd[page][row][0].gd.popup_msg = (unichar_t *) math_constants_descriptor[i].message;
	gcd[page][row][0].creator = GLabelCreate;
	hvarray[page][row][0] = &gcd[page][row][0];

	gcd[page][row][1].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	gcd[page][row][1].gd.pos.width = 50;
	gcd[page][row][1].gd.cid = 2*i+1;
	gcd[page][row][1].gd.popup_msg = (unichar_t *) math_constants_descriptor[i].message;
	gcd[page][row][1].creator = GTextFieldCreate;
	hvarray[page][row][1] = &gcd[page][row][1];

#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( math_constants_descriptor[i].devtab_offset>=0 ) {
	    gcd[page][row][2].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    gcd[page][row][2].gd.cid = 2*i+2;
	    gcd[page][row][2].gd.popup_msg = (unichar_t *) math_constants_descriptor[i].message;
	    gcd[page][row][2].creator = GTextFieldCreate;
	    hvarray[page][row][2] = &gcd[page][row][2];
	} else
#endif
	    hvarray[page][row][2] = GCD_Glue;
	hvarray[page][row][3] = NULL;

	if ( ++row>=MAX_ROW ) {
	    IError( "Too many rows" );
return;
	}
    }
    hvarray[page][row][0] = hvarray[page][row][1] = hvarray[page][row][2] = GCD_Glue;
    hvarray[page][row][3] = NULL;
    hvarray[page][row+1][0] = NULL;

    for ( i=0; aspectnames[i]!=NULL; ++i ) {
	boxes[i][0].gd.flags = gg_enabled|gg_visible;
	boxes[i][0].gd.u.boxelements = hvarray[i][0];
	boxes[i][0].creator = GHVBoxCreate;

	aspects[i].text = (unichar_t *) aspectnames[i];
	aspects[i].text_is_1byte = true;
	aspects[i].nesting = i!=0;
	aspects[i].gcd = boxes[i];
    }
    if ( i!=page+1 ) {	/* Page never gets its final increment */
	IError( "Page miscount %d in descriptor table, but only %d names.", page+1, i );
return;
    }

    for ( j=0; mis[j].col_cnt!=0; ++j ) {
	gi[j][0].gd.flags = gg_enabled|gg_visible;
	gi[j][0].gd.u.matrix = &mis[j];
	gi[j][0].gd.cid = CID_Exten+j;
	gi[j][0].creator = GMatrixEditCreate;

	aspects[i+j].text = (unichar_t *) gi_aspectnames[j];
	aspects[i+j].text_is_1byte = true;
	aspects[i+j].gcd = gi[j];
    }

    memset(mgcd,0,sizeof(mgcd));
    memset(mlabel,0,sizeof(mlabel));
    memset(mboxes,0,sizeof(mboxes));

    mgcd[0].gd.u.tabs = aspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
    /*mgcd[0].gd.cid = CID_Tabs;*/
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _("_OK");
    mlabel[1].text_is_1byte = true;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = MATH_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _("_Cancel");
    mlabel[2].text_is_1byte = true;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = MATH_Cancel;
    mgcd[2].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &mgcd[1]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &mgcd[2]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    mboxes[2].gd.flags = gg_enabled|gg_visible;
    mboxes[2].gd.u.boxelements = harray;
    mboxes[2].creator = GHBoxCreate;

    varray[0] = &mgcd[0]; varray[1] = NULL;
    varray[2] = &mboxes[2]; varray[3] = NULL;
    varray[4] = NULL;

    mboxes[0].gd.pos.x = mboxes[0].gd.pos.y = 2;
    mboxes[0].gd.flags = gg_enabled|gg_visible;
    mboxes[0].gd.u.boxelements = varray;
    mboxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,mboxes);
    GHVBoxSetExpandableRow(mboxes[0].ret,0);
    GHVBoxSetExpandableCol(mboxes[2].ret,gb_expandgluesame);
    for ( i=0; aspectnames[i]!=NULL; ++i ) {
	GHVBoxSetExpandableCol(boxes[i][0].ret,2);
	GHVBoxSetExpandableRow(boxes[i][0].ret,gb_expandglue);
    }
    for ( j=0; mis[j].col_cnt!=0; ++j )
	GMatrixEditSetColumnCompletion(gi[j][0].ret,0,MATH_GlyphNameCompletion);
    for ( h=0; h<2; ++h ) {
	g = GWidgetGetControl(md.gw,CID_VGlyphVar+2*h);
	GMatrixEditSetColumnCompletion(g,1,MATH_GlyphListCompletion);
	GMatrixEditSetMouseMoveReporter(g,MATHVar_PopupPrepare);
	g = GWidgetGetControl(md.gw,CID_VGlyphConst+2*h);
	GMatrixEditSetMouseMoveReporter(g,MATHConst_PopupPrepare);
    }
    GMatrixEditSetMouseMoveReporter(GWidgetGetControl(md.gw,CID_Italic),MATHLine_PopupPrepare);
    GMatrixEditSetMouseMoveReporter(GWidgetGetControl(md.gw,CID_TopAccent),MATHLine_PopupPrepare);
    MATH_Init(&md);
    GHVBoxFitWindow(mboxes[0].ret);

    GDrawSetVisible(md.gw,true);

    while ( !md.done )
	GDrawProcessOneEvent(NULL);
    if ( sf->MATH==NULL && !md.ok )
	MATHFree(md.math);

    GDrawDestroyWindow(md.gw);
}

/* ************************************************************************** */
/* ****************************** Math Kern Dlg ***************************** */
/* ************************************************************************** */

#define CID_TopBox	1000
#define CID_Glyph	1001

static void MKD_SetGlyphList(MathKernDlg *mkd, SplineChar *sc) {
    SplineFont *sf = sc->parent;
    int k,cnt, gid;
    GTextInfo **tis = NULL;
    SplineChar *test;

    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (test=sf->glyphs[gid])!=NULL ) {
	    if ( test==sc || test->mathkern!=NULL ) {
		if ( k ) {
		    tis[cnt] = gcalloc(1,sizeof(GTextInfo));
		    tis[cnt]->text = utf82u_copy(test->name);
		    tis[cnt]->userdata = test;
		    tis[cnt]->selected = test==sc;
		    tis[cnt]->fg = tis[cnt]->bg = COLOR_DEFAULT;
		}
		++cnt;
	    }
	}
	if ( !k )
	    tis = galloc((cnt+1)*sizeof(GTextInfo *));
	else
	    tis[cnt] = gcalloc(1,sizeof(GTextInfo));
    }
    GGadgetSetList(GWidgetGetControl(mkd->gw,CID_Glyph),tis,false);
}
    
static void MKDResize(MathKernDlg *mkd, GEvent *event) {
    int width, height;
    int i;

    if ( !event->u.resize.sized )
return;

    GGadgetMove(GWidgetGetControl(mkd->gw,CID_TopBox),4,4);
    GGadgetResize(GWidgetGetControl(mkd->gw,CID_TopBox),
	    event->u.resize.size.width-8,
	    event->u.resize.size.height-12);
    
    width = (event->u.resize.size.width-4*mkd->mid_space)/4;
    height = (event->u.resize.size.height-mkd->cv_y-mkd->button_height-8);
    if ( width<70 || height<80 ) {
	if ( width<70 ) width = 70;
	width = 4*(width+mkd->mid_space);
	if ( height<80 ) height = 80;
	height += mkd->cv_y+mkd->button_height+8;
	GDrawResize(mkd->gw,width,height);
return;
    }
    if ( width!=mkd->cv_width || height!=mkd->cv_height ) {
	mkd->cv_width = width; mkd->cv_height = height;
	for ( i=0; i<4; ++i ) {
	    CharView *cv = (&mkd->cv_topright)+i;
	    GDrawResize(cv->gw,width,height);
	    if ( i!=0 )
		GDrawMove(cv->gw,10+i*(mkd->cv_width+mkd->mid_space),mkd->cv_y);
	}
    }

    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    GDrawRequestExpose(mkd->gw,NULL,false);
}


static void MKDDraw(MathKernDlg *mkd, GWindow pixmap, GEvent *event) {
    GRect r;
    int i;

    GDrawSetLineWidth(pixmap,0);
    for ( i=0; i<4; ++i ) {
	CharView *cv = (&mkd->cv_topright)+i;

	r.x = 10+i*(mkd->cv_width+mkd->mid_space)-1; r.y=mkd->cv_y-1;
	r.width = mkd->cv_width+1; r.height = mkd->cv_height+1;
	GDrawDrawRect(pixmap,&r,0);

	GDrawSetFont(pixmap,cv->inactive ? mkd->plain : mkd->bold);
	GDrawDrawText8(pixmap,r.x,mkd->mbh+5+mkd->as,cornernames[i],-1,NULL,0);
    }
}

void MKDMakeActive(MathKernDlg *mkd,CharView *cv) {
    GRect r;
    int i;

    if ( mkd==NULL )
return;
    for ( i=0; i<4; ++i )
	(&mkd->cv_topright)[i].inactive = true;
    cv->inactive = false;
    GDrawSetUserData(mkd->gw,cv);
    for ( i=0; i<4; ++i )
	GDrawRequestExpose((&mkd->cv_topright)[i].v,NULL,false);
    GDrawGetSize(mkd->gw,&r);
    r.x = 0;
    r.y = mkd->mbh;
    r.height = mkd->fh+10;
    GDrawRequestExpose(mkd->gw,&r,false);
}

void MKDChar(MathKernDlg *mkd, GEvent *event) {
    int i;
    for ( i=0; i<4; ++i )
	if ( !(&mkd->cv_topright)[i].inactive )
    break;

    if ( event->u.chr.keysym==GK_Tab || event->u.chr.keysym==GK_BackTab ) {
	if ( event->u.chr.keysym==GK_Tab ) ++i; else --i;
	if ( i<0 ) i=3; else if ( i>3 ) i = 0;
	MKDMakeActive(mkd,(&mkd->cv_topright)+i);
    } else
	CVChar((&mkd->cv_topright)+i,event);
}

void MKD_DoClose(struct cvcontainer *cvc) {
    MathKernDlg *mkd = (MathKernDlg *) cvc;
    int i;

    for ( i=0; i<4; ++i ) {
	SplineChar *msc = &(&mkd->sc_topright)[i];
	SplinePointListsFree(msc->layers[0].splines);
	SplinePointListsFree(msc->layers[1].splines);
#ifdef FONTFORGE_CONFIG_TYPE3
	free( msc->layers );
#endif
    }

    mkd->done = true;
}

static int mkd_e_h(GWindow gw, GEvent *event) {
    MathKernDlg *mkd = (MathKernDlg *) ((CharView *) GDrawGetUserData(gw))->container;
    int i;

    switch ( event->type ) {
      case et_expose:
	MKDDraw(mkd,gw,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    MKDResize(mkd,event);
      break;
      case et_char:
	MKDChar(mkd,event);
      break;
      case et_close:
	MKD_DoClose((struct cvcontainer *) mkd);
      break;
      case et_create:
      break;
      case et_map:
	for ( i=0; i<4; ++i ) {
	    CharView *cv = (&mkd->cv_topright)+i;
	    if ( !cv->inactive ) {
		if ( event->u.map.is_visible )
		    CVPaletteActivate(cv);
		else
		    CVPalettesHideIfMine(cv);
	break;
	    }
	}
	/* mkd->isvisible = event->u.map.is_visible; */
      break;
    }
return( true );
}

static void MKDFillup(MathKernDlg *mkd, SplineChar *sc) {
    int i, j, rows;
    SplineSet *last, *cur;
    RefChar *ref;
    GTextInfo **list;

    for ( i=0; i<4; ++i ) {
	SplineChar *msc = &(&mkd->sc_topright)[i];
	struct mathkernvertex *mkv = sc->mathkern==NULL ? NULL : &(&sc->mathkern->top_right)[i];
	msc->width = sc->width;
	msc->italic_correction = sc->italic_correction;
	msc->top_accent_horiz = sc->top_accent_horiz;
	last = NULL;
	SplinePointListsFree(msc->layers[0].splines);
	SplinePointListsFree(msc->layers[1].splines);
	msc->layers[0].splines = msc->layers[1].splines = NULL;

	/* copy the character itself into the background */
	last = msc->layers[0].splines = SplinePointListCopy(sc->layers[ly_fore].splines);
	if ( last!=NULL )
	    while ( last->next!=NULL ) last = last->next;
	for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	    if ( last==NULL )
		cur = SplinePointListCopy(ref->layers[0].splines);
	    if ( last==NULL )
		msc->layers[0].splines = cur;
	    else
		last->next = cur;
	    if ( cur!=NULL )
		for ( last=cur; last->next==NULL; last = last->next );
	}
	/* Now copy the dots from the mathkern vertex structure */
	last = NULL;
	for ( j=0; j<mkv->cnt; ++j ) {
	    cur = chunkalloc(sizeof(SplineSet));
	    cur->first = cur->last = SplinePointCreate(mkv->mkd[j].kern + ((i&1)?0:sc->width), mkv->mkd[j].height );
	    cur->first->pointtype = pt_corner;
	    if ( last==NULL )
		msc->layers[ly_fore].splines = cur;
	    else
		last->next = cur;
	    last = cur;
	}
    }
    mkd->cursc = sc;

    list = GGadgetGetList(GWidgetGetControl(mkd->gw,CID_Glyph),&rows);
    for ( i=rows-1; i>=0; --i )
	if ( list[i]->userdata==sc )
    break;
    if ( i>=0 )
	GGadgetSelectOneListItem(GWidgetGetControl(mkd->gw,CID_Glyph),i);
}

static void MKDFillupRefresh(MathKernDlg *mkd, SplineChar *sc) {
    int i;

    MKDFillup(mkd, sc);
    for ( i=0; i<4; ++i ) {
	CharView *cv = &mkd->cv_topright + i;
	GDrawRequestExpose(cv->gw,NULL,false);
	GDrawRequestExpose(cv->v,NULL,false);
    }
}

static int bp_order_height(const void *bpp1, const void *bpp2) {
    const BasePoint *bp1 = *(const BasePoint **) bpp1;
    const BasePoint *bp2 = *(const BasePoint **) bpp2;
    if ( bp1->y > bp2->y )
return( 1 );
    else if ( bp1->y < bp2->y )
return( -1 );

return( 0 );
}

static int MKD_Parse(MathKernDlg *mkd) {
    int i, cnt, j, k;
    SplineSet *ss;
    SplinePoint *sp;
    BasePoint **bases;
    int allzeros = true;

    if ( mkd->cursc->mathkern==NULL )
	mkd->cursc->mathkern = chunkalloc(sizeof(struct mathkern));

    for ( i=0; i<3; ++i ) {
	SplineChar *msc = &(&mkd->sc_topright)[i];
	struct mathkernvertex *mkv = &(&mkd->cursc->mathkern->top_right)[i];

	for ( k=0; k<2; ++k ) {
	    cnt = 0;
	    for ( ss = msc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
		for ( sp=ss->first ; ; ) {
		    if ( k )
			bases[cnt] = &sp->me;
		    ++cnt;
		    if ( sp->next == NULL )
		break;
		    sp = sp->next->to;
		    if ( sp == ss->first )
		break;
		}
	    }
	    if ( !k )
		bases = galloc(cnt*sizeof(BasePoint *));
	}
	qsort(bases,cnt,sizeof(BasePoint *),bp_order_height);
	if ( cnt>mkv->cnt ) {
	    mkv->mkd = grealloc(mkv->mkd,cnt*sizeof(struct mathkernvertex));
	    if ( mkv->cnt!=0 )
		memset(mkv->mkd+mkv->cnt,0,(cnt-mkv->cnt)*sizeof(struct mathkernvertex));
	}
	for ( j=0; j<cnt; ++j ) {
	    bases[j]->x = rint(bases[j]->x);
	    if ( !(i&1) ) bases[j]->x -= mkd->cursc->width;
	    bases[j]->y = rint(bases[j]->y);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    /* If we have a previous entry with this height retain the height dv */
	    /* If we have a previous entry with this height and width retain the width dv too */
	    for ( k=j; k<mkv->cnt; ++k )
		if ( bases[j]->y == mkv->mkd[k].height )
	    break;
	    if ( k!=j ) {
		DeviceTableFree(mkv->mkd[j].height_adjusts);
		DeviceTableFree(mkv->mkd[j].kern_adjusts);
		mkv->mkd[j].height_adjusts = mkv->mkd[j].kern_adjusts = NULL;
	    }
	    if ( k<mkv->cnt ) {
		mkv->mkd[j].height_adjusts = mkv->mkd[k].height_adjusts;
		if ( bases[j]->x == mkv->mkd[k].kern )
		    mkv->mkd[j].kern_adjusts = mkv->mkd[k].kern_adjusts;
		else
		    DeviceTableFree(mkv->mkd[k].kern_adjusts);
		mkv->mkd[k].height_adjusts = mkv->mkd[k].kern_adjusts = NULL;
	    }
#endif
	    mkv->mkd[j].height = bases[j]->y;
	    mkv->mkd[j].kern   = bases[j]->x;
	}
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	for ( ; j<mkv->cnt; ++j ) {
	    DeviceTableFree(mkv->mkd[j].height_adjusts);
	    DeviceTableFree(mkv->mkd[j].kern_adjusts);
	    mkv->mkd[j].height_adjusts = mkv->mkd[j].kern_adjusts = NULL;
	}
#endif
	mkv->cnt = cnt;
	free(bases);
	if ( cnt!=0 )
	    allzeros = false;
    }
    if ( allzeros ) {
	MathKernFree(mkd->cursc->mathkern);
	mkd->cursc->mathkern = NULL;
    }
    /* The only potential error is two entries with the same height, and I don't */
    /*  check for that */
return( true );
}

static int MathKernD_GlyphChanged(GGadget *g, GEvent *e) {
    MathKernDlg *mkd = (MathKernDlg *) (((CharView *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GTextInfo *sel = GGadgetGetListItemSelected(g);

	if ( sel!=NULL && MKD_Parse(mkd)) {
	    SplineChar *sc = sel->userdata;
	    MKDFillupRefresh(mkd, sc);
	}
    }
return( true );
}

static int MathKernD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	MKD_DoClose(((CharView *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
return( true );
}

static int MathKernD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathKernDlg *mkd = (MathKernDlg *) (((CharView *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	if ( MKD_Parse(mkd) )
	    MKD_DoClose( (struct cvcontainer *) mkd );
    }
return( true );
}

static int MKD_Can_Navigate(struct cvcontainer *cvc, enum nav_type type) {
return( true );
}

static int MKD_Can_Open(struct cvcontainer *cvc) {
return( false );
}

static void MKD_Do_Navigate(struct cvcontainer *cvc, enum nav_type type) {
    MathKernDlg *mkd = ( MathKernDlg * ) cvc;
    SplineChar *sc = NULL;
    int pos;
    GGadget *list = GWidgetGetControl(mkd->gw,CID_Glyph);
    int32 rows;
    GTextInfo **tis;

    if ( !MKD_Parse(mkd))
return;

    if ( type == nt_goto ) {
	SplineFont *sf = mkd->cursc->parent;
	int enc = GotoChar(sf,sf->fv->map);
	if ( enc==-1 || sf->fv->map->map[enc]==-1 || (sc = sf->glyphs[ sf->fv->map->map[enc] ])==NULL )
return;
	if ( sc->mathkern==NULL )
	    MKD_SetGlyphList(mkd,sc);
    } else if ( type == nt_next || type == nt_nextdef ) {
	tis = GGadgetGetList(list,&rows);
	for ( pos=rows-1; pos>=0; --pos )
	    if ( tis[pos]->selected )
	break;
	++pos;
	if ( pos==rows )
return;
	sc = tis[pos]->userdata;
    } else {
	tis = GGadgetGetList(list,&rows);
	for ( pos=rows-1; pos>=0; --pos )
	    if ( tis[pos]->selected )
	break;
	if ( pos<=0 )
return;
	--pos;
	sc = tis[pos]->userdata;
    }
    MKDFillupRefresh(mkd,sc);
}

struct cvcontainer_funcs mathkern_funcs = {
    cvc_mathkern,
    (void (*) (struct cvcontainer *cvc,CharView *cv)) MKDMakeActive,
    (void (*) (struct cvcontainer *cvc,GEvent *)) MKDChar,
    MKD_Can_Navigate,
    MKD_Do_Navigate,
    MKD_Can_Open,
    MKD_DoClose
};

static void MKDInit(MathKernDlg *mkd,SplineChar *sc) {
    int i;

    memset(mkd,0,sizeof(*mkd));
    mkd->base.funcs = &mathkern_funcs;

    for ( i=0; i<4; ++i ) {
	SplineChar *msc = &(&mkd->sc_topright)[i];
	CharView *mcv = &(&mkd->cv_topright)[i];
	msc->orig_pos = i;
	msc->unicodeenc = -1;
	msc->name = i==0 ? "TopRight" :
		    i==1 ? "TopLeft"  :
		    i==2 ? "BottomRight":
			    "BottomLeft";
	msc->parent = &mkd->dummy_sf;
	msc->layer_cnt = 2;
#ifdef FONTFORGE_CONFIG_TYPE3
	msc->layers = gcalloc(2,sizeof(Layer));
	LayerDefault(&msc->layers[0]);
	LayerDefault(&msc->layers[1]);
#endif
	mkd->chars[i] = msc;

	mcv->sc = msc;
	mcv->layerheads[dm_fore] = &msc->layers[ly_fore];
	mcv->layerheads[dm_back] = &msc->layers[ly_back];
	mcv->layerheads[dm_grid] = NULL;
	mcv->drawmode = dm_fore;
	mcv->container = (struct cvcontainer *) mkd;
	mcv->inactive = i!=0;
    }
    mkd->dummy_sf.glyphs = mkd->chars;
    mkd->dummy_sf.glyphcnt = mkd->dummy_sf.glyphmax = 4;
    mkd->dummy_sf.pfminfo.fstype = -1;
    mkd->dummy_sf.fontname = mkd->dummy_sf.fullname = mkd->dummy_sf.familyname = "dummy";
    mkd->dummy_sf.weight = "Medium";
    mkd->dummy_sf.origname = "dummy";
    mkd->dummy_sf.ascent = sc->parent->ascent;
    mkd->dummy_sf.descent = sc->parent->descent;
    mkd->dummy_sf.order2 = sc->parent->order2;
    mkd->dummy_sf.anchor = NULL;

    mkd->dummy_sf.fv = &mkd->dummy_fv;
    mkd->dummy_fv.sf = &mkd->dummy_sf;
    mkd->dummy_fv.selected = mkd->sel;
    mkd->dummy_fv.cbw = mkd->dummy_fv.cbh = default_fv_font_size+1;
    mkd->dummy_fv.magnify = 1;

    mkd->dummy_fv.map = &mkd->dummy_map;
    mkd->dummy_map.map = mkd->map;
    mkd->dummy_map.backmap = mkd->backmap;
    mkd->dummy_map.enccount = mkd->dummy_map.encmax = mkd->dummy_map.backmax = 4;
    mkd->dummy_map.enc = &custom;
}

void MathKernDialog(SplineChar *sc) {
    MathKernDlg mkd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[5], boxes[4], *harray[8], *varray[5], *garray[5];
    GTextInfo label[5];
    FontRequest rq;
    int as, ds, ld;
    static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a',',','c','a','l','i','b','a','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    int i,k;

    MKDInit( &mkd, sc );

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_isdlg|wam_restrict|wam_undercursor|wam_utf8_wtitle;
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Math Kerning");
    pos.width = 600;
    pos.height = 400;
    mkd.gw = gw = GDrawCreateTopWindow(NULL,&pos,mkd_e_h,&mkd.cv_topright,&wattrs);

    memset(&rq,0,sizeof(rq));
    rq.family_name = helv;
    rq.point_size = 12;
    rq.weight = 400;
    mkd.plain = GDrawInstanciateFont(NULL,&rq);
    rq.weight = 700;
    mkd.bold = GDrawInstanciateFont(NULL,&rq);
    GDrawFontMetrics(mkd.plain,&as,&ds,&ld);
    mkd.fh = as+ds; mkd.as = as;

    MKDCharViewInits(&mkd);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k = 0;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Glyph;
    gcd[k].gd.handle_controlevent = MathKernD_GlyphChanged;
    gcd[k++].creator = GListButtonCreate;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_default;
    gcd[k].gd.handle_controlevent = MathKernD_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Done");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
    gcd[k].gd.handle_controlevent = MathKernD_Cancel;
    gcd[k].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &gcd[1]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &gcd[2]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;

    garray[0] = &gcd[0]; garray[1] = GCD_Glue; garray[2] = NULL;
    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = garray;
    boxes[3].creator = GHBoxCreate;

    varray[0] = GCD_Glue;
    varray[1] = &boxes[3];
    varray[2] = &boxes[2];
    varray[3] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].gd.cid = CID_TopBox;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,boxes);

    MKD_SetGlyphList(&mkd, sc);
    MKDFillup( &mkd, sc );

    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GGadgetResize(boxes[0].ret,pos.width,pos.height);

    mkd.button_height = GDrawPointsToPixels(gw,60);
    GDrawResize(gw,1000,400);		/* Force a resize event */

    GDrawSetVisible(mkd.gw,true);

    while ( !mkd.done )
	GDrawProcessOneEvent(NULL);

    for ( i=0; i<4; ++i ) {
	CharView *cv = &mkd.cv_topright + i;
	if ( cv->backimgs!=NULL ) {
	    GDrawDestroyWindow(cv->backimgs);
	    cv->backimgs = NULL;
	}
    }
    GDrawDestroyWindow(mkd.gw);
}
