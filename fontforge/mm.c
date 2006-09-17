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
#include "pfaeditui.h"
#include <ustring.h>
#include <math.h>
#include <gkeysym.h>
#include <locale.h>
#include <utype.h>
#include "ttf.h"

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
/* As far as I can tell, the CDV in AdobeSansMM is half gibberish */
/* This is disturbing */
/* But at least the CDV in Type1_supp.pdf for Myriad appears correct */
static char *standard_cdvs[5] = {
/* 0 axes? Impossible */
    "{}",
/* 1 axis */
    "{\n"
    "  1 1 index sub 2 1 roll\n"
    "  0 index 2 1 roll\n"
    "  pop\n"
    "}",
/* 2 axes */
    "{\n"
    "  1 2 index sub 1 2 index sub mul 3 1 roll\n"
    "  1 index 1 2 index sub mul 3 1 roll\n"
    "  1 2 index sub 1 index mul 3 1 roll\n"
    "  1 index 1 index mul 3 1 roll\n"
    "  pop pop\n"
    "}",
/* 3 axes */
    "{\n"
    "  1 3 index sub 1 3 index sub mul 1 3 index sub mul 4 1 roll\n"
    "  2 index 1 3 index sub mul 1 3 index sub mul 4 1 roll\n"
    "  1 3 index sub 2 index mul 1 3 index sub mul 4 1 roll\n"
    "  2 index 2 index mul 1 3 index sub mul 4 1 roll\n"
    "  1 3 index sub 1 3 index sub mul 2 index mul 4 1 roll\n"
    "  2 index 1 3 index sub mul 2 index mul 4 1 roll\n"
    "  1 3 index sub 2 index mul 2 index mul 4 1 roll\n"
    "  2 index 2 index mul 2 index mul 4 1 roll\n"
    "  pop pop pop\n"
    "}",
/* 4 axes */
/* This requires too big a string. We must build it at runtime */
    NULL
};
static char *cdv_4axis[3] = {
    "{\n"
    "  1 4 index sub 1 4 index sub mul 1 4 index sub 1 4 index sub mul 5 1 roll\n"
    "  3 index 1 4 index sub mul 1 4 index sub mul 1 4 index sub mul 5 1 roll\n"
    "  1 4 index sub 3 index mul 1 4 index sub mul 1 4 index sub mul 5 1 roll\n"
    "  3 index 3 index mul 1 4 index sub mul 1 4 index sub mul 5 1 roll\n"
    "  1 4 index sub 1 4 index sub mul 3 index mul 1 4 index sub mul 5 1 roll\n"
    "  3 index 1 4 index sub mul 3 index mul 1 4 index sub mul 5 1 roll\n",
    "  1 4 index sub 3 index mul 3 index mul 1 4 index sub mul 5 1 roll\n"
    "  3 index 3 index mul 3 index mul 1 4 index sub mul 5 1 roll\n"
    "  1 4 index sub 1 4 index sub mul 1 4 index sub 3 index mul 5 1 roll\n"
    "  3 index 1 4 index sub mul 1 4 index sub mul 3 index mul 5 1 roll\n"
    "  1 4 index sub 3 index mul 1 4 index sub mul 3 index mul 5 1 roll\n",
    "  3 index 3 index mul 1 4 index sub mul 3 index mul 5 1 roll\n"
    "  1 4 index sub 1 4 index sub mul 3 index mul 3 index mul 5 1 roll\n"
    "  3 index 1 4 index sub mul 3 index mul 3 index mul 5 1 roll\n"
    "  1 4 index sub 3 index mul 3 index mul 3 index mul 5 1 roll\n"
    "  3 index 3 index mul 3 index mul 3 index mul 5 1 roll\n"
    "  pop pop pop pop\n"
    "}"
};

static char *axistablab[] = { N_("Axis 1"), N_("Axis 2"), N_("Axis 3"), N_("Axis 4") };
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

char *MMAxisAbrev(char *axis_name) {
    if ( strcmp(axis_name,"Weight")==0 )
return( "wt" );
    if ( strcmp(axis_name,"Width")==0 )
return( "wd" );
    if ( strcmp(axis_name,"OpticalSize")==0 )
return( "op" );
    if ( strcmp(axis_name,"Slant")==0 )
return( "sl" );

return( axis_name );
}

static double MMAxisUnmap(MMSet *mm,int axis,double ncv) {
    struct axismap *axismap = &mm->axismaps[axis];
    int j;

    if ( ncv<=axismap->blends[0] )
return(axismap->designs[0]);

    for ( j=1; j<axismap->points; ++j ) {
	if ( ncv<=axismap->blends[j]) {
	    double t = (ncv-axismap->blends[j-1])/(axismap->blends[j]-axismap->blends[j-1]);
return( axismap->designs[j-1]+ t*(axismap->designs[j]-axismap->designs[j-1]) );
	}
    }

return(axismap->designs[axismap->points-1]);
}

static char *_MMMakeFontname(MMSet *mm,real *normalized,char **fullname) {
    char *pt, *pt2, *hyphen=NULL;
    char *ret = NULL;
    int i,j;

    if ( mm->apple ) {
	for ( i=0; i<mm->named_instance_count; ++i ) {
	    for ( j=0; j<mm->axis_count; ++j ) {
		if (( normalized[j] == -1 &&
			RealApprox(mm->named_instances[i].coords[j],mm->axismaps[j].min) ) ||
		    ( normalized[j] ==  0 &&
			RealApprox(mm->named_instances[i].coords[j],mm->axismaps[j].def) ) ||
		    ( normalized[j] ==  1 &&
			RealApprox(mm->named_instances[i].coords[j],mm->axismaps[j].max) ))
		    /* A match so far */;
		else
	    break;
	    }
	    if ( j==mm->axis_count )
	break;
	}
	if ( i!=mm->named_instance_count ) {
	    char *styles = PickNameFromMacName(mm->named_instances[i].names);
	    if ( styles==NULL )
		styles = FindEnglishNameInMacName(mm->named_instances[i].names);
	    if ( styles!=NULL ) {
		ret = galloc(strlen(mm->normal->familyname)+ strlen(styles)+3 );
		strcpy(ret,mm->normal->familyname);
		hyphen = ret+strlen(ret);
		strcpy(hyphen," ");
		strcpy(hyphen+1,styles);
		free(styles);
	    }
	}
    }

    if ( ret==NULL ) {
	pt = ret = galloc(strlen(mm->normal->familyname)+ mm->axis_count*15 + 1);
	strcpy(pt,mm->normal->familyname);
	pt += strlen(pt);
	*pt++ = '_';
	for ( i=0; i<mm->axis_count; ++i ) {
	    if ( !mm->apple )
		sprintf( pt, " %d%s", (int) rint(MMAxisUnmap(mm,i,normalized[i])),
			MMAxisAbrev(mm->axes[i]));
	    else
		sprintf( pt, " %.1f%s", MMAxisUnmap(mm,i,normalized[i]),
			MMAxisAbrev(mm->axes[i]));
	    pt += strlen(pt);
	}
	if ( pt>ret && pt[-1]==' ' )
	    --pt;
	*pt = '\0';
    }

    *fullname = ret;

    ret = copy(ret);
    for ( pt=*fullname, pt2=ret; *pt!='\0'; ++pt )
	if ( pt==hyphen )
	    *pt2++ = '-';
	else if ( *pt!=' ' )
	    *pt2++ = *pt;
    *pt2 = '\0';
return( ret );
}

char *MMMakeMasterFontname(MMSet *mm,int ipos,char **fullname) {
return( _MMMakeFontname(mm,&mm->positions[ipos*mm->axis_count],fullname));
}

static char *_MMGuessWeight(MMSet *mm,real *normalized,char *def) {
    int i;
    char *ret;
    real design;

    for ( i=0; i<mm->axis_count; ++i ) {
	if ( strcmp(mm->axes[i],"Weight")==0 )
    break;
    }
    if ( i==mm->axis_count )
return( def );
    design = MMAxisUnmap(mm,i,normalized[i]);
    if ( design<50 || design>1500 )	/* Er... Probably not the 0...1000 range I expect */
return( def );
    ret = NULL;
    if ( design<150 )
	ret = "Thin";
    else if ( design<350 )
	ret = "Light";
    else if ( design<550 )
	ret = "Medium";
    else if ( design<650 )
	ret = "DemiBold";
    else if ( design<750 )
	ret = "Bold";
    else if ( design<850 )
	ret = "Heavy";
    else
	ret = "Black";
    free( def );
return( copy(ret) );
}

char *MMGuessWeight(MMSet *mm,int ipos,char *def) {
return( _MMGuessWeight(mm,&mm->positions[ipos*mm->axis_count],def));
}

/* Given a postscript array of scalars, what's the ipos'th element? */
char *MMExtractNth(char *pt,int ipos) {
    int i;
    char *end;

    while ( *pt==' ' ) ++pt;
    if ( *pt=='[' ) ++pt;
    for ( i=0; *pt!=']' && *pt!='\0'; ++i ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt==']' || *pt=='\0' )
return( NULL );
	for ( end=pt; *end!=' ' && *end!=']' && *end!='\0'; ++end );
	if ( i==ipos )
return( copyn(pt,end-pt));
	pt = end;
    }
return( NULL );
}

/* Given a postscript array of arrays, such as those found in Blend Private BlueValues */
/* return the array composed of the ipos'th element of each sub-array */
char *MMExtractArrayNth(char *pt,int ipos) {
    char *hold[40], *ret;
    int i,j,len;

    while ( *pt==' ' ) ++pt;
    if ( *pt=='[' ) ++pt;
    i = 0;
    while ( *pt!=']' && *pt!=' ' ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='[' ) {
	    if ( i<sizeof(hold)/sizeof(hold[0]) )
		hold[i++] = MMExtractNth(pt,ipos);
	    ++pt;
	    while ( *pt!=']' && *pt!='\0' ) ++pt;
	}
	if ( *pt!='\0' )
	    ++pt;
    }
    if ( i==0 )
return( NULL );
    for ( j=len=0; j<i; ++j ) {
	if ( hold[j]==NULL ) {
	    for ( j=0; j<i; ++j )
		free(hold[j]);
return( NULL );
	}
	len += strlen( hold[j] )+1;
    }

    pt = ret = galloc(len+4);
    *pt++ = '[';
    for ( j=0; j<i; ++j ) {
	strcpy(pt,hold[j]);
	free(hold[j]);
	pt += strlen(pt);
	if ( j!=i-1 )
	    *pt++ = ' ';
    }
    *pt++ = ']';
    *pt++ = '\0';
return( ret );
}

void MMKern(SplineFont *sf,SplineChar *first,SplineChar *second,int diff,
	int sli,KernPair *oldkp) {
    MMSet *mm = sf->mm;
    KernPair *kp;
    int i;
    /* If the user creates a kern pair in one font of a multiple master set */
    /*  then we should create the same kern pair in all the others. Similarly */
    /*  if s/he modifies a kern pair in the weighted version then apply that */
    /*  mod to all the others */

    if ( mm==NULL )
return;
    if ( sf==mm->normal || oldkp==NULL ) {
	for ( i= -1; i<mm->instance_count; ++i ) {
	    SplineFont *cur = i==-1 ? mm->normal : mm->instances[i];
	    SplineChar *psc, *ssc;
	    if ( cur==sf )	/* Done in caller */
	continue;
	    psc = cur->glyphs[first->orig_pos];
	    ssc = cur->glyphs[second->orig_pos];
	    if ( psc==NULL || ssc==NULL )		/* Should never happen*/
	continue;
	    for ( kp = psc->kerns; kp!=NULL && kp->sc!=ssc; kp = kp->next );
	    /* No mm support for vertical kerns */
	    if ( kp==NULL ) {
		kp = chunkalloc(sizeof(KernPair));
		if ( oldkp!=NULL )
		    *kp = *oldkp;
		else {
		    kp->off = diff;
		    if ( sli==-1 )
			sli = SFFindBiggestScriptLangIndex(cur,
				    SCScriptFromUnicode(psc),DEFAULT_LANG);
		    kp->sli = sli;
		}
		kp->sc = ssc;
		kp->next = psc->kerns;
		psc->kerns = kp;
	    } else
		kp->off += diff;
	}
    }
}

static int ContourCount(SplineChar *sc) {
    SplineSet *spl;
    int i;

    for ( spl=sc->layers[ly_fore].splines, i=0; spl!=NULL; spl=spl->next, ++i );
return( i );
}

static int ContourPtMatch(SplineChar *sc1, SplineChar *sc2) {
    SplineSet *spl1, *spl2;
    SplinePoint *sp1, *sp2;

    for ( spl1=sc1->layers[ly_fore].splines, spl2=sc2->layers[ly_fore].splines; spl1!=NULL && spl2!=NULL; spl1=spl1->next, spl2=spl2->next ) {
	for ( sp1=spl1->first, sp2 = spl2->first; ; ) {
	    if ( sp1->nonextcp!=sp2->nonextcp || sp1->noprevcp!=sp2->noprevcp )
return( false );
	    if ( sp1->next==NULL || sp2->next==NULL ) {
		if ( sp1->next==NULL && sp2->next==NULL )
	break;
return( false );
	    }
	    sp1 = sp1->next->to; sp2 = sp2->next->to;
	    if ( sp1==spl1->first || sp2==spl2->first ) {
		if ( sp1==spl1->first && sp2==spl2->first )
	break;
return( false );
	    }
	}
    }
return( true );
}

static int ContourDirMatch(SplineChar *sc1, SplineChar *sc2) {
    SplineSet *spl1, *spl2;

    for ( spl1=sc1->layers[ly_fore].splines, spl2=sc2->layers[ly_fore].splines; spl1!=NULL && spl2!=NULL; spl1=spl1->next, spl2=spl2->next ) {
	if ( SplinePointListIsClockwise(spl1)!=SplinePointListIsClockwise(spl2) )
return( false );
    }
return( true );
}

static int ContourHintMaskMatch(SplineChar *sc1, SplineChar *sc2) {
    SplineSet *spl1, *spl2;
    SplinePoint *sp1, *sp2;

    for ( spl1=sc1->layers[ly_fore].splines, spl2=sc2->layers[ly_fore].splines; spl1!=NULL && spl2!=NULL; spl1=spl1->next, spl2=spl2->next ) {
	for ( sp1=spl1->first, sp2 = spl2->first; ; ) {
	    if ( (sp1->hintmask==NULL)!=(sp2->hintmask==NULL) )
return( false );
	    if ( sp1->hintmask!=NULL && memcmp(sp1->hintmask,sp2->hintmask,sizeof(HintMask))!=0 )
return( false );
	    if ( sp1->next==NULL || sp2->next==NULL ) {
		if ( sp1->next==NULL && sp2->next==NULL )
	break;
return( false );
	    }
	    sp1 = sp1->next->to; sp2 = sp2->next->to;
	    if ( sp1==spl1->first || sp2==spl2->first ) {
		if ( sp1==spl1->first && sp2==spl2->first )
	break;
return( false );
	    }
	}
    }
return( true );
}

static int RefMatch(SplineChar *sc1, SplineChar *sc2) {
    RefChar *ref1, *ref2;
    /* I don't require the reference list to be ordered */

    for ( ref1=sc1->layers[ly_fore].refs, ref2=sc2->layers[ly_fore].refs; ref1!=NULL && ref2!=NULL; ref1=ref1->next, ref2=ref2->next )
	ref2->checked = false;
    if ( ref1!=NULL || ref2!=NULL )
return( false );

    for ( ref1=sc1->layers[ly_fore].refs; ref1!=NULL ; ref1=ref1->next ) {
	for ( ref2=sc2->layers[ly_fore].refs; ref2!=NULL ; ref2=ref2->next ) {
	    if ( ref2->sc->orig_pos==ref1->sc->orig_pos && !ref2->checked )
	break;
	}
	if ( ref2==NULL )
return( false );
	ref2->checked = true;
    }

return( true );
}

static int RefTransformsMatch(SplineChar *sc1, SplineChar *sc2) {
    /* Apple only provides a means to change the translation of a reference */
    /*  so if rotation, skewing, scaling, etc. differ then we can't deal with */
    /*  it. */
    RefChar *r1 = sc1->layers[ly_fore].refs;
    RefChar *r2 = sc2->layers[ly_fore].refs;

    while ( r1!=NULL && r2!=NULL ) {
	if ( r1->transform[0]!=r2->transform[0] ||
		r1->transform[1]!=r2->transform[1] ||
		r1->transform[2]!=r2->transform[2] ||
		r1->transform[3]!=r2->transform[3] )
return( false );
	r1 = r1->next;
	r2 = r2->next;
    }
return( true );
}

static int HintsMatch(StemInfo *h1,StemInfo *h2) {
    while ( h1!=NULL && h2!=NULL ) {
#if 0		/* Nope. May conflict in one instance and not in another */
	if ( h1->hasconflicts != h2->hasconflicts )
return( false );
#endif
	h1 = h1->next;
	h2 = h2->next;
    }
    if ( h1!=NULL || h2!=NULL )
return( false );

return( true );
}

static int KernsMatch(SplineChar *sc1, SplineChar *sc2) {
    /* I don't require the kern list to be ordered */
    /* Only interested in kerns that go into afm files (ie. no kernclasses) */
    KernPair *k1, *k2;

    for ( k1=sc1->kerns, k2=sc2->kerns; k1!=NULL && k2!=NULL; k1=k1->next, k2=k2->next )
	k2->kcid = false;
    if ( k1!=NULL || k2!=NULL )
return( false );

    for ( k1=sc1->kerns; k1!=NULL ; k1=k1->next ) {
	for ( k2=sc2->kerns; k2!=NULL ; k2=k2->next ) {
	    if ( k2->sc->orig_pos==k1->sc->orig_pos && !k2->kcid )
	break;
	}
	if ( k2==NULL )
return( false );
	k2->kcid = true;
    }

return( true );
}

static int ArrayCount(char *val) {
    char *end;
    int cnt;

    if ( val==NULL )
return( 0 );
    while ( *val==' ' ) ++val;
    if ( *val=='[' ) ++val;
    cnt=0;
    while ( *val ) {
	strtod(val,&end);
	if ( val==end )
    break;
	++cnt;
	val = end;
    }
return( cnt );
}

int MMValid(MMSet *mm,int complain) {
    int i, j;
    SplineFont *sf;
    static char *arrnames[] = { "BlueValues", "OtherBlues", "FamilyBlues", "FamilyOtherBlues", "StdHW", "StdVW", "StemSnapH", "StemSnapV", NULL };

    if ( mm==NULL )
return( false );

    for ( i=0; i<mm->instance_count; ++i )
	if ( mm->instances[i]->order2 != mm->apple ) {
	    if ( complain ) {
		if ( mm->apple )
#if defined(FONTFORGE_CONFIG_GTK)
		    gwwv_post_error(_("Bad Multiple Master Font"),_("The font %.30s contains cubic splines. It must be converted to quadratic splines before it can be used in an apple distortable font"),
#else
		    gwwv_post_error(_("Bad Multiple Master Font"),_("The font %.30s contains cubic splines. It must be converted to quadratic splines before it can be used in an apple distortable font"),
#endif
			    mm->instances[i]->fontname);
		else
#if defined(FONTFORGE_CONFIG_GTK)
		    gwwv_post_error(_("Bad Multiple Master Font"),_("The font %.30s contains quadratic splines. It must be converted to cubic splines before it can be used in a multiple master"),
#else
		    gwwv_post_error(_("Bad Multiple Master Font"),_("The font %.30s contains quadratic splines. It must be converted to cubic splines before it can be used in a multiple master"),
#endif
			    mm->instances[i]->fontname);
	    }
return( false );
	}

    sf = mm->apple ? mm->normal : mm->instances[0];

    if ( !mm->apple && PSDictHasEntry(sf->private,"ForceBold")!=NULL &&
	    PSDictHasEntry(mm->normal->private,"ForceBoldThreshold")==NULL) {
	if ( complain )
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Bad Multiple Master Font"),_("There is no ForceBoldThreshold entry in the weighted font, but there is a ForceBold entry in font %30s"),
#else
	    gwwv_post_error(_("Bad Multiple Master Font"),_("There is no ForceBoldThreshold entry in the weighted font, but there is a ForceBold entry in font %30s"),
#endif
		    sf->fontname);
return( false );
    }

    for ( j=mm->apple ? 0 : 1; j<mm->instance_count; ++j ) {
	if ( sf->glyphcnt!=mm->instances[j]->glyphcnt ) {
	    if ( complain )
#if defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Bad Multiple Master Font"),_("The fonts %1$.30s and %2$.30s have a different number of glyphs or different encodings"),
#else
		gwwv_post_error(_("Bad Multiple Master Font"),_("The fonts %1$.30s and %2$.30s have a different number of glyphs or different encodings"),
#endif
			sf->fontname, mm->instances[j]->fontname);
return( false );
	} else if ( sf->order2!=mm->instances[j]->order2 ) {
	    if ( complain )
#if defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Bad Multiple Master Font"),_("The fonts %1$.30s and %2$.30s use different types of splines (one quadratic, one cubic)"),
#else
		gwwv_post_error(_("Bad Multiple Master Font"),_("The fonts %1$.30s and %2$.30s use different types of splines (one quadratic, one cubic)"),
#endif
			sf->fontname, mm->instances[j]->fontname);
return( false );
	}
	if ( !mm->apple ) {
	    if ( PSDictHasEntry(mm->instances[j]->private,"ForceBold")!=NULL &&
		    PSDictHasEntry(mm->normal->private,"ForceBoldThreshold")==NULL) {
		if ( complain )
#if defined(FONTFORGE_CONFIG_GTK)
		    gwwv_post_error(_("Bad Multiple Master Font"),_("There is no ForceBoldThreshold entry in the weighted font, but there is a ForceBold entry in font %30s"),
#else
		    gwwv_post_error(_("Bad Multiple Master Font"),_("There is no ForceBoldThreshold entry in the weighted font, but there is a ForceBold entry in font %30s"),
#endif
			    mm->instances[j]->fontname);
return( false );
	    }
	    for ( i=0; arrnames[i]!=NULL; ++i ) {
		if ( ArrayCount(PSDictHasEntry(mm->instances[j]->private,arrnames[i]))!=
				ArrayCount(PSDictHasEntry(sf->private,arrnames[i])) ) {
		    if ( complain )
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The entry \"%1$.20s\" is not present in the private dictionary of both %2$.30s and %3$.30s"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The entry \"%1$.20s\" is not present in the private dictionary of both %2$.30s and %3$.30s"),
#endif
				arrnames[i], sf->fontname, mm->instances[j]->fontname);
return( false );
		}
	    }
	}
    }

    for ( i=0; i<sf->glyphcnt; ++i ) {
	for ( j=mm->apple?0:1; j<mm->instance_count; ++j ) {
	    if ( SCWorthOutputting(sf->glyphs[i])!=SCWorthOutputting(mm->instances[j]->glyphs[i]) ) {
		if ( complain ) {
		    FVChangeChar(sf->fv,i);
		    if ( SCWorthOutputting(sf->glyphs[i]) )
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s is defined in font %2$.30s but not in %3$.30s"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s is defined in font %2$.30s but not in %3$.30s"),
#endif
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    else
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s is defined in font %2$.30s but not in %3$.30s"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s is defined in font %2$.30s but not in %3$.30s"),
#endif
				mm->instances[j]->glyphs[i]->name, mm->instances[j]->fontname,sf->fontname);
		}
return( false );
	    }
	}
	if ( SCWorthOutputting(sf->glyphs[i]) ) {
	    if ( mm->apple && sf->glyphs[i]->layers[ly_fore].refs!=NULL && sf->glyphs[i]->layers[ly_fore].splines!=NULL ) {
		if ( complain ) {
		    FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
		    gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in %2$.30s has both references and contours. This is not supported in a font with variations"),
#else
		    gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in %2$.30s has both references and contours. This is not supported in a font with variations"),
#endif
			    sf->glyphs[i]->name,sf->fontname);
		}
return( false );
	    }
	    for ( j=mm->apple?0:1; j<mm->instance_count; ++j ) {
		if ( mm->apple && mm->instances[j]->glyphs[i]->layers[ly_fore].refs!=NULL &&
			mm->instances[j]->glyphs[i]->layers[ly_fore].splines!=NULL ) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in %2$.30s has both references and contours. This is not supported in a font with variations"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in %2$.30s has both references and contours. This is not supported in a font with variations"),
#endif
				sf->glyphs[i]->name,mm->instances[j]->fontname);
		    }
return( false );
		}
		if ( ContourCount(sf->glyphs[i])!=ContourCount(mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s has a different number of contours in font %2$.30s than in %3$.30s"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s has a different number of contours in font %2$.30s than in %3$.30s"),
#endif
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !mm->apple && !ContourPtMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different number of points (or control points) on its contours than in %3$.30s"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different number of points (or control points) on its contours than in %3$.30s"),
#endif
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !ContourDirMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has contours running in a different direction than in %3$.30s"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has contours running in a different direction than in %3$.30s"),
#endif
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !RefMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different number of references than in %3$.30s"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different number of references than in %3$.30s"),
#endif
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( mm->apple && !RefTransformsMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has references with different scaling or rotation (etc.) than in %3$.30s"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has references with different scaling or rotation (etc.) than in %3$.30s"),
#endif
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !mm->apple && !KernsMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different set of kern pairs than in %3$.30s"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different set of kern pairs than in %3$.30s"),
#endif
				"vertical", sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		}
	    }
	    if ( mm->apple && !ContourPtNumMatch(mm,i)) {
		if ( complain ) {
		    FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
		    gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s has a different numbering of points (and control points) on its contours than in the various instances of the font"),
#else
		    gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s has a different numbering of points (and control points) on its contours than in the various instances of the font"),
#endif
			    sf->glyphs[i]->name);
		}
return( false );
	    }
	    if ( !mm->apple ) {
		for ( j=1; j<mm->instance_count; ++j ) {
		    if ( !HintsMatch(sf->glyphs[i]->hstem,mm->instances[j]->glyphs[i]->hstem)) {
			if ( complain ) {
			    FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			    gwwv_post_error(_("Bad Multiple Master Font"),_("The %1$s hints in glyph \"%2$.30s\" in font %3$.30s do not match those in %4$.30s (different number or different overlap criteria)"),
#else
			    gwwv_post_error(_("Bad Multiple Master Font"),_("The %1$s hints in glyph \"%2$.30s\" in font %3$.30s do not match those in %4$.30s (different number or different overlap criteria)"),
#endif
				    "horizontal", sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
			}
return( false );
		    } else if ( !HintsMatch(sf->glyphs[i]->vstem,mm->instances[j]->glyphs[i]->vstem)) {
			if ( complain ) {
			    FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			    gwwv_post_error(_("Bad Multiple Master Font"),_("The %1$s hints in glyph \"%2$.30s\" in font %3$.30s do not match those in %4$.30s (different number or different overlap criteria)"),
#else
			    gwwv_post_error(_("Bad Multiple Master Font"),_("The %1$s hints in glyph \"%2$.30s\" in font %3$.30s do not match those in %4$.30s (different number or different overlap criteria)"),
#endif
				    "vertical", sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
			}
return( false );
		    }
		}
		for ( j=1; j<mm->instance_count; ++j ) {
		    if ( !ContourHintMaskMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
			if ( complain ) {
			    FVChangeChar(sf->fv,i);
#if defined(FONTFORGE_CONFIG_GTK)
			    gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different hint mask on its contours than in %3$.30s"),
#else
			    gwwv_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different hint mask on its contours than in %3$.30s"),
#endif
				    sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
			}
return( false );
		    }
		}
	    }
	}
    }
    if ( mm->apple ) {
	struct ttf_table *cvt;
	for ( cvt = mm->normal->ttf_tables; cvt!=NULL && cvt->tag!=CHR('c','v','t',' '); cvt=cvt->next );
	if ( cvt==NULL ) {
	    for ( j=0; j<mm->instance_count; ++j ) {
		if ( mm->instances[j]->ttf_tables!=NULL ) {
		    if ( complain )
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The default font does not have a 'cvt ' table, but the instance %.30s does"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The default font does not have a 'cvt ' table, but the instance %.30s does"),
#endif
				mm->instances[j]->fontname);
return( false );
		}
	    }
	} else {
	    /* Not all instances are required to have cvts, but any that do */
	    /*  must be the same size */
	    for ( j=0; j<mm->instance_count; ++j ) {
		if ( mm->instances[j]->ttf_tables!=NULL &&
			(mm->instances[j]->ttf_tables->next!=NULL ||
			 mm->instances[j]->ttf_tables->tag!=CHR('c','v','t',' '))) {
		    if ( complain )
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("Instance fonts may only contain a 'cvt ' table, but %.30s has some other truetype table as well"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("Instance fonts may only contain a 'cvt ' table, but %.30s has some other truetype table as well"),
#endif
				mm->instances[j]->fontname);
return( false );
		}
		if ( mm->instances[j]->ttf_tables!=NULL &&
			mm->instances[j]->ttf_tables->len!=cvt->len ) {
		    if ( complain )
#if defined(FONTFORGE_CONFIG_GTK)
			gwwv_post_error(_("Bad Multiple Master Font"),_("The 'cvt ' table in instance %.30s is a different size from that in the default font"),
#else
			gwwv_post_error(_("Bad Multiple Master Font"),_("The 'cvt ' table in instance %.30s is a different size from that in the default font"),
#endif
				mm->instances[j]->fontname);
return( false );
		}
	    }
	}
    }

    if ( complain )
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_notice(_("OK"),_("No problems detected"));
#else
	gwwv_post_notice(_("OK"),_("No problems detected"));
#endif
return( true );
}

static SplineChar *SFMakeGlyphLike(SplineFont *sf, int gid, SplineFont *base) {
    SplineChar *sc = SplineCharCreate(), *bsc = base->glyphs[gid];
    sc->orig_pos = gid;
    sf->glyphs[gid] = sc;
    sc->parent = sf;

    sc->width = bsc->width; sc->widthset = true; sc->vwidth = bsc->vwidth;
    free(sc->name); sc->name = copy(bsc->name);
    sc->unicodeenc = bsc->unicodeenc;
return( bsc );
}

static char *_MMBlendChar(MMSet *mm, int gid) {
    int i, j, worthit = -1;
    int all, any, any2, all2, anyend, allend, diff;
    SplineChar *sc;
    SplinePointList *spls[MmMax], *spl, *spllast;
    SplinePoint *tos[MmMax], *to;
    RefChar *refs[MmMax], *ref, *reflast;
    KernPair *kp0, *kptest, *kp, *kplast;
    StemInfo *hs[MmMax], *h, *hlast;
    real width;

    for ( i=0; i<mm->instance_count; ++i ) {
	if ( mm->instances[i]->order2 )
return( _("One of the multiple master instances contains quadratic splines. It must be converted to cubic splines before it can be used in a multiple master") );
	if ( gid>=mm->instances[i]->glyphcnt )
return( _("The different instances of this mm have a different number of glyphs") );
	if ( SCWorthOutputting(mm->instances[i]->glyphs[gid]) ) {
	    if ( worthit == -1 ) worthit = true;
	    else if ( worthit != true )
return( _("This glyph is defined in one instance font but not in another") );
	} else {
	    if ( worthit == -1 ) worthit = false;
	    else if ( worthit != false )
return( _("This glyph is defined in one instance font but not in another") );
	}
    }

    sc = mm->normal->glyphs[gid];
    if ( sc!=NULL ) {
	SCClearContents(sc);
	KernPairsFree(sc->kerns);
	sc->kerns = NULL;
	KernPairsFree(sc->vkerns);
	sc->vkerns = NULL;
    }

    if ( !worthit )
return( 0 );

    if ( sc==NULL )
	sc = SFMakeGlyphLike(mm->normal,gid,mm->instances[0]);

	/* Blend references => blend transformation matrices */
    diff = false;
    any = false; all = true;
    for ( i=0; i<mm->instance_count; ++i ) {
	refs[i] = mm->instances[i]->glyphs[gid]->layers[ly_fore].refs;
	if ( refs[i]!=NULL ) any = true;
	else all = false;
    }
    reflast = NULL;
    while ( all ) {
	ref = RefCharCreate();
	*ref = *refs[0];
	ref->layers[0].splines = NULL;
	ref->next = NULL;
	memset(ref->transform,0,sizeof(ref->transform));
	ref->sc = mm->normal->glyphs[refs[0]->sc->orig_pos];
	for ( i=0; i<mm->instance_count; ++i ) {
	    if ( ref->sc->orig_pos!=refs[i]->sc->orig_pos )
		diff = true;
	    for ( j=0; j<6; ++j )
		ref->transform[j] += refs[i]->transform[j]*mm->defweights[i];
	}
	if ( reflast==NULL )
	    sc->layers[ly_fore].refs = ref;
	else
	    reflast->next = ref;
	reflast = ref;
	any = false; all = true;
	for ( i=0; i<mm->instance_count; ++i ) {
	    refs[i] = refs[i]->next;
	    if ( refs[i]!=NULL ) any = true;
	    else all = false;
	}
    }
    if ( any )
return( _("This glyph contains a different number of references in different instances") );
    if ( diff )
return( _("A reference in this glyph refers to a different encoding in different instances") );

	/* Blend Width */
    width = 0;
    for ( i=0; i<mm->instance_count; ++i )
	width += mm->instances[i]->glyphs[gid]->width*mm->defweights[i];
    sc->width = width;
    width = 0;
    for ( i=0; i<mm->instance_count; ++i )
	width += mm->instances[i]->glyphs[gid]->vwidth*mm->defweights[i];
    sc->vwidth = width;

	/* Blend Splines */
    any = false; all = true;
    for ( i=0; i<mm->instance_count; ++i ) {
	spls[i] = mm->instances[i]->glyphs[gid]->layers[ly_fore].splines;
	if ( spls[i]!=NULL ) any = true;
	else all = false;
    }
    spllast = NULL;
    while ( all ) {
	spl = chunkalloc(sizeof(SplinePointList));
	if ( spllast==NULL )
	    sc->layers[ly_fore].splines = spl;
	else
	    spllast->next = spl;
	spllast = spl;
	for ( i=0; i<mm->instance_count; ++i )
	    tos[i] = spls[i]->first;
	all2 = true;
	spl->last = NULL;
	while ( all2 ) {
	    to = chunkalloc(sizeof(SplinePoint));
	    to->nonextcp = tos[0]->nonextcp;
	    to->noprevcp = tos[0]->noprevcp;
	    to->nextcpdef = tos[0]->nextcpdef;
	    to->prevcpdef = tos[0]->prevcpdef;
	    to->pointtype = tos[0]->pointtype;
	    if ( tos[0]->hintmask!=NULL ) {
		to->hintmask = chunkalloc(sizeof(HintMask));
		memcpy(to->hintmask,tos[0]->hintmask,sizeof(HintMask));
	    }
	    for ( i=0; i<mm->instance_count; ++i ) {
		to->me.x += tos[i]->me.x*mm->defweights[i];
		to->me.y += tos[i]->me.y*mm->defweights[i];
		to->nextcp.x += tos[i]->nextcp.x*mm->defweights[i];
		to->nextcp.y += tos[i]->nextcp.y*mm->defweights[i];
		to->prevcp.x += tos[i]->prevcp.x*mm->defweights[i];
		to->prevcp.y += tos[i]->prevcp.y*mm->defweights[i];
	    }
	    if ( spl->last==NULL )
		spl->first = to;
	    else
		SplineMake3(spl->last,to);
	    spl->last = to;
	    any2 = false; all2 = true;
	    for ( i=0; i<mm->instance_count; ++i ) {
		if ( tos[i]->next==NULL ) tos[i] = NULL;
		else tos[i] = tos[i]->next->to;
		if ( tos[i]!=NULL ) any2 = true;
		else all2 = false;
	    }
	    if ( !all2 && any2 )
return( _("A contour in this glyph contains a different number of points in different instances") );
	    anyend = false; allend = true;
	    for ( i=0; i<mm->instance_count; ++i ) {
		if ( tos[i]==spls[i]->first ) anyend = true;
		else allend = false;
	    }
	    if ( allend ) {
		SplineMake3(spl->last,spl->first);
		spl->last = spl->first;
	break;
	    }
	    if ( anyend )
return( _("A contour in this glyph contains a different number of points in different instances") );
	}
	any = false; all = true;
	for ( i=0; i<mm->instance_count; ++i ) {
	    spls[i] = spls[i]->next;
	    if ( spls[i]!=NULL ) any = true;
	    else all = false;
	}
    }
    if ( any )
return( _("This glyph contains a different number of contours in different instances") );

	/* Blend hints */
    for ( j=0; j<2; ++j ) {
	any = false; all = true;
	for ( i=0; i<mm->instance_count; ++i ) {
	    hs[i] = j ? mm->instances[i]->glyphs[gid]->hstem : mm->instances[i]->glyphs[gid]->vstem;
	    if ( hs[i]!=NULL ) any = true;
	    else all = false;
	}
	hlast = NULL;
	while ( all ) {
	    h = chunkalloc(sizeof(StemInfo));
	    *h = *hs[0];
	    h->where = NULL;
	    h->next = NULL;
	    h->start = h->width = 0;
	    for ( i=0; i<mm->instance_count; ++i ) {
		h->start += hs[i]->start*mm->defweights[i];
		h->width += hs[i]->width*mm->defweights[i];
	    }
	    if ( hlast!=NULL )
		hlast->next = h;
	    else if ( j )
		sc->hstem = h;
	    else
		sc->vstem = h;
	    hlast = h;
	    any = false; all = true;
	    for ( i=0; i<mm->instance_count; ++i ) {
		hs[i] = hs[i]->next;
		if ( hs[i]!=NULL ) any = true;
		else all = false;
	    }
	}
	if ( any )
return( _("This glyph contains a different number of hints in different instances") );
    }

	/* Blend kernpairs */
    /* I'm not requiring ordered kerning pairs */
    /* I'm not bothering with vertical kerning */
    kp0 = mm->instances[0]->glyphs[gid]->kerns;
    kplast = NULL;
    while ( kp0!=NULL ) {
	int off = kp0->off*mm->defweights[0];
	for ( i=1; i<mm->instance_count; ++i ) {
	    for ( kptest=mm->instances[i]->glyphs[gid]->kerns; kptest!=NULL; kptest=kptest->next )
		if ( kptest->sc->orig_pos==kp0->sc->orig_pos )
	    break;
	    if ( kptest==NULL )
return( _("This glyph contains different kerning pairs in different instances") );
	    off += kptest->off*mm->defweights[i];
	}
	kp = chunkalloc(sizeof(KernPair));
	kp->sc = mm->normal->glyphs[kp0->sc->orig_pos];
	kp->off = off;
	kp->sli = kp0->sli;
	kp->flags = kp0->flags;
	if ( kplast!=NULL )
	    kplast->next = kp;
	else
	    sc->kerns = kp;
	kplast = kp;
	kp0 = kp0->next;
    }
return( 0 );
}

char *MMBlendChar(MMSet *mm, int gid) {
    char *ret;
    RefChar *ref;

    if ( gid>=mm->normal->glyphcnt )
return( _("The different instances of this mm have a different number of glyphs") );
    ret = _MMBlendChar(mm,gid);
    if ( mm->normal->glyphs[gid]!=NULL ) {
	SplineChar *sc = mm->normal->glyphs[gid];
	for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	    SCReinstanciateRefChar(sc,ref);
	    SCMakeDependent(sc,ref->sc);
	}
    }
return(ret);
}

static struct psdict *BlendPrivate(struct psdict *private,MMSet *mm) {
    struct psdict *other;
    real sum, val;
    char *data;
    int i,j,k, cnt;
    char *values[MmMax], buffer[32], *space, *pt, *end;

    other = mm->instances[0]->private;
    if ( other==NULL )
return( private );

    if ( private==NULL )
	private = gcalloc(1,sizeof(struct psdict));

    i = PSDictFindEntry(private,"ForceBoldThreshold");
    if ( i!=-1 ) {
	val = strtod(private->values[i],NULL);
	sum = 0;
	for ( j=0; j<mm->instance_count; ++j ) {
	    i = PSDictFindEntry(mm->instances[j]->private,"ForceBold");
	    if ( i!=-1 && strcmp(mm->instances[j]->private->values[i],"true")==0 )
		sum += mm->defweights[j];
	}
	data = ( sum>=val ) ? "true" : "false";
	PSDictChangeEntry(private,"ForceBold",data);
    }
    for ( i=0; i<other->next; ++i ) {
	if ( *other->values[i]!='[' && !isdigit( *other->values[i]) && *other->values[i]!='.' )
    continue;
	for ( j=0; j<mm->instance_count; ++j ) {
	    k = PSDictFindEntry(mm->instances[j]->private,other->keys[i]);
	    if ( k==-1 )
	break;
	    values[j] = mm->instances[j]->private->values[k];
	}
	if ( j!=mm->instance_count )
    continue;
	if ( *other->values[i]!='[' ) {
	    /* blend a real number */
	    sum = 0;
	    for ( j=0; j<mm->instance_count; ++j ) {
		val = strtod(values[j],&end);
		if ( end==values[j])
	    break;
		sum += val*mm->defweights[j];
	    }
	    if ( j!=mm->instance_count )
    continue;
	    sprintf(buffer,"%g",(double) sum);
	    PSDictChangeEntry(private,other->keys[i],buffer);
	} else {
	    /* Blend an array of numbers */
	    for ( pt = values[0], cnt=0; *pt!='\0' && *pt!=']'; ++pt ) {
		if ( *pt==' ' ) {
		    while ( *pt==' ' ) ++pt;
		    --pt;
		    ++cnt;
		}
	    }
	    space = pt = galloc((cnt+2)*24+4);
	    *pt++ = '[';
	    for ( j=0; j<mm->instance_count; ++j )
		if ( *values[j]=='[' ) ++values[j];
	    while ( *values[0]!=']' ) {
		sum = 0;
		for ( j=0; j<mm->instance_count; ++j ) {
		    val = strtod(values[j],&end);
		    sum += val*mm->defweights[j];
		    while ( *end==' ' ) ++end;
		    values[j] = end;
		}
		sprintf( pt,"%g ", (double) sum);
		pt += strlen(pt);
	    }
	    if ( pt[-1]==' ' ) --pt;
	    *pt++ = ']';
	    *pt = '\0';
	    PSDictChangeEntry(private,other->keys[i],space);
	    free(space);
	}
    }

return( private );
}

int MMReblend(FontView *fv, MMSet *mm) {
    char *olderr, *err;
    int i, first = -1;
    SplineFont *sf = mm->instances[0];
    RefChar *ref;

    olderr = NULL;
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( i>=mm->normal->glyphcnt )
    break;
	err = MMBlendChar(mm,i);
	if ( mm->normal->glyphs[i]!=NULL )
	    _SCCharChangedUpdate(mm->normal->glyphs[i],-1);
	if ( err==NULL )
    continue;
	if ( olderr==NULL ) {
	    if ( fv!=NULL )
		FVDeselectAll(fv);
	    first = i;
	}
	if ( olderr==NULL || olderr == err )
	    olderr = err;
	else
	    olderr = (char *) -1;
	if ( fv!=NULL ) {
	    int enc = fv->map->backmap[i];
	    if ( enc!=-1 )
		fv->selected[enc] = true;
	}
    }

    sf = mm->normal;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( ref=sf->glyphs[i]->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	    SCReinstanciateRefChar(sf->glyphs[i],ref);
	    SCMakeDependent(sf->glyphs[i],ref->sc);
	}
    }
    sf->private = BlendPrivate(sf->private,mm);

    if ( olderr == NULL )	/* No Errors */
return( true );

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( fv!=NULL ) {
	FVScrollToChar(fv,first);
	if ( olderr==(char *) -1 )
	    gwwv_post_error(_("Bad Multiple Master Font"),_("Various errors occurred at the selected glyphs"));
	else
	    gwwv_post_error(_("Bad Multiple Master Font"),_("The following error occurred on the selected glyphs: %.100s"),olderr);
#endif
    }
return( false );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int ExecConvertDesignVector(real *designs, int dcnt, char *ndv, char *cdv,
	real *stack) {
    char *temp, dv[101];
    int j, len, cnt;
    char *oldloc;

    /* PostScript parses things in "C" locale too */
    oldloc = setlocale(LC_NUMERIC,"C");
    len = 0;
    for ( j=0; j<dcnt; ++j ) {
	sprintf(dv+len, "%g ", (double) designs[j]);
	len += strlen(dv+len);
    }
    setlocale(LC_NUMERIC,oldloc);

    temp = galloc(len+strlen(ndv)+strlen(cdv)+20);
    strcpy(temp,dv);
    /*strcpy(temp+len++," ");*/		/* dv always will end in a space */

    while ( isspace(*ndv)) ++ndv;
    if ( *ndv=='{' )
	++ndv;
    strcpy(temp+len,ndv);
    len += strlen(temp+len);
    while ( len>0 && (temp[len-1]==' '||temp[len-1]=='\n') ) --len;
    if ( len>0 && temp[len-1]=='}' ) --len;

    while ( isspace(*cdv)) ++cdv;
    if ( *cdv=='{' )
	++cdv;
    strcpy(temp+len,cdv);
    len += strlen(temp+len);
    while ( len>0 && (temp[len-1]==' '||temp[len-1]=='\n') ) --len;
    if ( len>0 && temp[len-1]=='}' ) --len;

    cnt = EvaluatePS(temp,stack,MmMax);
    free(temp);
return( cnt );
}

static int StandardPositions(MMSet *mm,int instance_count, int axis_count,int isapple) {
    int i,j,factor,k,v;

    if ( !isapple ) {
	for ( i=0; i<instance_count; ++i ) {
	    for ( j=0; j<axis_count; ++j )
		if ( mm->positions[i*mm->axis_count+j]!= ( (i&(1<<j)) ? 1 : 0 ))
    return( false );
	}
    } else {
	for ( i=0; i<instance_count; ++i ) {
	    factor = 1;
	    for ( j=0; j<axis_count; ++j ) {
		k = i==instance_count-1 ? (instance_count-1)/2 :
			    i>=(instance_count-1)/2 ? i+1 : i;
		v = (i/factor)%3 -1;
		if ( mm->positions[i*mm->axis_count+j]!= v )
return( false );
		factor *= 3;
	    }
	}
    }
return( true );
}

static int OrderedPositions(MMSet *mm,int instance_count, int isapple) {
    /* For a 1 axis system, check that the positions are ordered */
    int i;

    if ( mm->positions[0]!=isapple?-1:0 )		/* must start at 0 */
return( false );
    if ( mm->positions[(instance_count-1)*4]!=1 )	/* and end at 1 */
return( false );
    for ( i=1; i<mm->instance_count; ++i )
	if ( mm->positions[i*mm->axis_count]<=mm->positions[(i-1)*mm->axis_count] )
return( false );

return( true );
}

static void MMWeightsUnMap(real weights[MmMax], real axiscoords[4],
	int axis_count) {

    if ( axis_count==1 )
	axiscoords[0] = weights[1];
    else if ( axis_count==2 ) {
	axiscoords[0] = weights[3]+weights[1];
	axiscoords[1] = weights[3]+weights[2];
    } else if ( axis_count==3 ) {
	axiscoords[0] = weights[7]+weights[5]+weights[3]+weights[1];
	axiscoords[1] = weights[7]+weights[6]+weights[3]+weights[2];
	axiscoords[2] = weights[7]+weights[6]+weights[5]+weights[4];
    } else {
	axiscoords[0] = weights[15]+weights[13]+weights[11]+weights[9]+
			weights[7]+weights[5]+weights[3]+weights[1];
	axiscoords[1] = weights[15]+weights[14]+weights[11]+weights[10]+
			weights[7]+weights[6]+weights[3]+weights[2];
	axiscoords[2] = weights[15]+weights[14]+weights[13]+weights[12]+
			weights[7]+weights[6]+weights[5]+weights[4];
	axiscoords[3] = weights[15]+weights[14]+weights[13]+weights[12]+
			weights[11]+weights[10]+weights[9]+weights[8];
    }
}

static unichar_t *MMDesignCoords(MMSet *mm) {
    char buffer[80], *pt;
    int i;
    real axiscoords[4];

    if ( mm->instance_count!=(1<<mm->axis_count) ||
	    !StandardPositions(mm,mm->instance_count,mm->axis_count,false))
return( uc_copy(""));
    MMWeightsUnMap(mm->defweights,axiscoords,mm->axis_count);
    pt = buffer;
    for ( i=0; i<mm->axis_count; ++i ) {
	sprintf( pt,"%g ", MMAxisUnmap(mm,i,axiscoords[i]));
	pt += strlen(pt);
    }
    pt[-1] = ' ';
return( uc_copy( buffer ));
}

static SplineFont *_MMNewFont(MMSet *mm,int index,char *familyname,real *normalized) {
    SplineFont *sf, *base;
    char *pt1, *pt2;
    int i;

    sf = SplineFontNew();
    sf->order2 = mm->apple;
    free(sf->fontname); free(sf->familyname); free(sf->fullname); free(sf->weight);
    sf->familyname = copy(familyname);
    if ( index==-1 ) {
	sf->fontname = copy(familyname);
	for ( pt1=pt2=sf->fontname; *pt1; ++pt1 )
	    if ( *pt1!=' ' )
		*pt2++ = *pt1;
	*pt2='\0';
	sf->fullname = copy(familyname);
    } else
	sf->fontname = _MMMakeFontname(mm,normalized,&sf->fullname);
    sf->weight = copy( "All" );

    base = NULL;
    if ( mm->normal!=NULL )
	base = mm->normal;
    else {
	for ( i=0; i<mm->instance_count; ++i )
	    if ( mm->instances[i]!=NULL ) {
		base = mm->instances[i];
	break;
	}
    }

    if ( base!=NULL ) {
	free(sf->xuid);
	sf->xuid = copy(base->xuid);
	free(sf->glyphs);
	sf->glyphs = gcalloc(base->glyphcnt,sizeof(SplineChar *));
	sf->glyphcnt = sf->glyphmax = base->glyphcnt;
	sf->new = base->new;
	sf->ascent = base->ascent;
	sf->descent = base->descent;
	free(sf->origname);
	sf->origname = copy(base->origname);
	if ( index<0 ) {
	    free(sf->copyright);
	    sf->copyright = copy(base->copyright);
	}
	/* Make sure we get the encoding exactly right */
	for ( i=0; i<base->glyphcnt; ++i ) if ( base->glyphs[i]!=NULL )
	    SFMakeGlyphLike(sf,i,base);
    }
    sf->onlybitmaps = false;
    sf->mm = mm;
return( sf );
}

static SplineFont *MMNewFont(MMSet *mm,int index,char *familyname) {
return( _MMNewFont(mm,index,familyname,&mm->positions[index*mm->axis_count]));
}

static real DoDelta(int16 **deltas,int pt,int is_y,real *blends,int instance_count) {
    real diff = 0;
    int j;

    for ( j=0; j<instance_count; ++j ) {
	if ( blends[j]!=0 && deltas[2*j+is_y]!=NULL )
	    diff += blends[j]*deltas[2*j+is_y][pt];
    }
return( diff );
}

static void DistortChar(SplineFont *sf,MMSet *mm,int gid,real *blends) {
    int i,j,ptcnt;
    int16 **deltas;
    SplineSet *ss;
    SplinePoint *sp;
    RefChar *ref;
    SplineChar *sc = sf->glyphs[gid];
    Spline *s, *first;

    if ( sc==NULL )
return;
    deltas = SCFindDeltas(mm,gid,&ptcnt);
    if ( deltas==NULL )
return;
    /* I never delta the left side bearing or top */
    sc->width += DoDelta(deltas,ptcnt-3,0,blends,mm->instance_count);
    sc->vwidth += DoDelta(deltas,ptcnt-1,1,blends,mm->instance_count);
    if ( sc->layers[ly_fore].refs!=NULL ) {
	for ( i=0,ref = sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next, ++i ) {
	    ref->transform[4] += DoDelta(deltas,i,0,blends,mm->instance_count);
	    ref->transform[5] += DoDelta(deltas,i,1,blends,mm->instance_count);
	}
    } else {
	for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	    for ( sp=ss->first;; ) {
		if ( sp->ttfindex!=0xffff ) {
		    sp->me.x += DoDelta(deltas,sp->ttfindex,0,blends,mm->instance_count);
		    sp->me.y += DoDelta(deltas,sp->ttfindex,1,blends,mm->instance_count);
		}
		if ( sp->nextcpindex!=0xffff ) {
		    sp->nextcp.x += DoDelta(deltas,sp->nextcpindex,0,blends,mm->instance_count);
		    sp->nextcp.y += DoDelta(deltas,sp->nextcpindex,1,blends,mm->instance_count);
		} else
		    sp->nextcp = sp->me;
		if ( sp->next!=NULL )
		    sp->next->to->prevcp = sp->nextcp;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	    for ( sp=ss->first;; ) {
		if ( sp->ttfindex==0xffff ) {
		    sp->me.x = (sp->prevcp.x+sp->nextcp.x)/2;
		    sp->me.y = (sp->prevcp.y+sp->nextcp.y)/2;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	    first = NULL;
	    for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
		SplineRefigure(s);
		if ( first==NULL ) first = s;
	    }
	}
    }
    for ( j=0; j<2*mm->instance_count; ++j )
	free( deltas[j]);
    free(deltas);
}

static void DistortCvt(struct ttf_table *cvt,MMSet *mm,real *blends) {
    int i,j,ptcnt;
    real diff;
    int16 **deltas;

    deltas = CvtFindDeltas(mm,&ptcnt);
    if ( deltas==NULL )
return;
    for ( i=0; i<ptcnt; ++i ) {
	diff = 0;
	for ( j=0; j<mm->instance_count; ++j ) {
	    if ( blends[j]!=0 && deltas[j]!=NULL )
		diff += blends[j]*deltas[j][i];
	}
	memputshort(cvt->data,2*i,memushort(cvt->data,cvt->len,2*i)+rint(diff));
    }
    for ( j=0; j<mm->instance_count; ++j )
	free( deltas[j]);
    free(deltas);
}

static void MakeAppleBlend(MMSet *mm,real *blends,real *normalized) {
    SplineFont *base = mm->normal;
    SplineFont *sf = _MMNewFont(mm,-2,base->familyname,normalized);
    int i;
    struct ttf_table *tab, *cvt=NULL, *last=NULL, *cur;
    RefChar *ref;

    sf->mm = NULL;
    for ( i=0; i<base->glyphcnt && i<sf->glyphcnt; ++i ) if ( base->glyphs[i]!=NULL ) {
	sf->glyphs[i] = SplineCharCopy(base->glyphs[i],base);
	for ( ref=sf->glyphs[i]->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
	    ref->sc = NULL;
	sf->glyphs[i]->orig_pos = i;
	DistortChar(sf,mm,i,blends);
    }
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	ttfFixupRef(sf->glyphs,i);
    for ( tab=base->ttf_tables; tab!=NULL; tab=tab->next ) {
	cur = chunkalloc(sizeof(struct ttf_table));
	cur->tag = tab->tag;
	cur->len = tab->len;
	cur->data = galloc(tab->len);
	memcpy(cur->data,tab->data,tab->len);
	if ( cur->tag==CHR('c','v','t',' '))
	    cvt = cur;
	if ( last==NULL )
	    sf->ttf_tables = cur;
	else
	    last->next = cur;
	last = cur;
    }
    if ( cvt!=NULL )
	DistortCvt(cvt,mm,blends);
    /* I don't know how to blend kerns */
    /* Apple's Skia has 5 kerning classes (one for the base font and one for */
    /*  some of the instances) and the classes have different glyph classes */
    /* I can't make a kern class out of them. I suppose I could generate a bunch */
    /*  of kern pairs, but ug. */
    /* Nor is it clear whether the kerning info is a delta or not */

    sf->changed = true;
    FontViewCreate(sf);
}

struct mmcb {
    int done;
    GWindow gw;
    MMSet *mm;
    FontView *fv;
    int tonew;
};

#define CID_Explicit		6001
#define	CID_ByDesign		6002
#define CID_NewBlends		6003
#define CID_NewDesign		6004
#define CID_Knowns		6005

static GTextInfo *MMCB_KnownValues(MMSet *mm) {
    GTextInfo *ti = gcalloc(mm->named_instance_count+2,sizeof(GTextInfo));
    int i;

    ti[0].text = uc_copy(" --- ");
    ti[0].bg = ti[0].fg = COLOR_DEFAULT;
    for ( i=0; i<mm->named_instance_count; ++i ) {
	ti[i+1].text = (unichar_t *) PickNameFromMacName(mm->named_instances[i].names);
	ti[i+1].text_is_1byte = true;
	ti[i+1].bg = ti[i+1].fg = COLOR_DEFAULT;
    }
return( ti );
}

static int MMCB_PickedKnown(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct mmcb *mmcb = GDrawGetUserData(GGadgetGetWindow(g));
	int which = GGadgetGetFirstListSelectedItem(g);
	char buffer[24];
	int i;
	unichar_t *temp;

	--which;
	if ( which<0 )
return( true );
	for ( i=0; i<mmcb->mm->axis_count; ++i ) {
	    sprintf( buffer, "%.4g", (double) mmcb->mm->named_instances[which].coords[i]);
	    temp = uc_copy(buffer);
	    GGadgetSetTitle(GWidgetGetControl(mmcb->gw,1000+i),temp);
	    free(temp);
	}
    }
return( true );
}

static int MMCB_Changed(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	int explicitblends = GGadgetIsChecked(GWidgetGetControl(gw,CID_Explicit));
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NewBlends),explicitblends);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NewDesign),!explicitblends);
    }
return( true );
}

static int GetWeights(GWindow gw, real blends[MmMax], MMSet *mm,
	int instance_count, int axis_count) {
    int explicitblends = GGadgetIsChecked(GWidgetGetControl(gw,CID_Explicit));
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,
	    explicitblends?CID_NewBlends:CID_NewDesign)), *upt;
    unichar_t *uend;
    int i;
    real sum;

    sum = 0;
    for ( i=0, upt = ret; i<instance_count && *upt; ++i ) {
	blends[i] = u_strtod(upt,&uend);
	sum += blends[i];
	if ( upt==uend )
    break;
	upt = uend;
	while ( *upt==',' || *upt==' ' ) ++upt;
    }
    if ( (explicitblends && i!=instance_count ) ||
	    (!explicitblends && i!=axis_count ) ||
	    *upt!='\0' ) {
	gwwv_post_error(_("Bad MM Weights"),_("Incorrect number of instances weights, or illegal numbers"));
return(false);
    }
    if ( explicitblends ) {
	if ( sum<.99 || sum>1.01 ) {
	    gwwv_post_error(_("Bad MM Weights"),_("The weights for the default version of the font must sum to 1.0"));
return(false);
	}
    } else {
	i = ExecConvertDesignVector(blends, i, mm->ndv, mm->cdv,
		blends);
	if ( i!=instance_count ) {
	    gwwv_post_error(_("Bad MM Weights"),_("The results produced by applying the NormalizeDesignVector and ConvertDesignVector functions were not the results expected. You may need to change these functions"));
return(false);
	}
    }
return( true );
}

static int MMCB_OKApple(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct mmcb *mmcb = GDrawGetUserData(GGadgetGetWindow(g));
	real newcoords[4];
	int i, j, k, err=false;
	real blends[AppleMmMax];
	MMSet *mm = mmcb->mm;

	for ( i=0; i<mm->axis_count; ++i )
	    newcoords[i] = rint(GetReal8(mmcb->gw,1000+i,_(axistablab[i]),&err)*8096)/8096;
	if ( err )
return( true );
	/* Now normalize each */
	for ( i=0; i<mm->axis_count; ++i ) {
	    for ( j=1; j<mm->axismaps[i].points; ++j ) {
		if ( newcoords[i]<=mm->axismaps[i].designs[j] || j==mm->axismaps[i].points-1 ) {
		    if ( mm->axismaps[i].designs[j]==mm->axismaps[i].designs[j-1] )
			newcoords[i] = mm->axismaps[i].blends[j];
		    else
			newcoords[i] = mm->axismaps[i].blends[j-1] +
				(newcoords[i]-mm->axismaps[i].designs[j-1])/
			        (mm->axismaps[i].designs[j]-mm->axismaps[i].designs[j-1]) *
			        (mm->axismaps[i].blends[j]-mm->axismaps[i].blends[j-1]);
		    newcoords[i] = rint(8096*newcoords[i])/8096;	/* Apple's fixed numbers have a fair amount of rounding error */
	    break;
		}
	    }
	}
	/* Now figure out the contribution of each design */
	for ( k=0; k<mm->instance_count; ++k ) {
	    real factor = 1.0;
	    for ( i=0; i<mm->axis_count; ++i ) {
		if ( (newcoords[i]<=0 && mm->positions[k*mm->axis_count+i]>0) ||
			(newcoords[i]>=0 && mm->positions[k*mm->axis_count+i]<0)) {
		    factor = 0;
	    break;
		}
		if ( newcoords[i]==0 )
	    continue;
		if ( newcoords[i]<0 )
		    factor *= -newcoords[i];
		else
		    factor *= newcoords[i];
	    }
	    blends[k] = factor;
	}
	MakeAppleBlend(mm,blends,newcoords);
	mmcb->done = true;
    }
return( true );
}

FontView *MMCreateBlendedFont(MMSet *mm,FontView *fv,real blends[MmMax],int tonew ) {
    real oldblends[MmMax];
    SplineFont *hold = mm->normal;
    int i;
    real axispos[4];

    for ( i=0; i<mm->instance_count; ++i ) {
	oldblends[i] = mm->defweights[i];
	mm->defweights[i] = blends[i];
    }
    if ( tonew ) {
	SplineFont *new;
	FontView *oldfv = hold->fv;
	char *fn, *full;
	mm->normal = new = MMNewFont(mm,-1,hold->familyname);
	MMWeightsUnMap(blends,axispos,mm->axis_count);
	fn = _MMMakeFontname(mm,axispos,&full);
	free(new->fontname); free(new->fullname);
	new->fontname = fn; new->fullname = full;
	new->weight = _MMGuessWeight(mm,axispos,new->weight);
	new->private = BlendPrivate(PSDictCopy(hold->private),mm);
	new->fv = NULL;
	fv = FontViewCreate(new);
	MMReblend(fv,mm);
	new->mm = NULL;
	mm->normal = hold;
	for ( i=0; i<mm->instance_count; ++i ) {
	    mm->defweights[i] = oldblends[i];
	    mm->instances[i]->fv = oldfv;
	}
	hold->fv = oldfv;
    } else {
	for ( i=0; i<mm->instance_count; ++i )
	    mm->defweights[i] = blends[i];
	mm->changed = true;
	MMReblend(fv,mm);
    }
return( fv );
}

static int MMCB_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct mmcb *mmcb = GDrawGetUserData(GGadgetGetWindow(g));
	real blends[MmMax];

	if ( !GetWeights(mmcb->gw, blends, mmcb->mm, mmcb->mm->instance_count, mmcb->mm->axis_count))
return( true );
	MMCreateBlendedFont(mmcb->mm,mmcb->fv,blends,mmcb->tonew );
    }
return( true );
}

static int MMCB_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct mmcb *mmcb = GDrawGetUserData(GGadgetGetWindow(g));
	mmcb->done = true;
    }
return( true );
}

static int mmcb_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct mmcb *mmcb = GDrawGetUserData(gw);
	mmcb->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("mmmenu.html");
return( true );
	}
return( false );
    }
return( true );
}

static int GCDFillupMacWeights(GGadgetCreateData *gcd, GTextInfo *label, int k,
	char *axisnames[4], char axisval[4][24],
	real *defcoords,int axis_count,MMSet *mm) {
    int i;
    char *an;
    char axisrange[80];

    for ( i=0; i<axis_count; ++i ) {
	sprintf( axisrange, " [%.4g %.4g %.4g]", (double) mm->axismaps[i].min,
		(double) mm->axismaps[i].def, (double) mm->axismaps[i].max );
	an = PickNameFromMacName(mm->axismaps[i].axisnames);
	if ( an==NULL )
	    an = copy(mm->axes[i]);
	axisnames[i] = galloc(strlen(axisrange)+3+strlen(an));
	strcpy(axisnames[i],an);
	strcat(axisnames[i],axisrange);
	sprintf(axisval[i],"%.4g", (double) defcoords[i]);
	free(an);
    }
    for ( ; i<4; ++i ) {
	axisnames[i] = _(axistablab[i]);
	axisval[i][0] = '\0';
    }

    for ( i=0; i<4; ++i ) {
	label[k].text = (unichar_t *) axisnames[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = k==0 ? 4 : gcd[k-1].gd.pos.y+28;
	gcd[k].gd.flags = i<axis_count ? (gg_visible | gg_enabled) : gg_visible;
	gcd[k++].creator = GLabelCreate;

	label[k].text = (unichar_t *) axisval[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+12;
	gcd[k].gd.flags = gcd[k-1].gd.flags;
	gcd[k].gd.cid = 1000+i;
	gcd[k++].creator = GTextFieldCreate;
    }
return( k );
}

void MMChangeBlend(MMSet *mm,FontView *fv,int tonew) {
    char buffer[MmMax*20], *pt;
    unichar_t ubuf[MmMax*20];
    int i, k;
    struct mmcb mmcb;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14];
    GTextInfo label[14];
    unichar_t *utemp;
    char axisval[4][24];
    char *axisnames[4];
    real defcoords[4];

    if ( mm==NULL )
return;

    memset(&mmcb,0,sizeof(mmcb));
    mmcb.mm = mm;
    mmcb.fv = fv;
    mmcb.tonew = tonew;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = tonew ? _("Blend to New Font..."):_("MM Change Def Weights...");
    pos.x = pos.y = 0;

    if ( !mm->apple ) {
	pt = buffer;
	for ( i=0; i<mm->instance_count; ++i ) {
	    sprintf( pt, "%g ", (double) mm->defweights[i]);
	    pt += strlen(pt);
	}
	if ( pt>buffer )
	    pt[-2] = '\0';
	uc_strcpy(ubuf,buffer);

	pos.width =GDrawPointsToPixels(NULL,GGadgetScale(270));
	pos.height = GDrawPointsToPixels(NULL,200);
	mmcb.gw = gw = GDrawCreateTopWindow(NULL,&pos,mmcb_e_h,&mmcb,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	k=0;
/* GT: The following strings should be concatenated together, the result */
/* GT: translated, and then broken into lines by hand. I'm sure it would */
/* GT: be better to specify this all as one string, but my widgets won't support */
/* GT: that */
	label[k].text = (unichar_t *) (tonew ? _("You may specify the new instance of this font") : _("You may change the default instance of this font"));
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 10;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k++].creator = GLabelCreate;

	label[k].text = (unichar_t *) _("either by explicitly entering the contribution");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k++].creator = GLabelCreate;

	label[k].text = (unichar_t *) _("of each master design, or by entering the design");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k++].creator = GLabelCreate;

	label[k].text = (unichar_t *) _("values for each axis");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k++].creator = GLabelCreate;

	label[k].text = (unichar_t *) _("Contribution of each master design");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on;
	gcd[k].gd.cid = CID_Explicit;
	gcd[k].gd.handle_controlevent = MMCB_Changed;
	gcd[k++].creator = GRadioCreate;

	label[k].text = (unichar_t *) _("Design Axis Values");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+45;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k].gd.cid = CID_ByDesign;
	gcd[k].gd.handle_controlevent = MMCB_Changed;
	gcd[k++].creator = GRadioCreate;

	label[k].text = ubuf;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y+18;
	gcd[k].gd.pos.width = 240;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k].gd.cid = CID_NewBlends;
	gcd[k++].creator = GTextFieldCreate;

	label[k].text = utemp = MMDesignCoords(mm);
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y+18;
	gcd[k].gd.pos.width = 240;
	gcd[k].gd.flags = gg_visible;
	gcd[k].gd.cid = CID_NewDesign;
	gcd[k++].creator = GTextFieldCreate;

	gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
	gcd[k].gd.pos.width = -1;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[k].text = (unichar_t *) _("_OK");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = MMCB_OK;
	gcd[k++].creator = GButtonCreate;

	gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
	gcd[k].gd.pos.width = -1;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[k].text = (unichar_t *) _("_Cancel");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = MMCB_Cancel;
	gcd[k++].creator = GButtonCreate;

	gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
	gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-4;
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[k].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
	free(utemp);
    } else {
	pos.width =GDrawPointsToPixels(NULL,GGadgetScale(270));
	pos.height = GDrawPointsToPixels(NULL,200);
	mmcb.gw = gw = GDrawCreateTopWindow(NULL,&pos,mmcb_e_h,&mmcb,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	memset(defcoords,0,sizeof(defcoords));
	for ( i=0; i<mm->axis_count; ++i )
	    defcoords[i] = mm->axismaps[i].def;

	k=0;
	k = GCDFillupMacWeights(gcd,label,k,axisnames,axisval,defcoords,
		mm->axis_count,mm);

	gcd[k].gd.pos.x = 130; gcd[k].gd.pos.y = gcd[k-4].gd.pos.y-12;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	if ( mm->named_instance_count==0 )
	    gcd[k].gd.flags = 0;
	gcd[k].gd.u.list = MMCB_KnownValues(mm);
	gcd[k].gd.cid = CID_Knowns;
	gcd[k].gd.handle_controlevent = MMCB_PickedKnown;
	gcd[k++].creator = GListButtonCreate;

	gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
	gcd[k].gd.pos.width = -1;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[k].text = (unichar_t *) _("_OK");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = MMCB_OKApple;
	gcd[k++].creator = GButtonCreate;

	gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
	gcd[k].gd.pos.width = -1;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[k].text = (unichar_t *) _("_Cancel");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = MMCB_Cancel;
	gcd[k++].creator = GButtonCreate;

	gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
	gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-4;
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[k++].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
	for ( i=0; i<4; ++i )
	    free(axisnames[i]);
	GTextInfoListFree(gcd[k-4].gd.u.list);
	GWidgetIndicateFocusGadget(gcd[1].ret);
    }

    GDrawSetVisible(gw,true);

    while ( !mmcb.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

GTextInfo axiscounts[] = {
    { (unichar_t *) "1", NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "2", NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "3", NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "4", NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

/* These names are fixed by Adobe & Apple and are not subject to translation */
GTextInfo axistypes[] = {
    { (unichar_t *) "Weight",		NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "Width",		NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "OpticalSize",	NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "Slant",		NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

GTextInfo mastercounts[] = {
    { (unichar_t *) "1", NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "2", NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "3", NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "4", NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "5", NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "6", NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "16", NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "17", NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "18", NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "19", NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "20", NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "21", NULL, 0, 0, (void *) 21, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "22", NULL, 0, 0, (void *) 22, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "23", NULL, 0, 0, (void *) 23, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "24", NULL, 0, 0, (void *) 24, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "25", NULL, 0, 0, (void *) 25, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "26", NULL, 0, 0, (void *) 26, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "27", NULL, 0, 0, (void *) 27, NULL, 0, 0, 0, 0, 0, 0, 1},
#if AppleMmMax!=26
 #error "The mastercounts array needs to be expanded to match AppleMmMax"
    /* Actually it should be one bigger than AppleMmMax */
#endif
    { NULL }
};

typedef struct mmw {
    GWindow gw;
    enum mmw_state { mmw_counts, mmw_axes, mmw_designs, mmw_named, mmw_funcs,
	    mmw_others } state;
    GWindow subwins[mmw_others+1];
    MMSet *mm, *old;
    int isnew;
    int done;
    int old_axis_count, old_adobe;
    int axis_count, instance_count;	/* The data in mm are set to the max for each */
    int last_instance_count, last_axis_count, lastw_instance_count;
    struct axismap last_axismaps[4];
    int canceldrop, subheightdiff;
    int lcnt, lmax;
    SplineFont **loaded;
} MMW;

#define MMW_Width	340
#define MMW_Height	300
#define ESD_Width	262
#define ESD_Height	316

#define CID_OK		1001
#define CID_Prev	1002
#define CID_Next	1003
#define CID_Cancel	1004
#define CID_Group	1005

#define CID_AxisCount	2001
#define CID_MasterCount	2002
#define CID_Adobe	2003
#define CID_Apple	2004

#define CID_WhichAxis			3000
#define CID_AxisType			3001	/* +[0,3]*100 */
#define CID_AxisBegin			3002	/* +[0,3]*100 */
#define CID_AxisDefault			3003	/* +[0,3]*100 */
#define CID_AxisDefaultLabel		3004	/* +[0,3]*100 */
#define CID_AxisEnd			3005	/* +[0,3]*100 */
#define CID_IntermediateDesign		3006	/* +[0,3]*100 */
#define CID_IntermediateNormalized	3007	/* +[0,3]*100 */

#define DesignScaleFactor	20

#define CID_WhichDesign	4000
#define CID_DesignFonts	4001	/* +[0,26]*DesignScaleFactor */
#define CID_AxisWeights	4002	/* +[0,26]*DesignScaleFactor */

#define CID_NDV			5002
#define CID_CDV			5003

/* CID_Explicit-CID_NewDesign already defined */
#define CID_ForceBoldThreshold	6005
#define CID_FamilyName		6006

#define CID_NamedDesigns	7001
#define CID_NamedNew		7002
#define CID_NamedEdit		7003
#define CID_NamedDelete		7004

struct esd {
    GWindow gw;
    MMW *mmw;
    GGadget *list;
    int index;
    int done;
};

static void ESD_Close(struct esd *esd) {
    MacNameListFree(NameGadgetsGetNames(esd->gw));
    esd->done = true;
}

static int ESD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct esd *esd =  GDrawGetUserData(GGadgetGetWindow(g));
	ESD_Close(esd);
    }
return( true );
}

static int ESD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct esd *esd =  GDrawGetUserData(GGadgetGetWindow(g));
	int i,axis_count;
	int err = false;
	real coords[4];
	struct macname *mn;
	char buffer[120], *pt;
	unichar_t *name; char *style;

	for ( i=0; i<esd->mmw->axis_count && i<4; ++i )
	    coords[i] = rint(GetReal8(esd->gw,1000+i,_(axistablab[i]),&err)*8096)/8096;
	if ( err )
return( true );
	axis_count = i;
	mn = NameGadgetsGetNames(esd->gw);
	if ( mn==NULL ) {
	    gwwv_post_error(_("Bad Multiple Master Font"),_("You must provide at least one name here"));
return( true );
	}
	pt = buffer; *pt++ = ' '; *pt++ = '[';
	for ( i=0; i<axis_count; ++i ) {
	    sprintf(pt, "%g ", (double) coords[i]);
	    pt += strlen(pt);
	}
	pt[-1] = ']';
	*pt = '\0';
	style = PickNameFromMacName(mn);
	name = galloc(((pt-buffer) + strlen(style) + 1)*sizeof(unichar_t));
	utf82u_strcpy(name,style);
	uc_strcat(name,buffer);
	free(style);
	if ( esd->index==-1 )
	    GListAppendLine(esd->list,name,false)->userdata = mn;
	else {
	    GTextInfo *ti = GGadgetGetListItem(esd->list,esd->index);
	    MacNameListFree(ti->userdata);
	    GListChangeLine(esd->list,esd->index,name)->userdata = mn;
	}
	esd->done = true;
    }
return( true );
}

static int esd_eh(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct esd *esd = GDrawGetUserData(gw);
	ESD_Close(esd);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("multiplemaster.html#NamedStyles");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		ESD_Close(GDrawGetUserData(gw));
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	}
return( false );
    }
return( true );
}

static void EditStyleName(MMW *mmw,int index) {
    GGadget *list = GWidgetGetControl(mmw->subwins[mmw_named],CID_NamedDesigns);
    GTextInfo *ti = NULL;
    int i,k;
    unichar_t *pt = NULL, *end;
    real axes[4];
    struct macname *mn = NULL;
    char axisval[4][24];
    char *axisnames[4];
    GGadgetCreateData gcd[17];
    GTextInfo label[17];
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    struct esd esd;

    for ( i=0; i<mmw->axis_count; ++i )
	axes[i] = mmw->mm->axismaps[i].def;
    if ( index != -1 ) {
	ti = GGadgetGetListItem(list,index);
	if ( ti!=NULL ) {
	    pt = u_strchr(ti->text,'[');
	    mn = ti->userdata;
	}
	if ( pt!=NULL ) {
	    for ( i=0, ++pt; i<4 && (*pt!=']' || *pt!='\0'); ++i ) {
		axes[i] = u_strtod(pt,&end);
		pt = end;
	    }
	}
    }

    memset(&esd,0,sizeof(esd));
    esd.mmw = mmw;
    esd.index = index;
    esd.list = list;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Named Styles");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(ESD_Width));
    pos.height = GDrawPointsToPixels(NULL,ESD_Height);
    esd.gw = gw = GDrawCreateTopWindow(NULL,&pos,esd_eh,&esd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    k = GCDFillupMacWeights(gcd,label,k,axisnames,axisval,axes,
	    mmw->axis_count,mmw->mm);
    k = GCDBuildNames(gcd,label,k,mn);

    gcd[k].gd.pos.x = 20; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+33-3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = ESD_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -20; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = ESD_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[k].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);

    for ( i=0; i<4; ++i )
	free( axisnames[i]);

    GDrawSetVisible(gw,true);

    while ( !esd.done )
	GDrawProcessOneEvent(NULL);

    GDrawDestroyWindow(gw);
}

static void SetMasterToAxis(MMW *mmw, int initial) {
    int i, cnt, def, isadobe;

    cnt = GGadgetGetFirstListSelectedItem(GWidgetGetControl(mmw->subwins[mmw_counts],CID_AxisCount))
	    +1;
    isadobe = GGadgetIsChecked(GWidgetGetControl(mmw->subwins[mmw_counts],CID_Adobe));
    if ( cnt!=mmw->old_axis_count || isadobe!=mmw->old_adobe ) {
	GGadget *list = GWidgetGetControl(mmw->subwins[mmw_counts],CID_MasterCount);
	int32 len;
	GTextInfo **ti = GGadgetGetList(list,&len);
	if ( isadobe ) {
	    for ( i=0; i<MmMax; ++i )
		ti[i]->disabled = (i+1) < (1<<cnt);
	    for ( ; i<AppleMmMax+1 ; ++i )
		ti[i]->disabled = true;
	    def = (1<<cnt);
	} else {
	    for ( i=0; i<AppleMmMax+1; ++i )
		ti[i]->disabled = (i+1) < cnt;
	    def = 1;
	    for ( i=0; i<cnt; ++i )
		def *= 3;
	    if ( def>AppleMmMax+1 )
		def = AppleMmMax+1;
	}
	if ( !initial )
	    GGadgetSelectOneListItem(list,def-1);
	mmw->old_axis_count = cnt;
	mmw->old_adobe = isadobe;
    }
}

static int MMW_AxisCntChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	SetMasterToAxis(GDrawGetUserData(GGadgetGetWindow(g)),false);
    }
return( true );
}

static int MMW_TypeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	int isapple = GGadgetIsChecked(GWidgetGetControl(mmw->subwins[mmw_counts],CID_Apple));
	int i;
	SetMasterToAxis(mmw,false);
	for ( i=0; i<4; ++i ) {
	    GGadgetSetEnabled(GWidgetGetControl(mmw->subwins[mmw_axes],CID_AxisDefault+i*100),isapple);
	    GGadgetSetEnabled(GWidgetGetControl(mmw->subwins[mmw_axes],CID_AxisDefaultLabel+i*100),isapple);
	    NameGadgetsSetEnabled(GTabSetGetSubwindow(
		    GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),i),isapple);
	}
    }
return( true );
}

static void MMUsurpNew(SplineFont *sf) {
    /* This is a font that wasn't in the original MMSet */
    /* We ARE going to use it in the final result */
    /* So if it is attached to a fontview, we must close that window and */
    /*  claim the splinefont for ourselves */
    FontView *fv, *nextfv;

    if ( sf->fv!=NULL ) {
	if ( sf->kcld!=NULL )
	    KCLD_End(sf->kcld);
	if ( sf->vkcld!=NULL )
	    KCLD_End(sf->vkcld);
	sf->kcld = sf->vkcld = NULL;

	for ( fv=sf->fv; fv!=NULL; fv=nextfv ) {
	    nextfv = fv->nextsame;
	    fv->nextsame = NULL;
	    _FVCloseWindows(fv);
	    fv->sf = NULL;
	    GDrawDestroyWindow(fv->gw);
	}
	sf->fv = NULL;
	SFClearAutoSave(sf);
    }
}

static void MMDetachNew(SplineFont *sf) {
    /* This is a font that wasn't in the original MMSet */
    /* We aren't going to use it in the final result */
    /* If it is attached to a fontview, then the fontview retains control */
    /* If not, then free it */
    if ( sf->fv==NULL )
	SplineFontFree(sf);
}

static void MMDetachOld(SplineFont *sf) {
    /* This is a font that was in the original MMSet */
    /* We aren't going to use it in the final result */
    /* So then free it */
    sf->mm = NULL;
    SplineFontFree(sf);
}

static void MMW_Close(MMW *mmw) {
    int i;
    GGadget *list = GWidgetGetControl(mmw->subwins[mmw_named],CID_NamedDesigns);
    int32 len;
    GTextInfo **ti = GGadgetGetList(list,&len);

    for ( i=0; i<len; ++i )
	if ( ti[i]->userdata!=NULL )
	    MacNameListFree(ti[i]->userdata);
    for ( i=0; i<4; ++i )
	MacNameListFree(NameGadgetsGetNames(GTabSetGetSubwindow(
	    GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),i)));
    for ( i=0; i<mmw->lcnt; ++i )
	MMDetachNew(mmw->loaded[i]);
    free(mmw->loaded);
    for ( i=0; i<4; ++i )
	mmw->mm->axismaps[i].axisnames = NULL;
    MMSetFreeContents(mmw->mm);
    chunkfree(mmw->mm,sizeof(MMSet));
    mmw->done = true;
}

static int MMW_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	MMW_Close(mmw);
    }
return( true );
}

static void MMW_SetState(MMW *mmw) {
    int i;

    GDrawSetVisible(mmw->subwins[mmw->state],true);
    for ( i=mmw_counts; i<=mmw_others; ++i )
	if ( i!=mmw->state )
	    GDrawSetVisible(mmw->subwins[i],false);

    GGadgetSetEnabled(GWidgetGetControl(mmw->gw,CID_Prev),mmw->state!=mmw_counts);
    GGadgetSetEnabled(GWidgetGetControl(mmw->gw,CID_Next),
	    mmw->state!=mmw_others && mmw->state!=mmw_named);
    GGadgetSetEnabled(GWidgetGetControl(mmw->gw,CID_OK),
	    mmw->state==mmw_others || mmw->state==mmw_named);
}

static int ParseWeights(GWindow gw,int cid, char *str,
	real *list, int expected, int tabset_cid, int aspect ) {
    int cnt=0;
    const unichar_t *ret, *pt; unichar_t *endpt;

    ret= _GGadgetGetTitle(GWidgetGetControl(gw,cid));

    for ( pt=ret; *pt==' '; ++pt );
    for ( ; *pt; ) {
	list[cnt++] = u_strtod(pt,&endpt);
	if ( pt==endpt || ( *endpt!='\0' && *endpt!=' ' )) {
	    if ( tabset_cid!=-1 )
		GTabSetSetSel(GWidgetGetControl(gw,tabset_cid),aspect);
	    gwwv_post_error(_("Bad Axis"),_("Bad Number in %s"), str);
return( 0 );
	}
	for ( pt = endpt; *pt==' '; ++pt );
    }
    if ( cnt!=expected && expected!=-1 ) {
	if ( tabset_cid!=-1 )
	    GTabSetSetSel(GWidgetGetControl(gw,tabset_cid),aspect);
	gwwv_post_error(_("Bad Axis"),_("Wrong number of entries in %s"), str);
return( 0 );
    }

return( cnt );
}

static int ParseList(GWindow gw,int cid, char *str8, int *err, real start,
	real def, real end, real **_list, int tabset_cid, int aspect,
	int isapple ) {
    int i, cnt;
    const unichar_t *ret, *pt; unichar_t *endpt;
    real *list, val;
    int defdone = false;

    *_list = NULL;

    ret= _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    for ( pt=ret; *pt==' '; ++pt );
    cnt = *pt=='\0'?0:1 ;
    for ( ; *pt; ++pt ) {
	if ( *pt==' ' ) ++cnt;
	while ( *pt==' ' ) ++pt;
    }
    if ( start!=end )
	cnt+=2;
    if ( isapple && start!=end )
	++cnt;
    if ( !isapple || start==end )
	defdone = true;
    list = galloc(cnt*sizeof(real));
    if ( start==end )
	cnt = 0;
    else {
	list[0] = start;
	cnt = 1;
    }

    for ( pt=ret; *pt==' '; ++pt );
    for ( ; *pt; ) {
	val = u_strtod(pt,&endpt);
	if ( !defdone && val>def ) {
	    list[cnt++] = def;
	    defdone = true;
	}
	list[cnt++] = val;
	if ( pt==endpt || ( *endpt!='\0' && *endpt!=' ' )) {
	    GTabSetSetSel(GWidgetGetControl(gw,tabset_cid),aspect);
	    free(list);
	    gwwv_post_error(_("Bad Axis"),_("Bad Number in %s"), str8);
	    *err = true;
return( 0 );
	}
	for ( pt = endpt; *pt==' '; ++pt );
    }
    if ( start!=end )
	list[cnt++] = end;
    for ( i=1; i<cnt; ++i )
	if ( list[i-1]>list[i] ) {
	    GTabSetSetSel(GWidgetGetControl(gw,tabset_cid),aspect);
	    gwwv_post_error(_("Bad Axis"),_("The %s list is not ordered"), str8);
	    free(list);
	    *err = true;
return( 0 );
	}

    *_list = list;
return( cnt );
}

static char *_ChooseFonts(char *buffer, SplineFont **sfs, real *positions,
	int i, int cnt) {
    char *elsepart=NULL, *ret;
    int pos;
    int k;

    if ( i<cnt-2 )
	elsepart = _ChooseFonts(buffer,sfs,positions,i+1,cnt);

    pos = 0;
    if ( positions[i]!=0 ) {
	sprintf(buffer, "%g sub ", (double) positions[i]);
	pos += strlen(buffer);
    }
    sprintf(buffer+pos, "%g div dup 1 sub exch ", (double) (positions[i+1]-positions[i]));
    pos += strlen( buffer+pos );
    for ( k=0; k<i; ++k ) {
	strcpy(buffer+pos, "0 ");
	pos += 2;
    }
    if ( i!=0 ) {
	sprintf(buffer+pos, "%d -2 roll ", i+2 );
	pos += strlen(buffer+pos);
    }
    for ( k=i+2; k<cnt; ++k ) {
	strcpy(buffer+pos, "0 ");
	pos += 2;
    }

    if ( elsepart==NULL )
return( copy(buffer));

    ret = galloc(strlen(buffer)+strlen(elsepart)+40);
    sprintf(ret,"dup %g le {%s} {%s} ifelse", (double) positions[i+1], buffer, elsepart );
    free(elsepart);
return( ret );
}

static unichar_t *Figure1AxisCDV(MMW *mmw) {
    real positions[MmMax];
    SplineFont *sfs[MmMax];
    int i;
    char *temp;
    unichar_t *ret;
    char buffer[400];

    if ( mmw->axis_count!=1 )
return( uc_copy(""));
    if ( mmw->instance_count==2 )
return( uc_copy( standard_cdvs[1]));

    for ( i=0; i<mmw->instance_count; ++i ) {
	positions[i] = mmw->mm->positions[4*i];
	sfs[i] = mmw->mm->instances[i];
	if ( i>0 && positions[i-1]>=positions[i] )
return( uc_copy(""));
    }
    temp = _ChooseFonts(buffer,sfs,positions,0,mmw->instance_count);
    ret = uc_copy(temp);
    free(temp);
return( ret );
}

static char *_NormalizeAxis(char *buffer, struct axismap *axis, int i) {
    char *elsepart=NULL, *ret;
    int pos;

    if ( i<axis->points-2 )
	elsepart = _NormalizeAxis(buffer,axis,i+1);

    pos = 0;
    if ( axis->blends[i+1]==axis->blends[i] ) {
	sprintf( buffer, "%g ", (double) axis->blends[i] );
	pos = strlen(buffer);
    } else {
	if ( axis->designs[i]!=0 ) {
	    sprintf(buffer, "%g sub ", (double) axis->designs[i]);
	    pos += strlen(buffer);
	}
	sprintf(buffer+pos, "%g div ", (double) ((axis->designs[i+1]-axis->designs[i])/
		    (axis->blends[i+1]-axis->blends[i])));
	pos += strlen( buffer+pos );
	if ( axis->blends[i]!=0 ) {
	    sprintf(buffer+pos, "%g add ", (double) axis->blends[i]);
	    pos += strlen(buffer+pos);
	}
    }

    if ( elsepart==NULL )
return( copy(buffer));

    ret = galloc(strlen(buffer)+strlen(elsepart)+40);
    sprintf(ret,"dup %g le {%s} {%s} ifelse", (double) axis->designs[i+1], buffer, elsepart );
    free(elsepart);
return( ret );
}
    
static char *NormalizeAxis(char *header,struct axismap *axis) {
    char *ret;
    char buffer[200];

    ret = _NormalizeAxis(buffer,axis,0);
    if ( *header ) {
	char *temp;
	temp = galloc(strlen(header)+strlen(ret)+2);
	strcpy(temp,header);
	strcat(temp,ret);
	strcat(temp,"\n");
	free(ret);
	ret = temp;
    }
return( ret );
}

static int SameAxes(int cnt1,int cnt2,struct axismap *axismaps1,struct axismap *axismaps2) {
    int i,j;

    if ( cnt1!=cnt2 )
return( false );
    for ( i=0; i<cnt1; ++i ) {
	if ( axismaps1[i].points!=axismaps2[i].points )
return( false );
	for ( j=0; j<axismaps1[i].points; ++j ) {
	    if ( axismaps1[i].designs[j]>=axismaps2[i].designs[j]+.01 ||
		    axismaps1[i].designs[j]<=axismaps2[i].designs[j]-.01 )
return( false );
	    if ( axismaps1[i].blends[j]>=axismaps2[i].blends[j]+.001 ||
		    axismaps1[i].blends[j]<=axismaps2[i].blends[j]-.001 )
return( false );
	}
    }
return( true );
}

static void AxisDataCopyFree(struct axismap *into,struct axismap *from,int count) {
    int i;

    for ( i=0; i<4; ++i ) {
	free(into->blends); free(into->designs);
	into->blends = NULL; into->designs = NULL;
	into->points = 0;
    }
    for ( i=0; i<count; ++i ) {
	into[i].points = from[i].points;
	into[i].blends = galloc(into[i].points*sizeof(real));
	memcpy(into[i].blends,from[i].blends,into[i].points*sizeof(real));
	into[i].designs = galloc(into[i].points*sizeof(real));
	memcpy(into[i].designs,from[i].designs,into[i].points*sizeof(real));
    }
}

static int PositionsMatch(MMSet *old,MMSet *mm) {
    int i,j;

    for ( i=0; i<old->instance_count; ++i ) {
	for ( j=0; j<old->axis_count; ++j )
	    if ( old->positions[i*old->axis_count+j] != mm->positions[i*mm->axis_count+j] )
return( false );
    }
return( true );
}

static void MMW_FuncsValid(MMW *mmw) {
    unichar_t *ut;
    int pos, i;

    if ( !SameAxes(mmw->axis_count,mmw->last_axis_count,mmw->mm->axismaps,mmw->last_axismaps)) {
	if ( mmw->old!=NULL &&
		SameAxes(mmw->axis_count,mmw->old->axis_count,mmw->mm->axismaps,mmw->old->axismaps)) {
	    ut = uc_copy(mmw->old->ndv);
	} else {
	    char *header = mmw->axis_count==1 ?  "  " :
			    mmw->axis_count==2 ? "  exch " :
			    mmw->axis_count==3 ? "  3 -1 roll " :
						 "  4 -1 roll ";
	    char *lines[4];
	    for ( i=0; i<mmw->axis_count; ++i )
		lines[i] = NormalizeAxis(header,&mmw->mm->axismaps[i]);
	    pos = 0;
	    for ( i=0; i<mmw->axis_count; ++i )
		pos += strlen(lines[i]);
	    ut = galloc((pos+20)*sizeof(unichar_t));
	    uc_strcpy(ut,"{\n" ); pos = 2;
	    for ( i=0; i<mmw->axis_count; ++i ) {
		uc_strcpy(ut+pos,lines[i]);
		pos += strlen(lines[i]);
	    }
	    uc_strcpy(ut+pos,"}" );
	}
	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_NDV),
		ut);
	free(ut);
	AxisDataCopyFree(mmw->last_axismaps,mmw->mm->axismaps,mmw->axis_count);
	mmw->last_axis_count = mmw->axis_count;
    }
    if ( mmw->last_instance_count!=mmw->instance_count ) {
	if ( standard_cdvs[4]==NULL ) {
	    standard_cdvs[4] = galloc(strlen(cdv_4axis[0])+strlen(cdv_4axis[1])+
		    strlen(cdv_4axis[2])+2);
	    strcpy(standard_cdvs[4],cdv_4axis[0]);
	    strcat(standard_cdvs[4],cdv_4axis[1]);
	    strcat(standard_cdvs[4],cdv_4axis[2]);
	}
	if ( mmw->old!=NULL &&
		mmw->axis_count==mmw->old->axis_count &&
		mmw->instance_count==mmw->old->instance_count &&
		PositionsMatch(mmw->old,mmw->mm)) {
	    ut = uc_copy(mmw->old->cdv);
	} else if ( mmw->instance_count==(1<<mmw->axis_count) &&
		StandardPositions(mmw->mm,mmw->instance_count,mmw->axis_count,false)) {
	    ut = uc_copy(standard_cdvs[mmw->axis_count]);
	} else if ( mmw->axis_count==1 &&
		OrderedPositions(mmw->mm,mmw->instance_count,false)) {
	    ut = Figure1AxisCDV(mmw);
	} else {
	    ut = uc_copy("");
	}
	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_CDV),
		ut);
	free(ut);
    }
    mmw->last_instance_count = mmw->instance_count;
}

static void MMW_WeightsValid(MMW *mmw) {
    char *temp;
    unichar_t *ut, *utc;
    int pos, i;
    real axiscoords[4], weights[MmMax];

    if ( mmw->lastw_instance_count!=mmw->instance_count ) {
	temp = galloc(mmw->instance_count*20+1);
	pos = 0;
	if ( mmw->old!=NULL && mmw->instance_count==mmw->old->instance_count ) {
	    for ( i=0; i<mmw->instance_count; ++i ) {
		sprintf(temp+pos,"%g ", (double) mmw->old->defweights[i] );
		pos += strlen(temp+pos);
	    }
	    utc = MMDesignCoords(mmw->old);
	} else {
	    for ( i=0; i<mmw->axis_count; ++i ) {
		if ( strcmp(mmw->mm->axes[i],"Weight")==0 &&
			400>=mmw->mm->axismaps[i].designs[0] &&
			400<=mmw->mm->axismaps[i].designs[mmw->mm->axismaps[i].points-1])
		    axiscoords[i] = 400;
		else if ( strcmp(mmw->mm->axes[i],"OpticalSize")==0 &&
			12>=mmw->mm->axismaps[i].designs[0] &&
			12<=mmw->mm->axismaps[i].designs[mmw->mm->axismaps[i].points-1])
		    axiscoords[i] = 12;
		else
		    axiscoords[i] = (mmw->mm->axismaps[i].designs[0]+
			    mmw->mm->axismaps[i].designs[mmw->mm->axismaps[i].points-1])/2;
	    }
	    i = ExecConvertDesignVector(axiscoords,mmw->axis_count,mmw->mm->ndv,mmw->mm->cdv,
		weights);
	    if ( i!=mmw->instance_count ) {	/* The functions don't work */
		for ( i=0; i<mmw->instance_count; ++i )
		    weights[i] = 1.0/mmw->instance_count;
		utc = uc_copy("");
	    } else {
		for ( i=0; i<mmw->axis_count; ++i ) {
		    sprintf(temp+pos,"%g ", (double) axiscoords[i] );
		    pos += strlen(temp+pos);
		}
		temp[pos-1] = '\0';
		utc = uc_copy(temp);
		pos = 0;
	    }
	    for ( i=0; i<mmw->instance_count; ++i ) {
		sprintf(temp+pos,"%g ", (double) weights[i] );
		pos += strlen(temp+pos);
	    }
	}
	temp[pos-1] = '\0';
	ut = uc_copy(temp);
	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_others],CID_NewBlends),
		ut);
	free(temp); free(ut);

	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_others],CID_NewDesign),utc);
	free(utc);
	mmw->lastw_instance_count = mmw->instance_count;
    }
}

static GTextInfo *NamedDesigns(MMW *mmw) {
    int cnt, i, j;
    GTextInfo *ti;
    char buffer[120], *pt;
    char *ustyle;

    if ( !mmw->mm->apple || mmw->old==NULL )
return( NULL );

    cnt = mmw->old->named_instance_count;
    ti = gcalloc((cnt+1),sizeof(GTextInfo));
    for ( i=0; i<mmw->old->named_instance_count; ++i ) {
	pt = buffer; *pt++='[';
	for ( j=0; j<mmw->old->axis_count; ++j ) {
	    sprintf( pt, "%.4g ", (double) mmw->old->named_instances[i].coords[j]);
	    pt += strlen(pt);
	}
	pt[-1] = ']';
	ustyle = PickNameFromMacName(mmw->old->named_instances[i].names);
	ti[i].bg = ti[i].fg = COLOR_DEFAULT;
	ti[i].text = galloc((strlen(buffer)+3+strlen(ustyle))*sizeof(unichar_t));
	utf82u_strcpy(ti[i].text,ustyle);
	uc_strcat(ti[i].text," ");
	uc_strcat(ti[i].text,buffer);
	ti[i].userdata = MacNameCopy(mmw->old->named_instances[i].names);
	free(ustyle);
    }

return(ti);
}

static GTextInfo *TiFromFont(SplineFont *sf) {
    GTextInfo *ti = gcalloc(1,sizeof(GTextInfo));
    ti->text = uc_copy(sf->fontname);
    ti->fg = ti->bg = COLOR_DEFAULT;
    ti->userdata = sf;
return( ti );
}

static GTextInfo **FontList(MMW *mmw, int instance, int *sel) {
    FontView *fv;
    int cnt, i, pos;
    GTextInfo **ti;

    cnt = 0;
    if ( mmw->old!=NULL ) {
	cnt = mmw->old->instance_count;
	if ( mmw->old->apple )
	    ++cnt;
    }
    for ( fv=fv_list; fv!=NULL; fv=fv->next ) {
	if ( fv->cidmaster==NULL && fv->sf->mm==NULL )
	    ++cnt;
    }
    cnt += mmw->lcnt;
    
    ++cnt;	/* New */
    ++cnt;	/* Browse... */

    ti = galloc((cnt+1)*sizeof(GTextInfo *));
    pos = -1;
    cnt = 0;
    if ( mmw->old!=NULL ) {
	for ( i=0; i<mmw->old->instance_count; ++i ) {
	    if ( mmw->old->instances[i]==mmw->mm->instances[instance] ) pos = cnt;
	    ti[cnt++] = TiFromFont(mmw->old->instances[i]);
	}
	if ( mmw->old->apple ) {
	    if ( mmw->old->normal==mmw->mm->instances[instance] ) pos = cnt;
	    ti[cnt++] = TiFromFont(mmw->old->normal);
	}
    }
    for ( fv=fv_list; fv!=NULL; fv=fv->next ) {
	if ( fv->cidmaster==NULL && fv->sf->mm==NULL ) {
	    if ( fv->sf==mmw->mm->instances[instance] ) pos = cnt;
	    ti[cnt++] = TiFromFont(fv->sf);
	}
    }
    for ( i=0; i<mmw->lcnt; ++i ) {
	if ( mmw->loaded[i]==mmw->mm->instances[instance] ) pos = cnt;
	ti[cnt++] = TiFromFont( mmw->loaded[i]);
    }
    if ( pos==-1 ) pos=cnt;
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
    ti[cnt]->text = utf82u_copy(_("Font|New"));
    ti[cnt]->bg = ti[cnt]->fg = COLOR_DEFAULT;
    ++cnt;
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
    ti[cnt]->text = utf82u_copy(_("Browse..."));
    ti[cnt]->bg = ti[cnt]->fg = COLOR_DEFAULT;
    ti[cnt]->userdata = (void *) (-1);
    ++cnt;
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));

    ti[pos]->selected = true;
    *sel = pos;

return(ti);
}

static void MMW_DesignsSetup(MMW *mmw) {
    int i,j,sel;
    char buffer[80], *pt;
    unichar_t ubuf[80];
    GTextInfo **ti;

    for ( i=0; i<mmw->instance_count; ++i ) {
	GGadget *list = GWidgetGetControl(mmw->subwins[mmw_designs],CID_DesignFonts+i*DesignScaleFactor);
	ti = FontList(mmw,i,&sel);
	GGadgetSetList(list, ti,false);
	GGadgetSetTitle(list, ti[sel]->text);
	pt = buffer;
	for ( j=0; j<mmw->axis_count; ++j ) {
	    sprintf(pt,"%g ",(double) mmw->mm->positions[i*4+j]);
	    pt += strlen(pt);
	}
	if ( pt>buffer ) pt[-1] = '\0';
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_designs],CID_AxisWeights+i*DesignScaleFactor),
		ubuf);
    }
}

static void MMW_ParseNamedStyles(MMSet *setto,MMW *mmw) {
    GGadget *list = GWidgetGetControl(mmw->subwins[mmw_named],CID_NamedDesigns);
    int32 i,j,len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    unichar_t *upt, *end;

    setto->named_instance_count = len;
    if ( len!=0 ) {
	setto->named_instances = gcalloc(len,sizeof(struct named_instance));
	for ( i=0; i<len; ++i ) {
	    setto->named_instances[i].coords = gcalloc(setto->axis_count,sizeof(real));
	    upt = u_strchr(ti[i]->text,'[');
	    if ( upt!=NULL ) {
		for ( j=0, ++upt; j<setto->axis_count; ++j ) {
		    setto->named_instances[i].coords[j] = rint(u_strtod(upt,&end)*8096)/8096;
		    if ( *end==' ' ) ++end;
		    upt = end;
		}
	    }
	    setto->named_instances[i].names = ti[i]->userdata;
	    ti[i]->userdata = NULL;
	}
    }
}

static void MMW_DoOK(MMW *mmw) {
    real weights[AppleMmMax+1];
    real fbt;
    int err = false;
    char *familyname, *fn, *origname=NULL;
    int i,j;
    MMSet *setto, *dlgmm;
    FontView *fv = NULL;
    int isapple = GGadgetIsChecked(GWidgetGetControl(mmw->subwins[mmw_counts],CID_Apple));
    int defpos;
    struct psdict *oldprivate = NULL;

    if ( !isapple ) {
	if ( !GetWeights(mmw->gw, weights, mmw->mm, mmw->instance_count, mmw->axis_count))
return;
	fbt = GetReal8(mmw->subwins[mmw_others],CID_ForceBoldThreshold,
			_("Force Bold Threshold:"),&err);
	if ( err )
return;
    }

    familyname = cu_copy(_GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_counts],CID_FamilyName)));
    /* They only need specify a family name if there are new fonts */
    if ( *familyname=='\0' ) {
	free(familyname);
	for ( i=0; i<mmw->instance_count; ++i )
	    if ( mmw->mm->instances[i]==NULL )
	break;
	    else
		fn = mmw->mm->instances[i]->familyname;
	if ( i!=mmw->instance_count ) {
	    gwwv_post_error(_("Bad Multiple Master Font"),_("A Font Family name is required"));
return;
	}
	familyname = copy(fn);
    }

    /* Did we have a fontview open on this mm? */
    if ( mmw->old!=NULL ) {
	for ( j=0; j<mmw->old->instance_count; ++j )
	    if ( mmw->old->instances[j]->fv!=NULL ) {
		fv = mmw->old->instances[j]->fv;
		origname = copy(mmw->old->instances[j]->origname);
	break;
	    }
    }

    /* Make sure we free all fonts that we have lying around and aren't going */
    /* to be using. (ones we opened, ones in the old version of the mm). Also */
    /* if any font we want to use is attached to a fontview, then close the */
    /* window */
    for ( i=0; i<mmw->instance_count; ++i ) if ( mmw->mm->instances[i]!=NULL ) {
	if ( mmw->old!=NULL ) {
	    for ( j=0; j<mmw->old->instance_count; ++j )
		if ( mmw->mm->instances[i]==mmw->old->instances[j] )
	    break;
	    if ( j!=mmw->old->instance_count ) {
		mmw->old->instances[j] = NULL;
    continue;
	    } else if ( mmw->old->normal==mmw->mm->instances[i] ) {
		mmw->old->normal = NULL;
    continue;
	    }
	}
	for ( j=0; j<mmw->lcnt; ++j )
	    if ( mmw->mm->instances[i]==mmw->loaded[j] )
	break;
	if ( j!=mmw->lcnt ) {
	    mmw->loaded[j] = NULL;
continue;
	}
	MMUsurpNew(mmw->mm->instances[i]);
    }
    if ( mmw->old!=NULL ) {
	for ( j=0; j<mmw->old->instance_count; ++j )
	    if ( mmw->old->instances[j]!=NULL ) {
		MMDetachOld(mmw->old->instances[j]);
		mmw->old->instances[j] = NULL;
	    }
	if ( mmw->old->normal!=NULL ) {
	    oldprivate = mmw->old->normal->private;
	    mmw->old->normal->private = NULL;
	    MMDetachOld(mmw->old->normal);
	    mmw->old->normal = NULL;
	}
    }
    for ( j=0; j<mmw->lcnt; ++j ) {
	if ( mmw->loaded[j]!=NULL ) {
	    MMDetachNew(mmw->loaded[j]);
	    mmw->loaded[j] = NULL;
	}
    }

    dlgmm = mmw->mm;
    setto = mmw->old;
    if ( setto!=NULL ) {
	MMSetFreeContents(setto);
	memset(setto,0,sizeof(MMSet));
    } else
	setto = chunkalloc(sizeof(MMSet));
    setto->apple = isapple;
    setto->axis_count = mmw->axis_count;
    setto->instance_count = mmw->instance_count;
    defpos = mmw->instance_count;
    if ( isapple ) {
	for ( defpos=0; defpos<mmw->instance_count; ++defpos ) {
	    for ( j=0; j<mmw->axis_count; ++j )
		if ( dlgmm->positions[defpos*dlgmm->axis_count+j]!=0 )
	    break;
	    if ( j==mmw->axis_count )
	break;
	}
	if ( defpos==mmw->instance_count )
	    --defpos;
	--setto->instance_count;
	setto->normal = dlgmm->instances[defpos];
	if ( setto->normal!=NULL )
	    setto->normal->mm = setto;
    }
    for ( i=0; i<setto->axis_count; ++i )
	setto->axes[i] = dlgmm->axes[i];
    setto->axismaps = dlgmm->axismaps;
    setto->defweights = gcalloc(setto->instance_count,sizeof(real));
    if ( !isapple )
	memcpy(setto->defweights,weights,setto->instance_count*sizeof(real));
    free(dlgmm->defweights);
    setto->positions = galloc(setto->instance_count*setto->axis_count*sizeof(real));
    for ( i=0; i<setto->instance_count; ++i ) {
	int k = i<defpos ? i : i+1;
	memcpy(setto->positions+i*setto->axis_count,dlgmm->positions+k*dlgmm->axis_count,
		setto->axis_count*sizeof(real));
    }
    free(dlgmm->positions);
    setto->instances = gcalloc(setto->instance_count,sizeof(SplineFont *));
    for ( i=0; i<setto->instance_count; ++i ) {
	if ( dlgmm->instances[i]!=NULL ) {
	    int k = i<defpos ? i : i+1;
	    setto->instances[i] = dlgmm->instances[k];
	    setto->instances[i]->mm = setto;
	}
    }
    MMMatchGlyphs(setto);
    if ( setto->normal==NULL ) {
	setto->normal = MMNewFont(setto,-1,familyname);
	setto->normal->private = oldprivate;
    }
    if ( !isapple ) {
	if ( fbt>0 && fbt<=1 ) {
	    char buffer[20];
	    sprintf(buffer,"%g", (double) fbt );
	    if ( oldprivate==NULL )
		setto->normal->private = gcalloc(1,sizeof(struct psdict));
	    PSDictChangeEntry(setto->normal->private,"ForceBoldThreshold",buffer);
	}
    }
    if ( !isapple ) {
	setto->cdv = dlgmm->cdv;
	setto->ndv = dlgmm->ndv;
    } else
	MMW_ParseNamedStyles(setto,mmw);
    for ( i=0; i<setto->instance_count; ++i ) {
	if ( setto->instances[i]==NULL )
	    setto->instances[i] = MMNewFont(setto,i,familyname);
	setto->instances[i]->fv = fv;
    }
    free(dlgmm->instances);
    chunkfree(dlgmm,sizeof(MMSet));
    if ( origname!=NULL ) {
	for ( i=0; i<setto->instance_count; ++i ) {
	    free(setto->instances[i]->origname);
	    setto->instances[i]->origname = copy(origname);
	}
	free(setto->normal->origname);
	setto->normal->origname = origname;
    } else {
	for ( i=0; i<setto->instance_count; ++i ) {
	    free(setto->instances[i]->origname);
	    setto->instances[i]->origname = copy(setto->normal->origname);
	}
    }
    if ( !isapple )
	MMReblend(fv,setto);
    if ( fv!=NULL ) {
	for ( i=0; i<setto->instance_count; ++i )
	    if ( fv->sf==setto->instances[i])
	break;
	if ( i==setto->instance_count ) {
	    SplineFont *sf = setto->normal;
	    BDFFont *bdf;
	    int same = fv->filled == fv->show;
	    fv->sf = sf;
	    bdf = SplineFontPieceMeal(fv->sf,sf->display_size<0?-sf->display_size:default_fv_font_size,
		    (fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0),
		    NULL);
	    BDFFontFree(fv->filled);
	    fv->filled = bdf;
	    if ( same )
		fv->show = bdf;
	}
    }
    free(familyname);

    /* Multi-Mastered bitmaps don't make much sense */
    /* Well, maybe grey-scaled could be interpolated, but yuck */
    for ( i=0; i<setto->instance_count; ++i ) {
	BDFFont *bdf, *bnext;
	for ( bdf = setto->instances[i]->bitmaps; bdf!=NULL; bdf = bnext ) {
	    bnext = bdf->next;
	    BDFFontFree(bdf);
	}
	setto->instances[i]->bitmaps = NULL;
    }

    if ( fv==NULL )
	fv = FontViewCreate(setto->normal);
    mmw->done = true;
}

static void MMW_DoNext(MMW *mmw) {
    int i, err;
    real start, end, def, *designs, *norm;
    int n, n2;
    int isapple = GGadgetIsChecked(GWidgetGetControl(mmw->subwins[mmw_counts],CID_Apple));
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *yesno[3];
    yesno[0] = _("_Yes"); yesno[1] = _("_No"); yesno[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *yesno[3] = { GTK_STOCK_YES, GTK_STOCK_NO, NULL };
#endif

    if ( mmw->state==mmw_others )
return;

    if ( mmw->state==mmw_counts ) {
	mmw->axis_count = GGadgetGetFirstListSelectedItem(GWidgetGetControl(mmw->subwins[mmw_counts],CID_AxisCount))+1;
	mmw->instance_count = GGadgetGetFirstListSelectedItem(GWidgetGetControl(mmw->subwins[mmw_counts],CID_MasterCount))+1;
	/* Arrays are already allocated out to maximum, and we will just leave*/
	/*  them there until user hits OK, then we make them the right size */
	for ( i=0; i<4; ++i )
	    GTabSetSetEnabled(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
		    i,i<mmw->axis_count);
	for ( i=0; i<AppleMmMax+1; ++i )
	    GTabSetSetEnabled(GWidgetGetControl(mmw->subwins[mmw_designs],CID_WhichDesign),
		    i,i<mmw->instance_count);
	/* If we've changed the axis count, and the old selected axis isn't */
	/*  available any more, choose another one */
	if ( GTabSetGetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis))>=
		mmw->axis_count )
	    GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
		    0);
	if ( isapple ) {
	    int cnt = 1;
	    for ( i=0; i<mmw->axis_count; ++i ) cnt *= 3;
	    if ( mmw->instance_count==cnt ) {
		for ( i=(mmw->old==NULL)?0:mmw->old->instance_count; i<mmw->instance_count-1; ++i ) {
		    int j = (i>=(cnt-1)/2) ? i+1 : i;
		    mmw->mm->positions[i*4  ] = (j%3==0) ? -1: (j%3==1) ? 0 : 1;
		    mmw->mm->positions[i*4+1] = ((j/3)%3==0) ? -1: ((j/3)%3==1) ? 0 : 1;
		    mmw->mm->positions[i*4+2] = ((j/9)%3==0) ? -1: ((j/9)%3==1) ? 0 : 1;
		    mmw->mm->positions[i*4+3] = ((j/27)%3==0) ? -1: ((j/27)%3==1) ? 0 : 1;
		}
		/* Place the default psuedo-instance last */
		mmw->mm->positions[i*4  ] = 0;
		mmw->mm->positions[i*4+1] = 0;
		mmw->mm->positions[i*4+2] = 0;
		mmw->mm->positions[i*4+3] = 0;
	    }
	} else {
	    if ( mmw->instance_count==(1<<mmw->axis_count) ) {
		for ( i=(mmw->old==NULL)?0:mmw->old->instance_count; i<mmw->instance_count; ++i ) {
		    mmw->mm->positions[i*4  ] = (i&1) ? 1 : 0;
		    mmw->mm->positions[i*4+1] = (i&2) ? 1 : 0;
		    mmw->mm->positions[i*4+2] = (i&4) ? 1 : 0;
		    mmw->mm->positions[i*4+3] = (i&8) ? 1 : 0;
		}
	    }
	}
    } else if ( mmw->state==mmw_axes ) {
	for ( i=0; i<mmw->axis_count; ++i ) {
	    free(mmw->mm->axes[i]);
	    mmw->mm->axes[i] = cu_copy(_GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_axes],CID_AxisType+i*100)));
	    if ( *mmw->mm->axes[i]=='\0' ) {
		GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
			i);
		gwwv_post_error(_("Bad Axis"),_("Please set the Axis Type field"));
return;		/* Failure */
	    }
	    /* Don't free the current value. If it is non-null then it just */
	    /*  points into the data structure that the Names gadgets manipulate */
	    /*  and they will have done any freeing that needs doing. Freeing */
	    /*  it here would destroy the data they work on */
	    /*MacNameListFree(mmw->mm->axismaps[i].axisnames);*/
	    mmw->mm->axismaps[i].axisnames = NULL;
	    if ( isapple ) {
		mmw->mm->axismaps[i].axisnames = NameGadgetsGetNames(GTabSetGetSubwindow(
			GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),i));
		if ( mmw->mm->axismaps[i].axisnames == NULL ) {
		    GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
			    i);
		    gwwv_post_error(_("Bad Axis"),_("When building an Apple distortable font, you must specify at least one name for the axis"));
return;		    /* Failure */
		}
	    }
	    err = false;
	    start = GetReal8(mmw->subwins[mmw_axes],CID_AxisBegin+i*100,
		    _("Begin:"),&err);
	    end = GetReal8(mmw->subwins[mmw_axes],CID_AxisEnd+i*100,
		    _("End:"),&err);
	    if ( isapple ) {
		def = rint(GetReal8(mmw->subwins[mmw_axes],CID_AxisDefault+i*100,
			_("Default"),&err)*8096)/8096;
		start = rint(start*8096)/8096;
		end = rint(end*8096)/8096;
	    } else
		def = start;
	    if ( start>=end || def<start || def>end ) {
		GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
			i);
		gwwv_post_error(_("Bad Axis"),_("Axis range not valid"));
return;		/* Failure */
	    }
	    n = ParseList(mmw->subwins[mmw_axes],CID_IntermediateDesign+i*100,
		    _("Design Settings:"),&err,start,def,end,&designs,CID_WhichAxis,i,isapple);
	    n2 = ParseList(mmw->subwins[mmw_axes],CID_IntermediateNormalized+i*100,
		    _("Normalized Settings:"),&err,
			isapple?-1:0,0,1,&norm,CID_WhichAxis,i,isapple);
	    if ( n!=n2 || err ) {
		GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
			i);
		if ( !err )
		    gwwv_post_error(_("Bad Axis"),_("The number of entries in the design settings must match the number in normalized settings"));
		free(designs); free(norm);
return;		/* Failure */
	    }
	    mmw->mm->axismaps[i].points = n;
	    free(mmw->mm->axismaps[i].blends); free(mmw->mm->axismaps[i].designs);
	    mmw->mm->axismaps[i].blends = norm; mmw->mm->axismaps[i].designs=designs;
	    mmw->mm->axismaps[i].min = start;
	    mmw->mm->axismaps[i].def = def;
	    mmw->mm->axismaps[i].max = end;
	}
    } else if ( mmw->state==mmw_designs ) {
	real positions[AppleMmMax+1][4];
	int used[AppleMmMax+1];
	int j,k,mask, mul;
	SplineFont *sfs[AppleMmMax+1];
	GTextInfo *ti;

	memset(used,0,sizeof(used));
	memset(positions,0,sizeof(positions));
	for ( i=0; i<mmw->instance_count; ++i ) {
	    if ( !ParseWeights(mmw->subwins[mmw_designs],CID_AxisWeights+i*DesignScaleFactor,
		    _("Normalized position of this design along each axis"),positions[i],mmw->axis_count,
		    CID_WhichDesign,i))
return;
	    if ( isapple ) {
		mask = 0; mul = 1;
		for ( j=0; j<mmw->axis_count; ++j ) {
		    if ( positions[i][j]==-1 )
			/* Do Nothing */;
		    else if ( positions[i][j]==0.0 )
			mask += mul;
		    else if ( positions[i][j]==1.0 )
			mask += 2*mul;
		    else
		break;
		}
	    } else {
		mask = 0;
		for ( j=0; j<mmw->axis_count; ++j ) {
		    if ( positions[i][j]==0 )
			/* Do Nothing */;
		    else if ( positions[i][j]==1.0 )
			mask |= (1<<j);
		    else
		break;
		}
	    }
	    if ( j==mmw->axis_count )
		used[mask] = true;
	    for ( j=0; j<i-1; ++j ) {
		for ( k=0; k<mmw->axis_count; ++k )
		    if ( positions[j][k] != positions[i][k] )
		break;
		if ( k==mmw->axis_count ) {
		    char *temp;
		    GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_designs],CID_WhichDesign),i);
		    gwwv_post_error(_("Bad Multiple Master Font"),_("The set of positions, %.30s, is used more than once"),
			    temp = GGadgetGetTitle8(GWidgetGetControl(mmw->subwins[mmw_designs],CID_AxisWeights+i*DesignScaleFactor)));
		    free(temp);
return;
		}
	    }
	    ti = GGadgetGetListItemSelected(GWidgetGetControl(mmw->subwins[mmw_designs],CID_DesignFonts+i*DesignScaleFactor));
	    sfs[i] = ti->userdata;
	    if ( sfs[i]!=NULL ) {
		for ( j=0; j<i; ++j )
		    if ( sfs[i]==sfs[j] ) {
			GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_designs],CID_WhichDesign),i);
			gwwv_post_error(_("Bad Multiple Master Font"),_("The font %.30s is assigned to two master designs"),sfs[i]->fontname);
return;
		    }
	    }
	}
	for ( i=0; i<(1<<mmw->axis_count); ++i ) if ( !used[i] ) {
	    char buffer[20], *pt = buffer;
	    for ( j=0; j<mmw->axis_count; ++j ) {
		sprintf( pt, "%d ", (i&(1<<j))? 1: 0 );
		pt += 2;
	    }
	    if ( !isapple ) {
		gwwv_post_error(_("Bad Multiple Master Font"),_("The set of positions, %.30s, is not specified in any design (and should be)"), buffer );
return;
	    } else {
		if ( gwwv_ask(_("Bad Multiple Master Font"),(const char **) yesno,0,1,_("The set of positions, %.30s, is not specified in any design.\nIs that what you want?"),buffer)==1 )
return;
	    }
	}
	memcpy(mmw->mm->positions,positions,sizeof(positions));
	for ( i=0; i<mmw->instance_count; ++i )
	    mmw->mm->instances[i] = sfs[i];
	if ( mmw->old!=NULL &&
		mmw->axis_count==mmw->old->axis_count &&
		mmw->instance_count==mmw->old->instance_count &&
		PositionsMatch(mmw->old,mmw->mm))
	    /* It's what the font started with, don't complain, already has a cdv */;
	else if ( mmw->instance_count==(1<<mmw->axis_count) &&
		StandardPositions(mmw->mm,mmw->instance_count,mmw->axis_count,isapple))
	    /* It's arranged as we expect it to be */;
	else if ( mmw->axis_count==1 &&
		OrderedPositions(mmw->mm,mmw->instance_count,isapple))
	    /* It's arranged according to our secondary expectations */;
	else if ( !isapple && (mmw->instance_count==(1<<mmw->axis_count) ||
		mmw->axis_count==1 )) {
	    if ( gwwv_ask(_("Disordered designs"),(const char **) yesno,0,1,_("The master designs are not positioned in the expected order. FontForge will be unable to suggest a ConvertDesignVector for you. Is this what you want?"))==1 )
return;
	}
    } else if ( mmw->state==mmw_funcs ) {
	if ( *_GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_NDV))=='\0' ||
		*_GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_CDV))=='\0' ) {
	    gwwv_post_error(_("Bad PostScript function"),_("Bad PostScript function"));
return;
	}
	free(mmw->mm->ndv); free(mmw->mm->cdv);
	mmw->mm->ndv = cu_copy( _GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_NDV)));
	mmw->mm->cdv = cu_copy( _GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_CDV)));
    }

    if ( mmw->state==mmw_designs && !isapple )
	mmw->state += 2;
    else
	++mmw->state;
    if ( mmw->state==mmw_others )
	MMW_WeightsValid(mmw);
    else if ( mmw->state==mmw_funcs )
	MMW_FuncsValid(mmw);
    else if ( mmw->state==mmw_designs )
	MMW_DesignsSetup(mmw);
    MMW_SetState(mmw);
}

static void MMW_SimulateDefaultButton(MMW *mmw) {
    if ( mmw->state==mmw_others || mmw->state==mmw_named )
	MMW_DoOK(mmw);
    else
	MMW_DoNext(mmw);
}

static int MMW_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	MMW_DoOK(mmw);
    }
return( true );
}

static int MMW_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	MMW_DoNext(mmw);
    }
return( true );
}

static int MMW_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	if ( mmw->state!=mmw_counts ) {
	    if ( mmw->state==mmw_funcs )
		mmw->state = mmw_designs;
	    else
		--mmw->state;
	    MMW_SetState(mmw);
	}
    }
return( true );
}

static int MMW_NamedNew(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	EditStyleName(mmw,-1);
    }
return( true );
}

static int MMW_NamedEdit(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	EditStyleName(mmw,GGadgetGetFirstListSelectedItem(GWidgetGetControl(mmw->subwins[mmw_named],CID_NamedDesigns)));
    }
return( true );
}

static int MMW_NamedDelete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_NamedDesigns);
	int32 i,len;
	GTextInfo **ti = GGadgetGetList(list,&len);
	for ( i=0; i<len; ++i ) {
	    if ( ti[i]->selected ) {
		MacNameListFree(ti[i]->userdata);
		ti[i]->userdata = NULL;
	    }
	}
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NamedDelete),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NamedEdit),false);
    }
return( true );
}

static int MMW_NamedSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	GWindow gw = GGadgetGetWindow(g);
	int i, sel_cnt=0;
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected ) ++sel_cnt;
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NamedDelete),sel_cnt!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NamedEdit),sel_cnt==1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	EditStyleName(mmw,GGadgetGetFirstListSelectedItem(g));
    }
return( true );
}

static int MMW_CheckOptical(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	char *top, *bottom, *def;
	unichar_t *ut;
	const unichar_t *ret = _GGadgetGetTitle(g);
	int di = (GGadgetGetCid(g)-CID_AxisType)/100;
	char buf1[20], buf2[20], buf3[20];

	if ( mmw->old!=NULL && di<mmw->old->axis_count &&
		uc_strcmp(ret,mmw->old->axes[di])==0 ) {
	    sprintf(buf1,"%g", (double) mmw->old->axismaps[di].designs[0]);
	    sprintf(buf2,"%g", (double) mmw->old->axismaps[di].designs[mmw->old->axismaps[di].points-1]);
	    sprintf(buf3,"%g", (double) mmw->old->axismaps[di].def);
	    def = buf3;
	    top = buf2;
	    bottom = buf1;
	} else if ( uc_strcmp(ret,"OpticalSize")==0 ) {
	    top = "72";
	    def = "12";
	    bottom = "6";
	} else if ( uc_strcmp(ret,"Slant")==0 ) {
	    top = "-22";
	    def = "0";
	    bottom = "22";
	} else if ( GGadgetIsChecked(GWidgetGetControl(mmw->subwins[mmw_counts],CID_Apple)) ) {
	    top = "2.0";
	    bottom = "0.5";
	    def = "1.0";
	} else {
	    top = "999";
	    bottom = "50";
	    def = "400";
	}
	ut = uc_copy(top);
	GGadgetSetTitle(GWidgetGetControl(GGadgetGetWindow(g),
		GGadgetGetCid(g)-CID_AxisType + CID_AxisEnd), ut);
	free(ut);
	ut = uc_copy(bottom);
	GGadgetSetTitle(GWidgetGetControl(GGadgetGetWindow(g),
		GGadgetGetCid(g)-CID_AxisType + CID_AxisBegin), ut);
	free(ut);
	ut = uc_copy(def);
	GGadgetSetTitle(GWidgetGetControl(GGadgetGetWindow(g),
		GGadgetGetCid(g)-CID_AxisType + CID_AxisDefault), ut);
	free(ut);
    }
return( true );
}

static int MMW_CheckBrowse(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	/*int di = (GGadgetGetCid(g)-CID_DesignFonts)/DesignScaleFactor;*/
	GTextInfo *ti = GGadgetGetListItemSelected(g);
	char *temp;
	SplineFont *sf;
	GTextInfo **tis;
	int i,sel,oldsel;
	unichar_t *ut;

	if ( ti!=NULL && ti->userdata == (void *) -1 ) {
	    temp = GetPostscriptFontName(NULL,false);
	    if ( temp==NULL )
return(true);
	    sf = LoadSplineFont(temp,0);
	    if ( sf==NULL )
return(true);
	    if ( sf->cidmaster!=NULL || sf->subfonts!=0 ) {
		gwwv_post_error(_("Bad Multiple Master Font"),_("CID keyed fonts may not be a master design of a multiple master font"));
return(true);
	    } else if ( sf->mm!=NULL ) {
		gwwv_post_error(_("Bad Multiple Master Font"),_("CID keyed fonts may not be a master design of a multiple master font"));
return(true);
	    }
	    if ( sf->fv==NULL ) {
		if ( mmw->lcnt>=mmw->lmax ) {
		    if ( mmw->lmax==0 )
			mmw->loaded = galloc((mmw->lmax=10)*sizeof(SplineFont *));
		    else
			mmw->loaded = grealloc(mmw->loaded,(mmw->lmax+=10)*sizeof(SplineFont *));
		}
		mmw->loaded[mmw->lcnt++] = sf;
		for ( i=0; i<mmw->instance_count; ++i ) {
		    GGadget *list = GWidgetGetControl(mmw->subwins[mmw_designs],CID_DesignFonts+i*DesignScaleFactor);
		    oldsel = GGadgetGetFirstListSelectedItem(list);
		    tis = FontList(mmw,i,&sel);
		    tis[sel]->selected = false;
		    tis[oldsel]->selected = true;
		    GGadgetSetList(list, tis, false);
		}
	    }
	    GGadgetSetTitle(g,ut = uc_copy(sf->fontname));
	    free(ut);
	}
    }
return( true );
}

static int mmwsub_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("multiplemaster.html");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		MMW_Close(GDrawGetUserData(gw));
return( true );
	} else if ( event->u.chr.chars[0]=='\r' ) {
	    MMW_SimulateDefaultButton( (MMW *) GDrawGetUserData(gw));
return( true );
	}
return( false );
    }
return( true );
}

static int mmw_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	MMW *mmw = GDrawGetUserData(gw);
	MMW_Close(mmw);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("multiplemaster.html");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		MMW_Close(GDrawGetUserData(gw));
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	} else if ( event->u.chr.chars[0]=='\r' ) {
	    MMW_SimulateDefaultButton( (MMW *) GDrawGetUserData(gw));
return( true );
	}
return( false );
    }
return( true );
}

static MMSet *MMCopy(MMSet *orig) {
    MMSet *mm;
    int i;
    /* Allocate the arrays out to maximum, we'll fix them up later, and we */
    /*  retain the proper counts in the mmw structure. This means we don't */
    /*  lose data when they shrink and then restore a value */

    mm = chunkalloc(sizeof(MMSet));
    mm->apple = orig->apple;
    mm->instance_count = AppleMmMax+1;
    mm->axis_count = 4;
    for ( i=0; i<orig->axis_count; ++i )
	mm->axes[i] = copy(orig->axes[i]);
    mm->instances = gcalloc(AppleMmMax+1,sizeof(SplineFont *));
    memcpy(mm->instances,orig->instances,orig->instance_count*sizeof(SplineFont *));
    if ( mm->apple )
	mm->instances[orig->instance_count] = orig->normal;
    mm->positions = gcalloc((AppleMmMax+1)*4,sizeof(real));
    for ( i=0; i<orig->instance_count; ++i )
	memcpy(mm->positions+i*4,orig->positions+i*orig->axis_count,orig->axis_count*sizeof(real));
    mm->defweights = gcalloc(AppleMmMax+1,sizeof(real));
    memcpy(mm->defweights,orig->defweights,orig->instance_count*sizeof(real));
    mm->axismaps = gcalloc(4,sizeof(struct axismap));
    for ( i=0; i<orig->axis_count; ++i ) {
	mm->axismaps[i].points = orig->axismaps[i].points;
	mm->axismaps[i].blends = galloc(mm->axismaps[i].points*sizeof(real));
	memcpy(mm->axismaps[i].blends,orig->axismaps[i].blends,mm->axismaps[i].points*sizeof(real));
	mm->axismaps[i].designs = galloc(mm->axismaps[i].points*sizeof(real));
	memcpy(mm->axismaps[i].designs,orig->axismaps[i].designs,mm->axismaps[i].points*sizeof(real));
	mm->axismaps[i].min = orig->axismaps[i].min;
	mm->axismaps[i].max = orig->axismaps[i].max;
	mm->axismaps[i].def = orig->axismaps[i].def;
    }
    mm->cdv = copy(orig->cdv);
    mm->ndv = copy(orig->ndv);
return( mm );
}

void MMWizard(MMSet *mm) {
    MMW mmw;
    GRect pos, subpos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo axisaspects[5], designaspects[AppleMmMax+1+1];
    GGadgetCreateData bgcd[8], cntgcd[11], axisgcd[4][20], designgcd[AppleMmMax+1][5],
	    agcd[2], dgcd[3], ogcd[7], ngcd[7];
    GTextInfo blabel[8], cntlabel[11], axislabel[4][20], designlabel[AppleMmMax+1][5],
	    dlabel, olabels[7], nlabel[5];
    char axisbegins[4][20], axisends[4][20], axisdefs[4][20];
    char *normalized[4], *designs[4];
    char *pt, *freeme;
    int i,k;
    int isadobe = mm==NULL || !mm->apple;
    int space,blen= GIntGetResource(_NUM_Buttonsize)*100/GGadgetScale(100);
    static char *designtablab[] = { "1", "2", "3", "4", "5", "6", "7", "8",
	    "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
	    "20", "21", "22", "23", "24", "25", "26", "27", NULL };
#if AppleMmMax!=26
 #error "The designtablab array needs to be expanded to match AppleMmMax"
    /* Actually it should be one bigger than AppleMmMax */
#endif

    memset(&mmw,0,sizeof(mmw));
    mmw.old = mm;
    if ( mm!=NULL ) {
	mmw.mm = MMCopy(mm);
	mmw.axis_count = mm->axis_count;
	mmw.instance_count = mm->instance_count;
	if ( mm->apple )
	    ++mmw.instance_count;		/* Normal (default) design is a master in the mac format */
    } else {
	mmw.mm = chunkalloc(sizeof(MMSet));
	mmw.axis_count = 1;
	mmw.instance_count = 2;
	mmw.mm->axis_count = 4; mmw.mm->instance_count = AppleMmMax+1;
	mmw.mm->instances = gcalloc(AppleMmMax+1,sizeof(SplineFont *));
	mmw.mm->positions = gcalloc((AppleMmMax+1)*4,sizeof(real));
	mmw.mm->defweights = gcalloc(AppleMmMax+1,sizeof(real));
	mmw.mm->axismaps = gcalloc(4,sizeof(struct axismap));
	mmw.isnew = true;
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = mmw.isnew?_("Create MM..."):_("MM _Info...") ;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(MMW_Width));
    pos.height = GDrawPointsToPixels(NULL,MMW_Height);
    mmw.gw = gw = GDrawCreateTopWindow(NULL,&pos,mmw_e_h,&mmw,&wattrs);

    memset(&blabel,0,sizeof(blabel));
    memset(&bgcd,0,sizeof(bgcd));

    mmw.canceldrop = GDrawPointsToPixels(NULL,30);
    bgcd[0].gd.pos.x = 20; bgcd[0].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-33;
    bgcd[0].gd.pos.width = -1; bgcd[0].gd.pos.height = 0;
    bgcd[0].gd.flags = gg_visible | gg_enabled;
    blabel[0].text = (unichar_t *) _("_OK");
    blabel[0].text_is_1byte = true;
    blabel[0].text_in_resource = true;
    bgcd[0].gd.label = &blabel[0];
    bgcd[0].gd.cid = CID_OK;
    bgcd[0].gd.handle_controlevent = MMW_OK;
    bgcd[0].creator = GButtonCreate;

    space = (MMW_Width-4*blen-40)/3;
    bgcd[1].gd.pos.x = bgcd[0].gd.pos.x+blen+space; bgcd[1].gd.pos.y = bgcd[0].gd.pos.y;
    bgcd[1].gd.pos.width = -1; bgcd[1].gd.pos.height = 0;
    bgcd[1].gd.flags = gg_visible;
    blabel[1].text = (unichar_t *) _("< _Prev");
    blabel[1].text_is_1byte = true;
    blabel[1].text_in_resource = true;
    bgcd[1].gd.label = &blabel[1];
    bgcd[1].gd.handle_controlevent = MMW_Prev;
    bgcd[1].gd.cid = CID_Prev;
    bgcd[1].creator = GButtonCreate;

    bgcd[2].gd.pos.x = bgcd[1].gd.pos.x+blen+space; bgcd[2].gd.pos.y = bgcd[1].gd.pos.y;
    bgcd[2].gd.pos.width = -1; bgcd[2].gd.pos.height = 0;
    bgcd[2].gd.flags = gg_visible;
    blabel[2].text = (unichar_t *) _("_Next >");
    blabel[2].text_in_resource = true;
    blabel[2].text_is_1byte = true;
    bgcd[2].gd.label = &blabel[2];
    bgcd[2].gd.handle_controlevent = MMW_Next;
    bgcd[2].gd.cid = CID_Next;
    bgcd[2].creator = GButtonCreate;

    bgcd[3].gd.pos.x = -20; bgcd[3].gd.pos.y = bgcd[1].gd.pos.y;
    bgcd[3].gd.pos.width = -1; bgcd[3].gd.pos.height = 0;
    bgcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    blabel[3].text = (unichar_t *) _("_Cancel");
    blabel[3].text_in_resource = true;
    blabel[3].text_is_1byte = true;
    bgcd[3].gd.label = &blabel[3];
    bgcd[3].gd.handle_controlevent = MMW_Cancel;
    bgcd[3].gd.cid = CID_Cancel;
    bgcd[3].creator = GButtonCreate;

    bgcd[4].gd.pos.x = 2; bgcd[4].gd.pos.y = 2;
    bgcd[4].gd.pos.width = pos.width-4; bgcd[4].gd.pos.height = pos.height-4;
    bgcd[4].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    bgcd[4].gd.cid = CID_Group;
    bgcd[4].creator = GGroupCreate;

    GGadgetsCreate(gw,bgcd);

    subpos = pos;
    subpos.y = subpos.x = 4;
    subpos.width -= 8;
    mmw.subheightdiff = GDrawPointsToPixels(NULL,44)-8;
    subpos.height -= mmw.subheightdiff;
    wattrs.mask = wam_events;
    for ( i=mmw_counts; i<=mmw_others; ++i )
	mmw.subwins[i] = GWidgetCreateSubWindow(mmw.gw,&subpos,mmwsub_e_h,&mmw,&wattrs);

    memset(&cntlabel,0,sizeof(cntlabel));
    memset(&cntgcd,0,sizeof(cntgcd));

    k=0;
    cntlabel[k].text = (unichar_t *) _("Type of distortable font:");
    cntlabel[k].text_is_1byte = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 5; cntgcd[k].gd.pos.y = 11;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k++].creator = GLabelCreate;

    cntlabel[k].text = (unichar_t *) _("Adobe");
    cntlabel[k].text_is_1byte = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 10; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+12;
    cntgcd[k].gd.flags = isadobe ? (gg_visible | gg_enabled | gg_cb_on) :
	    ( gg_visible | gg_enabled );
    cntgcd[k].gd.cid = CID_Adobe;
    cntgcd[k].gd.handle_controlevent = MMW_TypeChanged;
    cntgcd[k++].creator = GRadioCreate;

    cntlabel[k].text = (unichar_t *) _("Apple");
    cntlabel[k].text_is_1byte = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 70; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y;
    cntgcd[k].gd.flags = !isadobe ? (gg_visible | gg_enabled | gg_cb_on) :
	    ( gg_visible | gg_enabled );
    cntgcd[k].gd.cid = CID_Apple;
    cntgcd[k].gd.handle_controlevent = MMW_TypeChanged;
    cntgcd[k++].creator = GRadioCreate;

    cntlabel[k].text = (unichar_t *) _("Number of Axes:");
    cntlabel[k].text_is_1byte = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 5; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+18;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k++].creator = GLabelCreate;

    cntgcd[k].gd.pos.x = 10; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+12;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k].gd.u.list = axiscounts;
    cntgcd[k].gd.label = &axiscounts[mmw.axis_count-1];
    cntgcd[k].gd.cid = CID_AxisCount;
    cntgcd[k].gd.handle_controlevent = MMW_AxisCntChanged;
    cntgcd[k++].creator = GListButtonCreate;
    for ( i=0; i<4; ++i )
	axiscounts[i].selected = false;
    axiscounts[mmw.axis_count-1].selected = true;

    cntlabel[k].text = (unichar_t *) _("Number of Master Designs:");
    cntlabel[k].text_is_1byte = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 5; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+30;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k++].creator = GLabelCreate;

    cntgcd[k].gd.pos.x = 10; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+12;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k].gd.u.list = mastercounts;
    cntgcd[k].gd.label = &mastercounts[mmw.instance_count-1];
    cntgcd[k].gd.cid = CID_MasterCount;
    cntgcd[k++].creator = GListButtonCreate;
    for ( i=0; i<AppleMmMax+1; ++i )
	mastercounts[i].selected = false;
    mastercounts[mmw.instance_count-1].selected = true;

    cntlabel[k].text = (unichar_t *) _("_Family Name:");
    cntlabel[k].text_is_1byte = true;
    cntlabel[k].text_in_resource = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 10; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+30;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k++].creator = GLabelCreate;

    freeme=NULL;
    if ( mmw.old!=NULL && mmw.old->normal->familyname!=NULL )
	cntlabel[k].text = (unichar_t *) (mmw.old->normal->familyname);
    else
	cntlabel[k].text = (unichar_t *) (freeme= GetNextUntitledName());
    cntlabel[k].text_is_1byte = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 15; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+14;
    cntgcd[k].gd.pos.width = 150;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k].gd.cid = CID_FamilyName;
    cntgcd[k++].creator = GTextFieldCreate;

    GGadgetsCreate(mmw.subwins[mmw_counts],cntgcd);
    SetMasterToAxis(&mmw,true);
    free(freeme);

    memset(&axisgcd,0,sizeof(axisgcd));
    memset(&axislabel,0,sizeof(axislabel));
    memset(&agcd,0,sizeof(agcd));
    memset(&axisaspects,0,sizeof(axisaspects));

    for ( i=0; i<4; ++i ) {
	k=0;
	axislabel[i][k].text = (unichar_t *) _("Axis Type:");
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 5; axisgcd[i][k].gd.pos.y = 11;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	axisgcd[i][k].gd.pos.x = 120; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y-4;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.u.list = axistypes;
	axisgcd[i][k].gd.cid = CID_AxisType+i*100;
	axisgcd[i][k].gd.handle_controlevent = MMW_CheckOptical;
	if ( i<mmw.axis_count && mmw.mm->axes[i]!=NULL ) {
	    axislabel[i][k].text = uc_copy(mmw.mm->axes[i]);
	    axisgcd[i][k].gd.label = &axislabel[i][k];
	}
	axisgcd[i][k++].creator = GListFieldCreate;

	axislabel[i][k].text = (unichar_t *) _("Axis Range:");
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 5; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+20;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	if ( mmw.mm->axismaps[i].points<2 ) {
	    strcpy(axisbegins[i],"50");
	    strcpy(axisdefs[i],"400");
	    strcpy(axisends[i],"999");
	} else {
	    sprintf(axisbegins[i],"%.4g", (double) mmw.mm->axismaps[i].designs[0]);
	    sprintf(axisends[i],"%.4g", (double) mmw.mm->axismaps[i].designs[mmw.mm->axismaps[i].points-1]);
	    if ( mmw.mm->apple )
		sprintf(axisdefs[i],"%.4g", (double) mmw.mm->axismaps[i].def );
	    else
		sprintf(axisdefs[i],"%g", (double) (mmw.mm->axismaps[i].designs[0]+
			mmw.mm->axismaps[i].designs[mmw.mm->axismaps[i].points-1])/2);
	}

	axislabel[i][k].text = (unichar_t *) _("Begin:");
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 10; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+16;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	axislabel[i][k].text = (unichar_t *) axisbegins[i];
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 50; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y-4;
	axisgcd[i][k].gd.pos.width=50;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.cid = CID_AxisBegin+i*100;
	axisgcd[i][k++].creator = GTextFieldCreate;

	axislabel[i][k].text = (unichar_t *) _("Default:");
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 110; axisgcd[i][k].gd.pos.y = axisgcd[i][k-2].gd.pos.y;
	axisgcd[i][k].gd.flags = mmw.mm->apple ? (gg_visible | gg_enabled) : gg_visible;
	axisgcd[i][k].gd.cid = CID_AxisDefaultLabel+i*100;
	axisgcd[i][k++].creator = GLabelCreate;

	axislabel[i][k].text = (unichar_t *) axisdefs[i];
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 148; axisgcd[i][k].gd.pos.y = axisgcd[i][k-2].gd.pos.y;
	axisgcd[i][k].gd.pos.width=50;
	axisgcd[i][k].gd.flags = mmw.mm->apple ? (gg_visible | gg_enabled) : gg_visible;
	axisgcd[i][k].gd.cid = CID_AxisDefault+i*100;
	axisgcd[i][k++].creator = GTextFieldCreate;

	axislabel[i][k].text = (unichar_t *) _("End:");
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 210; axisgcd[i][k].gd.pos.y = axisgcd[i][k-2].gd.pos.y;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	axislabel[i][k].text = (unichar_t *) axisends[i];
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 240; axisgcd[i][k].gd.pos.y = axisgcd[i][k-2].gd.pos.y;
	axisgcd[i][k].gd.pos.width=50;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.cid = CID_AxisEnd+i*100;
	axisgcd[i][k++].creator = GTextFieldCreate;

	axislabel[i][k].text = (unichar_t *) _("Intermediate Points:");
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 5; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+26;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	normalized[i]=NULL;
	designs[i]=NULL;
	if ( mmw.mm->axismaps[i].points>2+mmw.mm->apple ) {
	    int l,j,len1, len2;
	    char buffer[30];
	    len1 = len2 = 0;
	    for ( l=0; l<2; ++l ) {
		for ( j=1; j<mmw.mm->axismaps[i].points-1; ++j ) {
		    if ( mmw.mm->apple && mmw.mm->axismaps[i].designs[j]==mmw.mm->axismaps[i].def )
		continue;
		    /* I wanted to seperate things with commas, but that isn't*/
		    /*  a good idea in Europe (comma==decimal point) */
		    sprintf(buffer,"%g ",(double) mmw.mm->axismaps[i].designs[j]);
		    if ( designs[i]!=NULL )
			strcpy(designs[i]+len1, buffer );
		    len1 += strlen(buffer);
		    sprintf(buffer,"%g ",(double) mmw.mm->axismaps[i].blends[j]);
		    if ( normalized[i]!=NULL )
			strcpy(normalized[i]+len2, buffer );
		    len2 += strlen(buffer);
		}
		if ( l==0 ) {
		    normalized[i] = galloc(len2+2);
		    designs[i] = galloc(len1+2);
		} else {
		    normalized[i][len2-1] = '\0';
		    designs[i][len1-1] = '\0';
		}
	    }
	}

	axislabel[i][k].text = (unichar_t *) _("Design Settings:");
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 10; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+12;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	if ( designs[i]!=NULL ) {
	    axislabel[i][k].text = (unichar_t *) designs[i];
	    axislabel[i][k].text_is_1byte = true;
	    axisgcd[i][k].gd.label = &axislabel[i][k];
	}
	axisgcd[i][k].gd.pos.x = 120; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y-4;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.cid = CID_IntermediateDesign+i*100;
	axisgcd[i][k++].creator = GTextFieldCreate;

	axislabel[i][k].text = (unichar_t *) _("Normalized Settings:");
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 10; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+28;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	if ( normalized[i]!=NULL ) {
	    axislabel[i][k].text = (unichar_t *) normalized[i];
	    axislabel[i][k].text_is_1byte = true;
	    axisgcd[i][k].gd.label = &axislabel[i][k];
	}
	axisgcd[i][k].gd.pos.x = 120; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y-4;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.cid = CID_IntermediateNormalized+i*100;
	axisgcd[i][k++].creator = GTextFieldCreate;

	k = GCDBuildNames(axisgcd[i],axislabel[i],k,mm==NULL || i>=mm->axis_count ? NULL :
		mm->axismaps[i].axisnames);
	
	axisaspects[i].text = (unichar_t *) _(axistablab[i]);
	axisaspects[i].text_is_1byte = true;
	axisaspects[i].gcd = axisgcd[i];
    }
    axisaspects[0].selected = true;

    agcd[0].gd.pos.x = 3; agcd[0].gd.pos.y = 3;
    agcd[0].gd.pos.width = MMW_Width-10;
    agcd[0].gd.pos.height = MMW_Height-45;
    agcd[0].gd.u.tabs = axisaspects;
    agcd[0].gd.flags = gg_visible | gg_enabled;
    agcd[0].gd.cid = CID_WhichAxis;
    agcd[0].creator = GTabSetCreate;

    GGadgetsCreate(mmw.subwins[mmw_axes],agcd);
    for ( i=0; i<4; ++i ) {
	free(axislabel[i][1].text);
	free(normalized[i]);
	free(designs[i]);
    }

    memset(&designgcd,0,sizeof(designgcd));
    memset(&designlabel,0,sizeof(designlabel));
    memset(&dgcd,0,sizeof(dgcd));
    memset(&dlabel,0,sizeof(dlabel));
    memset(&designaspects,0,sizeof(designaspects));

    for ( i=0; i<AppleMmMax+1; ++i ) {
	designlabel[i][0].text = (unichar_t *) _("Source from which this design is to be taken");
	designlabel[i][0].text_is_1byte = true;
	designgcd[i][0].gd.label = &designlabel[i][0];
	designgcd[i][0].gd.pos.x = 3; designgcd[i][0].gd.pos.y = 4;
	designgcd[i][0].gd.flags = gg_visible | gg_enabled;
	designgcd[i][0].creator = GLabelCreate;

	designgcd[i][1].gd.pos.x = 15; designgcd[i][1].gd.pos.y = 18;
	designgcd[i][1].gd.pos.width = 200;
	designgcd[i][1].gd.flags = gg_visible | gg_enabled;
	designgcd[i][1].gd.cid = CID_DesignFonts + i*DesignScaleFactor;
	designgcd[i][1].gd.handle_controlevent = MMW_CheckBrowse;
	designgcd[i][1].creator = GListButtonCreate;

	designlabel[i][2].text = (unichar_t *) _("Normalized position of this design along each axis");
	designlabel[i][2].text_is_1byte = true;
	designgcd[i][2].gd.label = &designlabel[i][2];
	designgcd[i][2].gd.pos.x = 3; designgcd[i][2].gd.pos.y = 50;
	designgcd[i][2].gd.flags = gg_visible | gg_enabled;
	designgcd[i][2].creator = GLabelCreate;

	designgcd[i][3].gd.pos.x = 15; designgcd[i][3].gd.pos.y = 64;
	designgcd[i][3].gd.pos.width = 200;
	designgcd[i][3].gd.flags = gg_visible | gg_enabled;
	designgcd[i][3].gd.cid = CID_AxisWeights + i*DesignScaleFactor;
	designgcd[i][3].creator = GTextFieldCreate;

	designaspects[i].text = (unichar_t *) designtablab[i];
	designaspects[i].text_is_1byte = true;
	designaspects[i].gcd = designgcd[i];
    }
    designaspects[0].selected = true;

    dlabel.text = (unichar_t *) _("Master Designs");
    dlabel.text_is_1byte = true;
    dgcd[0].gd.label = &dlabel;
    dgcd[0].gd.pos.x = 3; dgcd[0].gd.pos.y = 4;
    dgcd[0].gd.flags = gg_visible | gg_enabled;
    dgcd[0].creator = GLabelCreate;

    dgcd[1].gd.pos.x = 3; dgcd[1].gd.pos.y = 18;
    dgcd[1].gd.pos.width = MMW_Width-10;
    dgcd[1].gd.pos.height = MMW_Height-60;
    dgcd[1].gd.u.tabs = designaspects;
    dgcd[1].gd.flags = gg_visible | gg_enabled;
    dgcd[1].gd.cid = CID_WhichDesign;
    dgcd[1].creator = GTabSetCreate;

    GGadgetsCreate(mmw.subwins[mmw_designs],dgcd);

    memset(&ngcd,0,sizeof(ngcd));
    memset(&nlabel,0,sizeof(nlabel));

    nlabel[0].text = (unichar_t *) _("Named Styles");
    nlabel[0].text_is_1byte = true;
    ngcd[0].gd.label = &nlabel[0];
    ngcd[0].gd.pos.x = 3; ngcd[0].gd.pos.y = 4;
    ngcd[0].gd.flags = gg_visible | gg_enabled;
    ngcd[0].creator = GLabelCreate;

    ngcd[1].gd.pos.x = 3; ngcd[1].gd.pos.y = 18;
    ngcd[1].gd.pos.width = MMW_Width-10;
    ngcd[1].gd.pos.height = MMW_Height-100;
    ngcd[1].gd.u.list = NamedDesigns(&mmw);
    ngcd[1].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    ngcd[1].gd.cid = CID_NamedDesigns;
    ngcd[1].gd.handle_controlevent = MMW_NamedSel;
    ngcd[1].creator = GListCreate;

    ngcd[2].gd.pos.x = 20; ngcd[2].gd.pos.y = ngcd[1].gd.pos.y + ngcd[1].gd.pos.height+5;
    ngcd[2].gd.pos.width = -1;
    ngcd[2].gd.flags = gg_visible | gg_enabled;
    nlabel[2].text = (unichar_t *) _("Design|_New...");
    nlabel[2].text_is_1byte = true;
    nlabel[2].text_in_resource = true;
    ngcd[2].gd.label = &nlabel[2];
    ngcd[2].gd.cid = CID_NamedNew;
    ngcd[2].gd.handle_controlevent = MMW_NamedNew;
    ngcd[2].creator = GButtonCreate;

    ngcd[3].gd.pos.x = 20+blen+10; ngcd[3].gd.pos.y = ngcd[2].gd.pos.y;
    ngcd[3].gd.pos.width = -1;
    ngcd[3].gd.flags = gg_visible;
    nlabel[3].text = (unichar_t *) _("_Delete");
    nlabel[3].text_is_1byte = true;
    nlabel[3].text_in_resource = true;
    ngcd[3].gd.label = &nlabel[3];
    ngcd[3].gd.cid = CID_NamedDelete;
    ngcd[3].gd.handle_controlevent = MMW_NamedDelete;
    ngcd[3].creator = GButtonCreate;

    ngcd[4].gd.pos.x = 20+2*blen+20; ngcd[4].gd.pos.y = ngcd[2].gd.pos.y;
    ngcd[4].gd.pos.width = -1;
    ngcd[4].gd.flags = gg_visible;
    nlabel[4].text = (unichar_t *) _("_Edit...");
    nlabel[4].text_is_1byte = true;
    nlabel[4].text_in_resource = true;
    ngcd[4].gd.label = &nlabel[4];
    ngcd[4].gd.cid = CID_NamedEdit;
    ngcd[4].gd.handle_controlevent = MMW_NamedEdit;
    ngcd[4].creator = GButtonCreate;

    GGadgetsCreate(mmw.subwins[mmw_named],ngcd);
    if ( ngcd[1].gd.u.list!=NULL )
	GTextInfoListFree(ngcd[1].gd.u.list);

    memset(&ogcd,0,sizeof(ogcd));
    memset(&olabels,0,sizeof(olabels));

    k=0;
    olabels[k].text = (unichar_t *) _("Normalize Design Vector Function:");
    olabels[k].text_is_1byte = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 3; ogcd[k].gd.pos.y = 4;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k++].creator = GLabelCreate;

    ogcd[k].gd.pos.x = 3; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+15;
    ogcd[k].gd.pos.width = MMW_Width-10;
    ogcd[k].gd.pos.height = 8*12+10;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_NDV;
    ogcd[k++].creator = GTextAreaCreate;

    olabels[k].text = (unichar_t *) _("Convert Design Vector Function:");
    olabels[k].text_is_1byte = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 3; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+ogcd[k-1].gd.pos.height+5;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k++].creator = GLabelCreate;

    ogcd[k].gd.pos.x = 3; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+15;
    ogcd[k].gd.pos.width = MMW_Width-10;
    ogcd[k].gd.pos.height = 8*12+10;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_CDV;
    ogcd[k++].creator = GTextAreaCreate;

    GGadgetsCreate(mmw.subwins[mmw_funcs],ogcd);

    memset(&ogcd,0,sizeof(ogcd));
    memset(&olabels,0,sizeof(olabels));

    k=0;
    olabels[k].text = (unichar_t *) _("Contribution of each master design");
    olabels[k].text_is_1byte = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 10; ogcd[k].gd.pos.y = 4;
    ogcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    ogcd[k].gd.cid = CID_Explicit;
    ogcd[k].gd.handle_controlevent = MMCB_Changed;
    ogcd[k++].creator = GRadioCreate;

    olabels[k].text = (unichar_t *) _("Design Axis Values");
    olabels[k].text_is_1byte = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 10; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+45;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_ByDesign;
    ogcd[k].gd.handle_controlevent = MMCB_Changed;
    ogcd[k++].creator = GRadioCreate;

    ogcd[k].gd.pos.x = 15; ogcd[k].gd.pos.y = ogcd[k-2].gd.pos.y+18;
    ogcd[k].gd.pos.width = 240;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_NewBlends;
    ogcd[k++].creator = GTextFieldCreate;

    ogcd[k].gd.pos.x = 15; ogcd[k].gd.pos.y = ogcd[k-2].gd.pos.y+18;
    ogcd[k].gd.pos.width = 240;
    ogcd[k].gd.flags = gg_visible;
    ogcd[k].gd.cid = CID_NewDesign;
    ogcd[k++].creator = GTextFieldCreate;

    olabels[k].text = (unichar_t *) _("Force Bold Threshold:");
    olabels[k].text_is_1byte = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 10; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+45;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k++].creator = GLabelCreate;

    if ( mmw.old!=NULL &&
	    (pt = PSDictHasEntry(mmw.old->normal->private,"ForceBoldThreshold"))!=NULL )
	olabels[k].text = (unichar_t *) pt;
    else
	olabels[k].text = (unichar_t *) ".3";
    olabels[k].text_is_1byte = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 15; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+13;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_ForceBoldThreshold;
    ogcd[k++].creator = GTextFieldCreate;

    GGadgetsCreate(mmw.subwins[mmw_others],ogcd);

    mmw.state = mmw_counts;
    MMW_SetState(&mmw);
    GDrawSetVisible(mmw.subwins[mmw.state],true);
    GDrawSetVisible(mmw.gw,true);

    while ( !mmw.done )
	GDrawProcessOneEvent(NULL);

    GDrawDestroyWindow(gw);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
