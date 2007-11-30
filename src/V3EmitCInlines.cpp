// $Id$
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
//
// Code available from: http://www.veripool.com/verilator
//
// AUTHORS: Wilson Snyder with Paul Wasson, Duane Gabli
//
//*************************************************************************
//
// Copyright 2003-2007 by Wilson Snyder.  This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// General Public License or the Perl Artistic License.
//
// Verilator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//*************************************************************************

#include "config_build.h"
#include "verilatedos.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include <map>
#include <vector>

#include "V3Global.h"
#include "V3EmitC.h"
#include "V3EmitCBase.h"
#include "V3Stats.h"

#define EMITCINLINES_NUM_CONSTW	10	// Number of VL_CONST_W_*X's in verilated.h (IE VL_CONST_W_9X is last)

//######################################################################

class EmitCInlines : EmitCBaseVisitor {
    // STATE
    vector<V3Double0>	m_wordWidths;	// What sizes are used?

    // METHODS
    void emitInt();

    // VISITORS
    virtual void visit(AstVar* nodep, AstNUser*) {
	// All wide constants load into variables, so we can just hunt for them
	if (nodep->widthWords() >= EMITCINLINES_NUM_CONSTW ) {
	    if (int(m_wordWidths.size()) <= nodep->widthWords()) {
		m_wordWidths.resize(nodep->widthWords()+5);
	    }
	    ++ m_wordWidths.at(nodep->widthWords());
	    v3Global.needHInlines(true);
	}
    }

    // NOPs
    virtual void visit(AstNodeStmt*, AstNUser*) {}
    // Default
    virtual void visit(AstNode* nodep, AstNUser*) {
	nodep->iterateChildren(*this);
    }
    //---------------------------------------
    // ACCESSORS
public:
    EmitCInlines(AstNetlist* nodep) {
	nodep->accept(*this);
	if (v3Global.needHInlines()) {
	    emitInt();
	}
    }
};

void EmitCInlines::emitInt() {
    string filename = v3Global.opt.makeDir()+"/"+topClassName()+"__Inlines.h";
    newCFile(filename, false/*slow*/, false/*source*/);
    V3OutCFile hf (filename);
    m_ofp = &hf;

    ofp()->putsHeader();
    puts("#ifndef _"+topClassName()+"__Inlines_H_\n");
    puts("#define _"+topClassName()+"__Inlines_H_\n");
    puts("\n");

    puts("#include \"verilated.h\"\n");

    puts("\n//======================\n\n");

    for (unsigned words=0; words<m_wordWidths.size(); words++) {
	if (m_wordWidths.at(words)) {
	    puts("#ifndef VL_HAVE_CONST_W_"+cvtToStr(words)+"X\n");
	    puts("# define VL_HAVE_CONST_W_"+cvtToStr(words)+"X\n");
	    puts("static inline WDataOutP VL_CONST_W_"+cvtToStr(words)+"X(int obits, WDataOutP o\n");
	    puts("\t");
	    for (int i=words-1; i>=0; --i) {
		puts(",IData d"+cvtToStr(i));
		if (i && (i % 8 == 0)) puts("\n\t");
	    }
	    puts(") {\n");
	    puts("   ");
	    for (int i=words-1; i>=0; --i) {
		puts(" o["+cvtToStr(i)+"]=d"+cvtToStr(i)+";");
		if (i && (i % 8 == 0)) puts("\n   ");
	    }
	    puts("\n");
	    puts("    for(int i="+cvtToStr(words)+";i<VL_WORDS_I(obits);i++) o[i] = (IData)0x0;\n");
	    puts("    return o;\n");
	    puts("}\n");
	    puts("#endif\n");
	    puts("\n");
	}
    }

    puts("//======================\n\n");
    puts("#endif  /*guard*/\n");
}

//######################################################################
// EmitC class functions

void V3EmitC::emitcInlines() {
    UINFO(2,__FUNCTION__<<": "<<endl);
    EmitCInlines syms (v3Global.rootp());
}