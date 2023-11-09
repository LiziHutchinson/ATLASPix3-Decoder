//-----------------------------------------------------------------------
//
//	Convoluted Landau and Gaussian Fitting Function
//       for ROOT (using ROOT's Landau and Gauss functions)
//
//  Based on a Fortran code by R.Fruehwirth (fruhwirth@hephy.oeaw.ac.at)
//  Adapted for C++/ROOT by H.Pernegger (Heinz.Pernegger@cern.ch) and
//   Markus Friedl (Markus.Friedl@cern.ch)
//
//  Mar 15, 1999
//
//  See Langau.cxx for function details
//
//-----------------------------------------------------------------------


// Root includes
#include "Rtypes.h"
#include "TF1.h"

extern "C" TF1 *langaufit(TH1F *his, Double_t *fitrange, Double_t *startvalues, Double_t *parlimitslo, Double_t *parlimitshi, Double_t fitparams[4]);

extern "C" Double_t langaufun(Double_t *x, Double_t *par);

extern "C" Int_t langaupro(Double_t *params, Double_t &maxx, Double_t &FWHM);










