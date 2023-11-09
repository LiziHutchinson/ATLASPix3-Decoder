/********************************************************
* LangauCint.cxx
********************************************************/
#include "LangauCint.h"

#ifdef G__MEMTEST
#undef malloc
#endif


extern "C" void G__set_cpp_environmentLangauCint() {
  G__add_macro("__MAKECINT__");
  G__add_compiledheader("TROOT.h");
  G__add_compiledheader("TMemberInspector.h");
  G__add_compiledheader("Langau.h");
}
extern "C" int G__cpp_dllrevLangauCint() { return(51111); }

/*********************************************************
* Member function Interface Method
*********************************************************/

/* Setting up global function */
static int G___langaufit_0_12(G__value *result7,char *funcname,struct G__param *libp,int hash) {
      G__letint(result7,85,(long)langaufit((TH1F*)G__int(libp->para[0]),(Double_t*)G__int(libp->para[1])
,(Double_t*)G__int(libp->para[2]),(Double_t*)G__int(libp->para[3])
,(Double_t*)G__int(libp->para[4]),(Double_t*)G__int(libp->para[5])));
   return(1 || funcname || hash || result7 || libp) ;
}

static int G___langaufun_1_12(G__value *result7,char *funcname,struct G__param *libp,int hash) {
      G__letdouble(result7,100,(double)langaufun((Double_t*)G__int(libp->para[0]),(Double_t*)G__int(libp->para[1])));
   return(1 || funcname || hash || result7 || libp) ;
}

static int G___langaupro_2_12(G__value *result7,char *funcname,struct G__param *libp,int hash) {
      G__letint(result7,105,(long)langaupro((Double_t*)G__int(libp->para[0]),libp->para[1].ref?*(Double_t*)libp->para[1].ref:G__Mdouble(libp->para[1])
,libp->para[2].ref?*(Double_t*)libp->para[2].ref:G__Mdouble(libp->para[2])));
   return(1 || funcname || hash || result7 || libp) ;
}


/*********************************************************
* Member function Stub
*********************************************************/

/*********************************************************
* Global function Stub
*********************************************************/

/*********************************************************
* Get size of pointer to member function
*********************************************************/
class G__Sizep2memfuncLangauCint {
 public:
  G__Sizep2memfuncLangauCint() {p=&G__Sizep2memfuncLangauCint::sizep2memfunc;}
    size_t sizep2memfunc() { return(sizeof(p)); }
  private:
    size_t (G__Sizep2memfuncLangauCint::*p)();
};

size_t G__get_sizep2memfuncLangauCint()
{
  G__Sizep2memfuncLangauCint a;
  G__setsizep2memfunc((int)a.sizep2memfunc());
  return((size_t)a.sizep2memfunc());
}


/*********************************************************
* virtual base class offset calculation interface
*********************************************************/

   /* Setting up class inheritance */

/*********************************************************
* Inheritance information setup/
*********************************************************/
extern "C" void G__cpp_setup_inheritanceLangauCint() {

   /* Setting up class inheritance */
}

/*********************************************************
* typedef information setup/
*********************************************************/
extern "C" void G__cpp_setup_typetableLangauCint() {

   /* Setting up typedef entry */
   G__search_typename2("Char_t",99,-1,0,
-1);
   G__setnewtype(-1,"Signed Character 1 byte",0);
   G__search_typename2("UChar_t",98,-1,0,
-1);
   G__setnewtype(-1,"Unsigned Character 1 byte",0);
   G__search_typename2("Short_t",115,-1,0,
-1);
   G__setnewtype(-1,"Signed Short integer 2 bytes",0);
   G__search_typename2("UShort_t",114,-1,0,
-1);
   G__setnewtype(-1,"Unsigned Short integer 2 bytes",0);
   G__search_typename2("Int_t",105,-1,0,
-1);
   G__setnewtype(-1,"Signed integer 4 bytes",0);
   G__search_typename2("UInt_t",104,-1,0,
-1);
   G__setnewtype(-1,"Unsigned integer 4 bytes",0);
   G__search_typename2("Seek_t",105,-1,0,
-1);
   G__setnewtype(-1,"File pointer",0);
   G__search_typename2("Long_t",108,-1,0,
-1);
   G__setnewtype(-1,"Signed long integer 8 bytes",0);
   G__search_typename2("ULong_t",107,-1,0,
-1);
   G__setnewtype(-1,"Unsigned long integer 8 bytes",0);
   G__search_typename2("Float_t",102,-1,0,
-1);
   G__setnewtype(-1,"Float 4 bytes",0);
   G__search_typename2("Double_t",100,-1,0,
-1);
   G__setnewtype(-1,"Float 8 bytes",0);
   G__search_typename2("Text_t",99,-1,0,
-1);
   G__setnewtype(-1,"General string",0);
   G__search_typename2("Bool_t",98,-1,0,
-1);
   G__setnewtype(-1,"Boolean (0=false, 1=true)",0);
   G__search_typename2("Byte_t",98,-1,0,
-1);
   G__setnewtype(-1,"Byte (8 bits)",0);
   G__search_typename2("Version_t",115,-1,0,
-1);
   G__setnewtype(-1,"Class version identifier",0);
   G__search_typename2("Option_t",99,-1,0,
-1);
   G__setnewtype(-1,"Option string",0);
   G__search_typename2("Ssiz_t",105,-1,0,
-1);
   G__setnewtype(-1,"String size",0);
   G__search_typename2("Real_t",102,-1,0,
-1);
   G__setnewtype(-1,"TVector and TMatrix element type",0);
   G__search_typename2("VoidFuncPtr_t",89,-1,0,
-1);
   G__setnewtype(-1,"pointer to void function",0);
   G__search_typename2("FreeHookFun_t",89,-1,0,
-1);
   G__setnewtype(-1,NULL,0);
   G__search_typename2("ReAllocFun_t",81,-1,0,
-1);
   G__setnewtype(-1,NULL,0);
   G__search_typename2("ReAllocCFun_t",81,-1,0,
-1);
   G__setnewtype(-1,NULL,0);
   G__search_typename2("Axis_t",102,-1,0,
-1);
   G__setnewtype(-1,"Axis values type",0);
   G__search_typename2("Stat_t",100,-1,0,
-1);
   G__setnewtype(-1,"Statistics type",0);
   G__search_typename2("Font_t",115,-1,0,
-1);
   G__setnewtype(-1,"Font number",0);
   G__search_typename2("Style_t",115,-1,0,
-1);
   G__setnewtype(-1,"Style number",0);
   G__search_typename2("Marker_t",115,-1,0,
-1);
   G__setnewtype(-1,"Marker number",0);
   G__search_typename2("Width_t",115,-1,0,
-1);
   G__setnewtype(-1,"Line width",0);
   G__search_typename2("Color_t",115,-1,0,
-1);
   G__setnewtype(-1,"Color number",0);
   G__search_typename2("SCoord_t",115,-1,0,
-1);
   G__setnewtype(-1,"Screen coordinates",0);
   G__search_typename2("Coord_t",102,-1,0,
-1);
   G__setnewtype(-1,"Pad world coordinates",0);
   G__search_typename2("Angle_t",102,-1,0,
-1);
   G__setnewtype(-1,"Graphics angle",0);
   G__search_typename2("Size_t",102,-1,0,
-1);
   G__setnewtype(-1,"Attribute size",0);
   G__search_typename2("Double_t (*)(Double_t*, Double_t*)",81,-1,0,
-1);
   G__setnewtype(-1,NULL,0);
}

/*********************************************************
* Data Member information setup/
*********************************************************/

   /* Setting up class,struct,union tag member variable */
extern "C" void G__cpp_setup_memvarLangauCint() {
}
/***********************************************************
************************************************************
************************************************************
************************************************************
************************************************************
************************************************************
************************************************************
***********************************************************/

/*********************************************************
* Member function information setup for each class
*********************************************************/

/*********************************************************
* Member function information setup
*********************************************************/
extern "C" void G__cpp_setup_memfuncLangauCint() {
}

/*********************************************************
* Global variable information setup for each class
*********************************************************/
extern "C" void G__cpp_setup_globalLangauCint() {

   /* Setting up global variables */
   G__resetplocal();


   G__resetglobalenv();
}

/*********************************************************
* Global function information setup for each class
*********************************************************/
extern "C" void G__cpp_setup_funcLangauCint() {
   G__lastifuncposition();

   G__memfunc_setup("langaufit",955,G___langaufit_0_12,85,G__get_linked_tagnum(&G__LangauCintLN_TF1),-1,0,6,1,1,0,
"U 'TH1F' - 0 - his D - 'Double_t' 0 - fitrange "
"D - 'Double_t' 0 - startvalues D - 'Double_t' 0 - parlimitslo "
"D - 'Double_t' 0 - parlimitshi D - 'Double_t' 0 - fitparams",(char*)NULL
,(void*)NULL,0);
   G__memfunc_setup("langaufun",961,G___langaufun_1_12,100,-1,G__defined_typename("Double_t"),0,2,1,1,0,
"D - 'Double_t' 0 - x D - 'Double_t' 0 - par",(char*)NULL
,(void*)NULL,0);
   G__memfunc_setup("langaupro",969,G___langaupro_2_12,105,-1,G__defined_typename("Int_t"),0,3,1,1,0,
"D - 'Double_t' 0 - params d - 'Double_t' 1 - maxx "
"d - 'Double_t' 1 - FWHM",(char*)NULL
,(void*)NULL,0);

   G__resetifuncposition();
}

/*********************************************************
* Class,struct,union,enum tag information setup
*********************************************************/
/* Setup class/struct taginfo */
G__linked_taginfo G__LangauCintLN_TF1 = { "TF1" , 99 , -1 };

extern "C" void G__cpp_setup_tagtableLangauCint() {

   /* Setting up class,struct,union tag entry */
}
extern "C" void G__cpp_setupLangauCint() {
  G__check_setup_version(51111,"G__cpp_setupLangauCint()");
  G__set_cpp_environmentLangauCint();
  G__cpp_setup_tagtableLangauCint();

  G__cpp_setup_inheritanceLangauCint();

  G__cpp_setup_typetableLangauCint();

  G__cpp_setup_memvarLangauCint();

  G__cpp_setup_memfuncLangauCint();
  G__cpp_setup_globalLangauCint();
  G__cpp_setup_funcLangauCint();

   if(0==G__getsizep2memfunc()) G__get_sizep2memfuncLangauCint();
  return;
}
class G__cpp_setup_initLangauCint {
  public:
    G__cpp_setup_initLangauCint() { G__add_setup_func("LangauCint",&G__cpp_setupLangauCint); }
   ~G__cpp_setup_initLangauCint() { G__remove_setup_func("LangauCint"); }
};
G__cpp_setup_initLangauCint G__cpp_setup_initializerLangauCint;

//
// File generated by rootcint at Tue Mar 16 19:55:25 1999.
// Do NOT change. Changes will be lost next time file is generated
//

#include "TBuffer.h"
#include "TMemberInspector.h"
#include "TError.h"

