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
//-----------------------------------------------------------------------


#ifndef __CINT__

// Root includes
#include "Rtypes.h"
#include "TMath.h"
#include "TH1.h"
#include "TF1.h"
#include "TROOT.h"

#include "Langau.h"

#endif


Double_t langaufun(Double_t *x, Double_t *par)

/*
   Fit parameters:
   par[0]=Width (scale) parameter of Landau density
   par[1]=Most Probable (MP, location) parameter of Landau density
   par[2]=Total area (integral -inf to inf, normalization constant)
   par[3]=Width (sigma) of convoluted Gaussian function

   In the Landau distribution (represented by the CERLIB approximation), 
   the maximum is located at x=-0.22278298 with the location parameter=0.
   This shift is corrected within this function, so that the actual
   maximum is identical to the MP parameter.
*/

{ 
      // Numeric constants
      Double_t invsq2pi = 0.3989422804014;   // (2 pi)^(-1/2)
      Double_t mpshift  = -0.22278298;       // Landau maximum location

      // Control constants
      Double_t np = 100.0;      // number of convolution steps
      Double_t sc =   5.0;      // convolution extends to +-sc Gaussian sigmas

      // Variables
      Double_t xx;
      Double_t mpc;
      Double_t fland;
      Double_t sum = 0.0;
      Double_t xlow,xupp;
      Double_t step;
      Double_t i;


      // MP shift correction
      mpc = par[1] - mpshift * par[0]; 

      // Range of convolution integral
      xlow = x[0] - sc * par[3];
      xupp = x[0] + sc * par[3];

      step = (xupp-xlow) / np;

      // Convolution integral of Landau and Gaussian by sum
      for(i=1.0; i<=np/2; i++)
      {
         xx = xlow + (i-.5) * step;
         fland = TMath::Landau(xx,mpc,par[0]) / par[0];
         sum += fland * TMath::Gaus(x[0],xx,par[3]);

         xx = xupp - (i-.5) * step;
         fland = TMath::Landau(xx,mpc,par[0]) / par[0];
         sum += fland * TMath::Gaus(x[0],xx,par[3]);
      }

      return (par[2] * step * sum * invsq2pi / par[3]);

}



TF1 *langaufit(TH1F *his, Double_t *fitrange, Double_t *startvalues, Double_t *parlimitslo, Double_t *parlimitshi, Double_t *fitparams)

/*
   Once again, here are the Landau * Gaussian parameters:
   par[0]=Width (scale) parameter of Landau density
   par[1]=Most Probable (MP, location) parameter of Landau density
   par[2]=Total area (integral -inf to inf, normalization constant)
   par[3]=Width (sigma) of convoluted Gaussian function

   Variables for langaufit call:
   his             histogram to fit
   fitrange[2]     lo and hi boundaries of fit range
   startvalues[4]  reasonable start values for the fit
   parlimitslo[4]  lower parameter limits
   parlimitshi[4]  upper parameter limits
   fitparams[4]    returns the final fit parameters

   Fixing a parameter:
   Fixing a parameter (e.g., the Gaussian sigma) is a tricky thing due to
   a bug in Root 2.20/06. (According to 
   http://root.cern.ch/root/roottalk/roottalk98/2472.html and 
   http://root.cern.ch/root/html/examples/V2.21.txt.html I guess
   this has been fixed in version 2.21/03.)
   However, it is still possible to fix a parameter with version 2.20/06.
   Equal parlimitlo and parlimithi settings just results in a floating
   point exception. Thus, set
     startvalue[i] = parlimitlo[i] = fixed value
     parlimithi[i] = 0.99999999999 * parlimitlo[i]
   Migrad (the Root minimization routine) detects that parlimitlo > parlimithi.
   For some strange reason, all three values must be close together, otherwise
   the parameter is still fixed, but at a wrong value (wherever that comes
   from). If this still doesn't work, exchange the hi and lo limits in the
   example above (then lo<hi). Then, the parameter is not really fixed, but
   quite tightened up in a very small range, which should result in almost the
   same fit.
*/

{

   Int_t i;
   Char_t FunName[100];


   sprintf(FunName,"Fitfcn_%s",his->GetName());

   TF1 *ffitold = (TF1*)gROOT->GetListOfFunctions()->FindObject(FunName);
   if (ffitold) delete ffitold;

   TF1 *ffit = new TF1(FunName,langaufun,fitrange[0],fitrange[1],4);
   ffit->SetParameters(startvalues);
   ffit->SetParNames("Width ","MP    ","Area  ","GSigma");
   
   for (i=0; i<4; i++)
   {
      ffit->SetParLimits(i, parlimitslo[i], parlimitshi[i]);
   }

   his->Fit(FunName,"RB0");   // fit within specified range, use ParLimits, do not plot

   ffit->GetParameters(fitparams);    // obtain fit parameters

   return (ffit);              // return fit function

}




Int_t langaupro(Double_t *params, Double_t &maxx, Double_t &FWHM)

/*
   Seaches for the location (x value) at the maximum of the 
   Landau-Gaussian convolute and its full width at half-maximum.

   The search is probably not very efficient, but it's a first try.
*/

{

   Double_t p,x,fy,fxr,fxl;
   Double_t step;
   Double_t l,lold;
   Int_t i = 0;
   Int_t MAXCALLS = 10000;


   // Search for maximum

   p = params[1] - 0.1 * params[0];
   step = 0.05 * params[0];
   lold = -2.0;
   l    = -1.0;


   while ( (l != lold) && (i < MAXCALLS) )
   {
//      printf("i=%5d  x=%15.12f  step=%15.12f  l=%20.18f \n",i,p,step,l);
      i++;

      lold = l;
      x = p + step;
      l = langaufun(&x,params);
 
      if (l < lold)
         step = -step/10;
 
      p += step;
   }

   if (i == MAXCALLS)
      return (-1);

   maxx = x;

   fy = l/2;


   // Search for right x location of fy

   p = maxx + params[0];
   step = params[0];
   lold = -2.0;
   l    = -1e300;
   i    = 0;


   while ( (l != lold) && (i < MAXCALLS) )
   {
//    printf("i=%5d  x=%15.12f  step=%15.12f  l=%20.18f \n",i,p,step,l);
      i++;

      lold = l;
      x = p + step;
      l = TMath::Abs(langaufun(&x,params) - fy);
 
      if (l > lold)
         step = -step/10;
 
      p += step;
   }

   if (i == MAXCALLS)
      return (-2);

   fxr = x;


   // Search for left x location of fy

   p = maxx - 0.5 * params[0];
   step = -params[0];
   lold = -2.0;
   l    = -1e300;
   i    = 0;

   while ( (l != lold) && (i < MAXCALLS) )
   {
//      printf("i=%5d  x=%15.12f  step=%15.12f  l=%20.18f \n",i,p,step,l);
      i++;

      lold = l;
      x = p + step;
      l = TMath::Abs(langaufun(&x,params) - fy);
 
      if (l > lold)
         step = -step/10;
 
      p += step;
   }

   if (i == MAXCALLS)
      return (-3);


   fxl = x;

   FWHM = fxr - fxl;

//printf("\nmaxx=%12.5f     FWHM=%12.5f\n\n",maxx,FWHM);

   return (0);

}

