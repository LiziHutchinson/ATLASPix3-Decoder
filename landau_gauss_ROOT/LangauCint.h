/********************************************************************
* LangauCint.h
********************************************************************/
#ifdef __CINT__
#error LangauCint.h/C is only for compilation. Abort cint.
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
extern "C" {
#define G__ANSIHEADER
#include "G__ci.h"
extern void G__cpp_setup_tagtableLangauCint();
extern void G__cpp_setup_inheritanceLangauCint();
extern void G__cpp_setup_typetableLangauCint();
extern void G__cpp_setup_memvarLangauCint();
extern void G__cpp_setup_globalLangauCint();
extern void G__cpp_setup_memfuncLangauCint();
extern void G__cpp_setup_funcLangauCint();
extern void G__set_cpp_environmentLangauCint();
}


#include "TROOT.h"
#include "TMemberInspector.h"
#include "Langau.h"

#ifndef G__MEMFUNCBODY
#endif

extern G__linked_taginfo G__LangauCintLN_TF1;
