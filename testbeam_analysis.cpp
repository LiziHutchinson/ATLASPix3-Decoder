/********************************************************
 * Data analysis scripts for ATLASPix3 telescope from   *
 * DESY test beam data in December 2019                 *
 *                                                      *
 * The script is made to be compiled with ROOT 6        *
 *                                                      *
 * Author: Rudolf Schimassek                            *
 * Version: 1.0 (12.12.19)                              *
 *              Initial Version used during test beam   *
 *                  at DESY                             *
 *          1.1 (16.01.20)                              *
 *              started adding more sophisticated       *
 *                  methods                             *
 *          1.2 (24.07.20)                              *
 *              added calibration for ToT and Landau-   *
 *                  Gaussian fits                       *
 *                                                      *
 * The script now relies on the LambertW Implementation *
 *  by Darco Veberic. Publications need to state that!  *
 *                                                      *
 ********************************************************/

#ifndef testbeam_analysis_cpp
#define testbeam_analysis_cpp


#include "TH2I.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TF1.h"
#include "TMinuit.h"
#include "TFile.h"

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <list>
#include <algorithm>
#include <utility>

#include "LambertW-master/LambertW.h"
#include "landau_gauss_ROOT/Langau.cxx"

#include "retrieve_data.cpp"

#include "dataset.cpp"

/*
 * Important: Due to the templates used in the LambertW implementation, it has to
 *              be compiled first in ROOT. Directly compiling it with this script
 *              does not work.
 *
 *              So, before calling the compiler for this script, do
 *
 *                  .L LambertW-master/LambertW.cc+
 */


bool TestFitSuccess()
{
    //check whether the fit converged or not:
    std::string minuitstatus;
    minuitstatus = std::string(gMinuit->fCstatu);

    return minuitstatus.compare("CONVERGED ") == 0 || minuitstatus.compare("OK        ") == 0;
}

/**
 * fits a Landau-Gauß-Distribution to the histogram. To do so, it uses an implementation from CERN which has to
 *      be included manually ("langau.cxx" in "landau_gauss_ROOT/")
 *
 * @param histogram     - the histogram to fit the distribution to
 * @param outputfile    - file to save the resulting parameters to
 * @param draw          - creates a plot of the histogram and the fit if true
 * @return              - a TF1 pointer to the fitted function
 */
TF1* fitLandauGaussToHistogram(TH1* histogram, std::string outputfile = "", bool draw = false,
                               double rangestart = -1e10, double rangeend = 1e10)
{
    if(histogram == 0)
    {
            std::cout << "No histogram passed!" << std::endl;
            return 0;
    }

    //convert the histogram to a TH1F with bin width = 1:
    static int counter = 0;
    std::stringstream index("");
    index << "floathist"<< counter++;
    //TH1F* hist = new TH1F(index.str().c_str(),histogram->GetTitle(),histogram->GetNbinsX(), 0, histogram->GetNbinsX());
    TH1F* hist = new TH1F(index.str().c_str(), histogram->GetTitle(), histogram->GetNbinsX(),
                          histogram->GetBinLowEdge(1), histogram->GetBinLowEdge(histogram->GetNbinsX())
                             + histogram->GetBinWidth(1));

    for(int i=0;i<histogram->GetNbinsX();++i)
        hist->SetBinContent(i, histogram->GetBinContent(i));



    double range[2] = {hist->GetBinLowEdge(0),hist->GetBinLowEdge(hist->GetNbinsX()-1)+hist->GetBinWidth(0)};
    if(rangestart != -1e10)
    //    range[0] = (histogram->GetBinLowEdge(1) - rangestart) / histogram->GetBinWidth(1);
        range[0] = rangestart;
    if(rangeend != 1e10)
    //    range[1] = (histogram->GetBinLowEdge(1) - rangeend) / histogram->GetBinWidth(1);
        range[1] = rangeend;
    double parlimitslo[4] = {0,  hist->GetBinCenter(0), 0, 0};
    double parlimitshi[4] = {10000, hist->GetBinCenter(hist->GetNbinsX()-1), 1000*hist->Integral(), 10000};
    double startvalues[4] = {1000,  hist->GetBinCenter(hist->GetMaximumBin()),
                                 hist->Integral()*hist->GetBinWidth(0), 100};
    double resultvalues[4] = {0, 0, 0, 0};

    TF1* func = langaufit(hist, range, startvalues, parlimitslo, parlimitshi, resultvalues);

    if(draw)
    {
        TCanvas* c = new TCanvas();
        c->Update();

        hist->DrawClone();
        func->SetLineColor(2);
        func->DrawClone("same");
    }

    //transform the time scales / values back:
    //func->SetParameter(0, func->GetParameter(0) * histogram->GetBinWidth(0));
    //func->SetParError(0, func->GetParError(0) * histogram->GetBinWidth(0));

    //func->SetParameter(1, (func->GetParameter(1) + 1) * histogram->GetBinWidth(0)
    //                        + histogram->GetBinLowEdge(1));
    //func->SetParError(1, func->GetParError(1) * histogram->GetBinWidth(0));

    //func->SetParameter(2, func->GetParameter(2) * histogram->GetBinWidth(0));
    //func->SetParError(2, func->GetParError(2) * histogram->GetBinWidth(0));

    //func->SetParameter(3, func->GetParameter(3) * histogram->GetBinWidth(0));
    //func->SetParError(3, func->GetParError(3) * histogram->GetBinWidth(0));

    if(rangestart != -1e10 && rangeend != 1e10)
        func->SetRange(rangestart, rangeend);
    else if(rangestart != -1e10)
        func->SetRange(rangestart,  histogram->GetBinLowEdge(histogram->GetNbinsX() + 1)
                       + histogram->GetBinWidth(0));
    else if(rangeend != 1e10)
        func->SetRange(histogram->GetBinLowEdge(1), rangeend);
    else
        func->SetRange(histogram->GetBinLowEdge(1),
                       histogram->GetBinLowEdge(histogram->GetNbinsX() + 1)
                        + histogram->GetBinWidth(0));

    //output the resuting parameters:
    if(outputfile != "")
    {
        std::fstream f;

        f.open(outputfile.c_str(), std::ios::in);
        if(!f.is_open())
        {
            //add column names on creation of a new file:
            f.open(outputfile.c_str(), std::ios::out | std::ios::app);
            f << "# Parameter Name; Parameter Value; Parameter Error;" << std::endl;
        }
        else
        {
            f.close();
            f.open(outputfile.c_str(), std::ios::out | std::ios::app);
        }


        f << "# Fit result for \"" << histogram->GetTitle() << "\":\n";

        for(int i=0;i<4;++i)
        {
            f << "  " << func->GetParName(i) << "\t" << func->GetParameter(i)
                    << "\t\t" << func->GetParError(i) << std::endl;
        }

        std::string minuitstatus;
        minuitstatus = std::string(gMinuit->fCstatu);

        f << "# Fit Status: \"" << minuitstatus << "\"" << std::endl;

        f.close();
    }

    return func;
}

TCanvas* DrawTGraph(TGraph* gr, TCanvas* c = nullptr, std::string xtitle = "",
                 std::string ytitle = "", std::string drawoptions = "AP")
{
    if(gr == nullptr)
        return nullptr;

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1000,700);
        c->SetLeftMargin(0.12);
        c->SetRightMargin(0.08);
        c->Update();
    }
    else if(drawoptions.find("same") == std::string::npos)
        std::cout << "Warning: drawing to existing canvas without \"same\" option" << std::endl;

    gr->GetXaxis()->SetTitle(xtitle.c_str());
    gr->GetXaxis()->SetTitleSize(0.05);
    gr->GetXaxis()->SetLabelSize(0.05);
    gr->GetYaxis()->SetTitle(ytitle.c_str());
    gr->GetYaxis()->SetTitleSize(0.05);
    gr->GetYaxis()->SetLabelSize(0.05);
    gr->GetYaxis()->SetTitleOffset(1.3);
    gr->SetMarkerStyle(8);
    gr->SetMarkerColor(2);

    gr->Draw(drawoptions.c_str());

    return c;
}

TCanvas* DrawTH1(TH1* hist, TCanvas* c = nullptr, std::string xtitle = "",
                 std::string ytitle = "", std::string drawoptions = "")
{
    if(hist == nullptr)
        return nullptr;

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1000,700);
        c->SetLeftMargin(0.12);
        c->SetRightMargin(0.08);
        c->Update();
    }
    else if(drawoptions.find("same") == std::string::npos)
        std::cout << "Warning: drawing to existing canvas without \"same\" option" << std::endl;

    hist->GetXaxis()->SetTitle(xtitle.c_str());
    hist->GetXaxis()->SetTitleSize(0.05);
    hist->GetXaxis()->SetLabelSize(0.05);
    hist->GetYaxis()->SetTitle(ytitle.c_str());
    hist->GetYaxis()->SetTitleSize(0.05);
    hist->GetYaxis()->SetLabelSize(0.05);
    hist->GetYaxis()->SetTitleOffset(1.3);

    hist->Draw(drawoptions.c_str());

    return c;
}

TCanvas* DrawTH2(TH2* hist, TCanvas* c = nullptr, std::string xtitle = "", std::string ytitle = "",
                 std::string ztitle = "", std::string drawoptions = "colz")
{
    if(hist == nullptr)
        return nullptr;

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1080,700);
        c->SetLeftMargin(0.12/1.08);
        c->SetRightMargin(0.08/1.08+0.08);
        c->Update();
    }
    else if(drawoptions.find("same") == std::string::npos)
        std::cout << "Warning: drawing to existing canvas without \"same\" option" << std::endl;
        
    hist->SetContour(255);

    hist->SetContour(255);

    hist->GetXaxis()->SetTitle(xtitle.c_str());
    hist->GetXaxis()->SetTitleSize(0.05);
    hist->GetXaxis()->SetLabelSize(0.05);
    hist->GetYaxis()->SetTitle(ytitle.c_str());
    hist->GetYaxis()->SetTitleSize(0.05);
    hist->GetYaxis()->SetLabelSize(0.05);
    hist->GetYaxis()->SetTitleOffset(1);
    hist->GetZaxis()->SetTitle(ztitle.c_str());
    hist->GetZaxis()->SetTitleSize(0.05);
    hist->GetZaxis()->SetLabelSize(0.05);

    hist->Draw(drawoptions.c_str());

    return c;
}

TH2* DrawHitMap(std::list<Dataset>* liste, int layer = 0, int groupPixX = 1, int groupPixY = 1,
                  std::string title = "")
{
    if(liste == nullptr)
        return nullptr;

    static int indexcnt = 0;
    std::stringstream sname("");
    sname << "spothist_" << ++indexcnt;
    TH2* hist = new TH2I(sname.str().c_str(),title.c_str(), 132 / groupPixX, -0.5,131.5,
                                                            372 / groupPixY, -0.5, 371.5);

    for(auto& it : *liste)
    {
        if(layer == 0 || it.layer == layer)
            hist->Fill(it.column, it.row);
    }

    return hist;
}

std::list<Dataset>* LoadFile(std::string filename, int maxcounter = 0)
{
    std::fstream f;
    f.open(filename.c_str(), std::ios::in);

    if(!f.is_open())
        return nullptr;

//    TH1* histcol = new TH1I("testcol", "Occurrence of Column 123", 10000, 0, 20000000);
//    TH1* histrow = new TH1I("testrow", "Occurrence of Row 71", 10000, 0, 20000000);

    //get rid of content line at the beginning:
    char text[200];
    f.getline(text, 200, '\n');
    //read the order of the data fields from the header line:
    Dataset fieldorder = DatasetFunctions::FindOrder(text);

    
    std::list<Dataset>* hits = new std::list<Dataset>();
    int counter = 0;

    while(!f.eof() && (counter < maxcounter || maxcounter == 0))
    {
        std::string line = "";
        std::getline(f, line, '\n');
        Dataset dat = DatasetFunctions::Construct(line, fieldorder);
        if(dat.is_valid())
            hits->push_back(dat);
//        if(dat.column == 123 && dat.layer == 3)
//            histcol->Fill(counter);
//        if(dat.row == 71 && dat.layer == 3)
//            histrow->Fill(counter);
        ++counter;
    }

//    DrawTH1(histcol, nullptr, "Time (au)", "Occurrences per Time");
//    DrawTH1(histrow, nullptr, "Time (au)", "Occurrences per Time");

    return hits;
}

/**
 * @brief RemoveInvalidHits removes hits which show non-expectable data from the list
 *      this includes invalid addresses and optionally timestamps too far from the time
 *      stamps of the neighbouring hits in the file.
 *      The function works directly on the passed list, no copy is generated.
 * @param liste              - the list of hits to clear from faulty data
 * @param hard               - remove hits with suspicious time stamps if set to true
 */
void RemoveInvalidHits(std::list<Dataset>* liste, bool hard = false)
{
    if(liste == nullptr)
        return;

    int deletions = 0;

    auto it = liste->begin();

    bool del = false;
    while(it != liste->end())
    {
        if(hard)
        {
            auto itp1 = it;
            ++itp1;
            auto itm1 = it;
            --itm1;

            //1 to 5 hits with TS way too high:
            for(int i = 0; i < 5; ++i)
            {
                if((itp1 != liste->end() && std::abs(it->ts - itp1->ts) > 15e6)
                     && (itm1 != it && std::abs(it->ts - itm1->ts) > 15e6))
                {
                    del = true;
                    break;
                }
                ++itp1;
            }
        }
        if(it->column < 0 || it->column > 131 || it->row < 0 || it->row > 371 || del)
        {
            it = liste->erase(it);
            del = false;
            ++deletions;
        }
        else
            ++it;
    }

    return;
}

/**
 * @brief GetLayerData creates a new list with data for only one layer
 * @param liste              - input data to search for hits
 * @param layer              - the layer to select
 * @return                   - a nullpointer on error, an empty list on
 *                              invalid layer (or no data for it) or a new list with
 *                              copies of the data for the selected layer
 */
std::list<Dataset>* GetLayerData(std::list<Dataset>* liste, int layer)
{
    if(liste == nullptr)
        return nullptr;

    std::list<Dataset>* newlist = new std::list<Dataset>();

    for(auto& it : *liste)
    {
        if(it.layer == layer)
            newlist->push_back(it);
    }

    return newlist;
}

struct TimestampPlots {
    TimestampPlots() : tothist(nullptr), ts1hist(nullptr), ts2hist(nullptr), ts1corrhist(nullptr),
        ts2corrhist(nullptr) {}
    TH1I* tothist;
    TH1I* ts1hist;
    TH1I* ts2hist;
    TH2I* ts1corrhist;
    TH2I* ts2corrhist;
};

double EvalToT(const Dataset& hit, int tot, double timerescale = 1,
               std::map<Dataset, TF1*>* calibration = nullptr)
{
    if(calibration != nullptr)
    {
        double caltot = tot * timerescale;
        TF1* calfunc = (*calibration)[hit];
        if(calfunc != nullptr)
        {
            caltot = calfunc->Eval(caltot);
            return caltot;
        }
        else
            return -1;
    }
    else
        return tot;
}

int CalculateToT(int ts1, int ts2, int ts1div, int ts2div, int overflow)
{
    int its1 = (ts1 % 1024) * ts1div;
    int its2 = (ts2 % 128) * ts2div;
    its1 = its1 % (128 * ts2div);
    its2 = its2 % (1024 * ts1div);
    int tot = its2 - its1;
    if(tot < 0)
        tot += overflow;

    return tot;
}

/**
 * @brief DecodeToT evaluates the difference of time stamp and tot time stamp
 *          and generates a histogram from it
 * @param liste              - the data to process
 * @param layer              - the layer to select from the data, put 0 to use all data
 * @param tsstepdown         - clock divider for the main clock
 * @param totstepdown        - clock divider for the tot clock
 * @param binscale           - factor to increase the bin width in the histogram
 * @return                   - the generated histogram or a nullpointer on an error
 */
TimestampPlots DecodeToT(std::list<Dataset>* liste, int layer = 0, int tsstepdown = 1,
                         int ts2stepdown = 2, int binscale = 1, double timerescale = 1.,
                         const double timescale = 25,
                         std::map<Dataset, TF1*>* calibration = nullptr)
{
    if(liste == nullptr)
        return TimestampPlots();

    int histwidth;
    double maxtot = (1024 * tsstepdown) - 0.5;
    double end = 1023.5;
    int numbins = 1024;
    if(128 * ts2stepdown < 1024 * tsstepdown)
    {
        histwidth = timescale * ts2stepdown;
        maxtot = (128 * ts2stepdown) - 0.5;
        end = 127.5;
        numbins = 128;
    }
    else
        histwidth = timescale * tsstepdown;


    TimestampPlots result;

    static int histcnt = 0;
    std::stringstream sname("");
    sname << "tothist_" << ++histcnt;
    result.tothist = new TH1I(sname.str().c_str(), "", numbins / binscale, -0.5 * histwidth,
                              end * histwidth);

    result.ts1corrhist = new TH2I((sname.str() + "_corr_ts1").c_str(),
                                  "ToT to TS1 correlation", 1024, -0.5, 1023.5,
                                  numbins / binscale, -0.5, maxtot);
    result.ts2corrhist = new TH2I((sname.str() + "_corr_ts2").c_str(),
                                  "ToT to TS2 correlation", 128, -0.5, 127.5,
                                  numbins / binscale, -0.5, maxtot);

    result.ts1hist = new TH1I((sname.str() + "_TS1").c_str(), "TS1 histogram", 1024, -0.5, 1023.5);
    result.ts2hist = new TH1I((sname.str() + "_TS2").c_str(), "TS2 histogram", 128, -0.5, 127.5);

    int range = 128 * ts2stepdown;
    if(range > 1024 * tsstepdown)
        range = 1024 * tsstepdown;

    for(auto& it : *liste)
    {
        if(it.layer == layer || layer == 0)
        {
            int tot = CalculateToT(it.shortts % 1024, it.shortts2 % 128, tsstepdown, ts2stepdown, range);

            result.tothist->Fill(EvalToT(it, tot, timerescale, calibration) * timescale);

            result.ts1corrhist->Fill(it.ts % 1024, tot);
            result.ts2corrhist->Fill(it.ts2 % 128, tot);

            result.ts1hist->Fill(it.ts % 1024);
            result.ts2hist->Fill(it.ts2 % 128);
        }
    }

    return result;
}

enum axes{
    X = 1,
    Y = 2
};
struct CorrRes{
    TH2* spacecorrelationX;
    TH2* spacecorrelationY;
    TH1* timedistance;

    TH1* failhistX;
    TH1* failhistT;

    TGraph* iteratorone;
    TGraph* iteratortwo;
    TGraph* startingpoint;
};

/**
 * @brief Correlate calculates the correlation of the data provided for two layers
 * @param layerone           - data for the first layer (for X axis in plots)
 * @param layertwo           - data for the second layer (for Y axis in plots)
 * @param spacedist          - maximum spatial distance to be considered (in um)
 * @param timedist           - maximum time distance to be considered (in s)
 * @return                   - a container with pointers to 3 histograms:
 *                                  spatial correlation in X and Y
 *                                  time difference histogram for the considered hits
 */
CorrRes Correlate(std::list<Dataset>* layerone, std::list<Dataset>* layertwo,
                            double spacedist, double timedist, bool debug = false)
{
    if(layerone == nullptr || layertwo == nullptr)
        return CorrRes{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    if(layerone->size() == 0 || layertwo->size() == 0)
        return CorrRes{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

    long long tdist = timedist / 25e-9 + 1;
    long long xdist = spacedist / 50e-6 + 1;

    static int histcnt = 0;
    std::stringstream six("");
    six << "corrhistX_" << ++histcnt;
    std::stringstream siy("");
    siy << "corrhistY_" << histcnt;
    TH2* corhistX = new TH2I(six.str().c_str(), "", 132, -0.5, 131.5, 132, -0.5, 131.5);
    TH2* corhistY = new TH2I(siy.str().c_str(), "", 372, -0.5, 371.5, 372, -0.5, 372.5);

    static int tsdiffcnt = 0;
    std::stringstream sits("");
    sits << "corrhist_ts_" << ++tsdiffcnt;
    TH1* tshist = new TH1I(sits.str().c_str(), "", 10000, -5000.5 * 25, 4999.5 * 25);

    std::stringstream sfailx("");
    sfailx << "corrfailhistSpace_" << histcnt;
    TH1* fhistX = new TH1I(sfailx.str().c_str(), "", 100000, -0.5 * 50, 699.5 * 50);
    std::stringstream sfailt("");
    sfailt << "corrfailhistTime_" << histcnt;
    TH1* fhistT = new TH1I(sfailt.str().c_str(), "", 100000, -0.5 * 25, 49999.5 * 25);

    //debug graphs:
    TGraph* gronepos = new TGraph(0);
    TGraph* grtwopos = new TGraph(0);
    TGraph* grposlast = new TGraph(0);


    std::list<Dataset>::iterator lastmatch = layertwo->begin();
    std::list<Dataset>::iterator newmatch = layertwo->end();
    int loopcounter = 0;
    int outofrangecounter = 0;  //as a limit to enable unsorted data

    //indices for debugging:
    int itonepos = 0;
    int ittwopos = 0;
    int lastmatchpos = 0;
    int newmatchpos = 0;

    for(auto& itone : *layerone)
    {
        int debug_counter = 0;
        if(debug)
        {
            ittwopos = lastmatchpos; //debug
            gronepos->SetPoint(gronepos->GetN(), gronepos->GetN()-1, itonepos); //debug
            grtwopos->SetPoint(grtwopos->GetN(), grtwopos->GetN()-1, ittwopos); //debug
            grposlast->SetPoint(grposlast->GetN(), grposlast->GetN()-1, lastmatchpos); //debug
        }
        outofrangecounter = 0;
        for(auto ittwo = lastmatch; ittwo != layertwo->end(); ++ittwo)
        {
            ++debug_counter;
            if(std::abs((itone.ts /*% 1024*/) - (ittwo->ts /*% 1024*/)) < tdist)
            {
                long long thisdist = sqrt(pow(itone.column - ittwo->column, 2) * 9
                                           + pow(itone.row - ittwo->row, 2));
                //the pixels are 3 times as wide as high, so calculate in multiples of 50um:
                if(thisdist < xdist)
                {
                    corhistX->Fill(itone.column, ittwo->column);
                    corhistY->Fill(itone.row, ittwo->row);

                    tshist->Fill(((itone.ts /*% 1024*/) - (ittwo->ts /*% 1024*/)) * 25);

                    newmatch = ittwo;
                    newmatchpos = ittwopos; //debug
                }
                else
                    fhistX->Fill(thisdist * 50);

                outofrangecounter = 0;
            }
            else if((itone.ts /*% 1024*/) - (ittwo->ts /*% 1024*/) < -3*tdist)
            {
                //std::cout << "forward: " << debug_counter << std::endl;
                fhistT->Fill(std::abs(itone.ts - ittwo->ts));
                if(++outofrangecounter >= 20)
                    break;
            }
            else
            {
                fhistT->Fill(std::abs(itone.ts - ittwo->ts));
                outofrangecounter = 0;
            }

            if(debug)
                ++ittwopos; //debug
        }
        if(debug)
        {
            gronepos->SetPoint(gronepos->GetN(), gronepos->GetN(), itonepos); //debug
            grtwopos->SetPoint(grtwopos->GetN(), grtwopos->GetN(), ittwopos); //debug
            grposlast->SetPoint(grposlast->GetN(), grposlast->GetN(), lastmatchpos); //debug
        }

        bool done = lastmatch != layertwo->begin();
        debug_counter = 0;
        auto start = lastmatch;
        --start;

        if(debug)
        {
            ittwopos = lastmatchpos - 1;
            if(ittwopos < 0)
                ittwopos = 0;
            gronepos->SetPoint(gronepos->GetN(), gronepos->GetN()-1, itonepos); //debug
            grtwopos->SetPoint(grtwopos->GetN(), grtwopos->GetN()-1, ittwopos); //debug
            grposlast->SetPoint(grposlast->GetN(), grposlast->GetN()-1, lastmatchpos); //debug
        }

        outofrangecounter = 0;
        for(auto ittwo = start; done == true; --ittwo)
        {
            ++debug_counter;
            if(ittwo == layertwo->begin())
                done = false;

            if(std::abs((itone.ts /*% 1024*/) - (ittwo->ts /*% 1024*/)) < tdist)
            {
                long long thisdist = sqrt(pow(itone.column - ittwo->column, 2) * 9
                                          + pow(itone.row - ittwo->row, 2));
                if(thisdist < xdist)
                {
                    corhistX->Fill(itone.column, ittwo->column);
                    corhistY->Fill(itone.row, ittwo->row);

                    tshist->Fill(((itone.ts /*% 1024*/) - (ittwo->ts /*% 1024*/)) * 25);

                    newmatch = ittwo;
                    newmatchpos = ittwopos; //debug
                }
                else
                    fhistX->Fill(thisdist * 50);

                outofrangecounter = 0;
            }
            else if((itone.ts /*% 1024*/) - (ittwo->ts /*% 1024*/) > 3*tdist)
            {
                //std::cout << "backwards: " << debug_counter << std::endl;
                fhistT->Fill(std::abs(itone.ts - ittwo->ts));
                if(++outofrangecounter >= 20)
                    break;
            }
            else
            {
                fhistT->Fill(std::abs(itone.ts - ittwo->ts));
                outofrangecounter = 0;
            }

            if(debug)
                --ittwopos; //debug
        }
        if(debug)
        {
            gronepos->SetPoint(gronepos->GetN(), gronepos->GetN(), itonepos); //debug
            grtwopos->SetPoint(grtwopos->GetN(), grtwopos->GetN(), ittwopos); //debug
            grposlast->SetPoint(grposlast->GetN(), grposlast->GetN(), lastmatchpos); //debug
        }


        if(newmatch != layertwo->end())
        {
            lastmatch = newmatch;
            lastmatchpos = newmatchpos; //debug
            newmatch = layertwo->end();
            newmatchpos = layertwo->size()-1; //debug
        }

        //TODO: add this again and remove debug_counter!
        std::cout << ++loopcounter << "/" << layerone->size() << " done\r";
        std::cout.flush();

        ++itonepos; //debug
    }
    std::cout << std::endl;

    //debug graph drawing:
//    TCanvas* c = new TCanvas();
//    c->SetWindowSize(1000,700);
//    c->Update();

//    gronepos->SetLineColor(2);
//    gronepos->SetLineWidth(2);
//    grtwopos->SetLineColor(3);
//    grtwopos->SetLineWidth(2);
//    grposlast->SetLineColor(4);

//    gronepos->GetYaxis()->SetRange(0, ((layerone->size() < layertwo->size())?
//                                           layertwo->size():layerone->size()));

//    gronepos->Draw("AL");
//    grtwopos->Draw("Lsame");
//    grposlast->Draw("Lsame");

//    TLegend* leg = new TLegend(0.7,0.2,0.9,0.4);
//    leg->AddEntry(gronepos, "iterator 1", "l");
//    leg->AddEntry(grtwopos, "iterator 2", "l");
//    leg->AddEntry(grposlast, "startpoint", "l");
//    leg->Draw("same");
    //end debug graph drawing

    CorrRes result;
    result.spacecorrelationX = corhistX;
    result.spacecorrelationY = corhistY;
    result.timedistance      = tshist;
    result.failhistT         = fhistT;
    result.failhistX         = fhistX;
    if(debug)
    {
        result.iteratorone   = gronepos;
        result.iteratortwo   = grtwopos;
        result.startingpoint = grposlast;
    }
    else
    {
        delete gronepos;
        delete grtwopos;
        delete grposlast;
    }
    return result;
}

struct Extent{
	int startcol;
	int endcol;
	int startrow;
	int endrow;
	long long time;
	
	void Fill(const Dataset& dat, int tolerance){
		startcol = dat.column - tolerance / 3;
		endcol   = dat.column + tolerance / 3;
		startrow = dat.row    - tolerance;
		endrow   = dat.row    + tolerance;
		time     = dat.ts;
	}
	
	void Extend(const Dataset& dat, int tolerance){
		if(startcol > dat.column - tolerance / 3)
			startcol = dat.column - tolerance / 3;
		else if(endcol < dat.column + tolerance / 3)
			endcol   = dat.column + tolerance / 3;
		if(startrow > dat.row - tolerance)
			startrow = dat.row - tolerance;
		else if(endrow < dat.row + tolerance)
			endrow   = dat.row + tolerance;
	}
	
	bool Inside(int col, int row, long long ts, const long long deltat = 3){
		return (std::abs(time - ts) < deltat && startcol <= col && col <= endcol && startrow <= row && row <= endrow);
	}
	
	bool Inside(Dataset dat, const long long deltat = 3){
		return Inside(dat.column, dat.row, dat.ts, deltat);
	}
	
	bool LongAgo(Dataset dat, long long longago = 200){
		return ((dat.ts - time) > longago);
	}
};

std::list<std::pair<Extent, std::list<Dataset> > >* Clusterise(std::list<Dataset>* liste, 
					double spacedist = 150e-6, double timedist = 75e-9)
{
	if(liste == nullptr || liste->size() == 0)
		return nullptr;

	//container to remove data from:
	std::list<Dataset> rawhits;
	rawhits.insert(rawhits.end(), liste->begin(), liste->end());
	
	auto doneclusters = new std::list<std::pair<Extent, std::list<Dataset> > >();
	auto clusters = new std::list<std::pair<Extent, std::list<Dataset> > >();
	
	const int deltat = int(ceil(timedist / 25e-9));
	const int deltax = int(ceil(spacedist / 50e-6));
	
	Extent newcluster;
	newcluster.Fill(rawhits.front(), deltax);
	
	std::list<Dataset> neueliste;
	neueliste.push_back(rawhits.front());
	clusters->push_back(std::make_pair(newcluster, neueliste));
	rawhits.pop_front();
	
	auto it = rawhits.begin();
	bool addedone = false;
	
	std::cout << "Clusters\tremaining hits\n" << clusters->size() << "\t\t" << rawhits.size() << "          \r";
	
	while(rawhits.size() > 0)
	{
		bool aborted = false;
		for(auto& cit : *clusters)
		{
			//coarse check for being closeby:
			if(cit.first.Inside(*it, deltat))
			{
				bool found = false;
				//precise check with the hits inside the cluster:
				for(auto& hit : cit.second)
				{
					//take care of 150x50µm² pixels of ATLASPix3:
					if(pow(deltax, 2) <= pow(hit.column - it->column, 2) * 3 + pow(hit.row - it->row, 2))
					{
						cit.second.push_back(*it);		//add to cluster
						cit.first.Extend(*it, deltax);  //update "closeby" area
						it = rawhits.erase(it);			//remove from unclustered list
						addedone = true;				//store that a hit was added in this cycle
						found = true;					//to restart the search with the next hit

						std::cout << clusters->size() << "\t\t" << rawhits.size() << "\r";
						std::cout.flush();

						break;					
					}
				}
				
				if(found)	//do not check other clusters if the hit was already added to one
					break;
			}
			//abort the search if the clusters are too early for the hits
			else if(cit.first.LongAgo(*it, 100 * deltat))
			{
				aborted = true;
				break;
			}
		}
		
		if(rawhits.size() == 0)
			break;
			
		++it;
		if(it == rawhits.end() || aborted)
		{
			//create a new cluster if no hit could be added to an existing cluster:
			if(addedone == false)
			{
				doneclusters->insert(doneclusters->end(), clusters->begin(), clusters->end());
				clusters->clear();
				
				Extent newcluster;
				newcluster.Fill(rawhits.front(), deltax);
				std::list<Dataset> neueliste;
				neueliste.push_back(rawhits.front());
				clusters->push_back(std::make_pair(newcluster, neueliste));
				
				rawhits.pop_front();
			}
			else
				addedone = false;
			
			it = rawhits.begin();
			
			std::cout << clusters->size() + doneclusters->size() << "\t\t" << rawhits.size() << "          \r";
			std::cout.flush();
		}
	}
	
	std::cout << std::endl;
	
	doneclusters->insert(doneclusters->end(), clusters->begin(), clusters->end());
	delete clusters;
	clusters = nullptr;
	
	return doneclusters;
}

struct ClusteringResult{
	TH1* sizehist;
	TH1* delayhist;
};

ClusteringResult AnalyseClusters(std::list<std::pair<Extent, std::list<Dataset> > >* clusters)
{
	static int sizecnt = 0;
	std::stringstream ssize("");
	ssize << "hist_clustersize_" << ++sizecnt;
	TH1* hist = new TH1I(ssize.str().c_str(),"",40, 0.5, 40.5);

	std::stringstream stime("");
	stime << "hist_timedistr_" << sizecnt;
	TH1* histtime = new TH1I(stime.str().c_str(), "", 40, -20.5, 19.5);
	
	for(auto& it : *clusters)
	{
		hist->Fill(it.second.size());
		for(auto& pit : it.second)
			histtime->Fill(it.first.time - pit.ts);
	}
		
	ClusteringResult result;
	result.sizehist  = hist;
	result.delayhist = histtime;
		
	return result;
}

/**
 * @brief Analysis analysis method to call all other methods to perform everything in one go
 * @param filename           - input data to analyse
 * @param outputprefix       - prefix for storing plots, nothing will be saved on an empty string
 * @param performcorrelation - execute the layer to layer correlations on true, skip them on false
 * @param generatedebuggraphs - generate more plots for debugging during correlation of layers
 */
void Analysis(std::string filename, std::string outputprefix = "", bool performcorrelation = true,
              const bool generatedebuggraphs = false)
{
    std::list<Dataset>* fullset = LoadFile(filename);

    if(fullset == nullptr || fullset->size() == 0)
    {
        std::cout << "no data loaded. Aborting" << std::endl;
        return;
    }

    std::list<Dataset>* layerdata[4];

    for(int i = 0; i < 4; ++i)
        layerdata[i] = GetLayerData(fullset, i+1);
        
    int missingpackages = 0;
    int lastid = fullset->front().packageid;
    for(auto& it : *fullset)
    {
    	if(it.packageid != lastid)
    	{
    		missingpackages += it.packageid - lastid - 1;
    		lastid = it.packageid;
    	}
    }
    std::cout << "Missing packages: " << missingpackages << "/" << (fullset->back().packageid - fullset->front().packageid + 1)
    		  << std::endl;


    for(int i = 0; i < 4; ++i)
    {
        RemoveInvalidHits(layerdata[i], true);

        if(layerdata[i]->size() == 0)
        {
            std::cout << "no data. -> Skipping layer " << (i+1) << std::endl;
            continue;
        }

        //Beam Spot:
        TH2*     hist = DrawHitMap(layerdata[i],0,1,1,std::string("Layer ") + char(i+49));
        TCanvas* c    = DrawTH2(hist, nullptr, "Column (in 150 um)", "Row (in 50 um)", "Counts", "colz");
        if(outputprefix != "" && c != nullptr)
            c->SaveAs((outputprefix + "_hitmap_L" + char(i+49) + ".pdf").c_str());
        //Beam Spot with grouped pixels (300 x 300 µm²):
        TH2*     hist2x6 = DrawHitMap(layerdata[i],0,2,6,std::string("Layer ") + char(i+49));
        c = DrawTH2(hist2x6, nullptr, "Column (in 150 um)", "Row (in 50 um)", "Counts", "colz");
        if(outputprefix != "" && c != nullptr)
            c->SaveAs((outputprefix + "_hitmap2x6_L" + char(i+49) + ".pdf").c_str());

        //ToT:
        TimestampPlots tsresult = DecodeToT(layerdata[i], 0, 1,2 , 1);
        tsresult.tothist->SetTitle((std::string("ToT Layer ") + char(i + 49)).c_str());
        c = DrawTH1(tsresult.tothist, nullptr, "ToT (in ns)", "Counts");
        if(outputprefix != "" && c != nullptr)
            c->SaveAs((outputprefix + "_ToT_L" + char(i+49) + ".pdf").c_str());

        tsresult.ts1hist->SetTitle((std::string("TS1 occurrences Layer ") + char(i+49)).c_str());
        c = DrawTH1(tsresult.ts1hist, nullptr, "TS1", "Counts");
        if(outputprefix != "" && c != nullptr)
            c->SaveAs((outputprefix + "_TS1_L" + char(i+49) + ".pdf").c_str());

        tsresult.ts2hist->SetTitle((std::string("TS2 occurrences Layer ") + char(i+49)).c_str());
        c = DrawTH1(tsresult.ts2hist, nullptr, "TS2", "Counts");
        if(outputprefix != "" && c != nullptr)
            c->SaveAs((outputprefix + "_TS2_L" + char(i+49) + ".pdf").c_str());

        tsresult.ts1corrhist->SetTitle((std::string("TS1 to ToT Correlation Layer ")
                                        + char(i+49)).c_str());
        c = DrawTH2(tsresult.ts1corrhist, nullptr, "TS1", "ToT", "Counts");
        if(outputprefix != "" && c != nullptr)
            c->SaveAs((outputprefix + "_TS1_to_ToT_corr_L" + char(i+49) + ".pdf").c_str());

        tsresult.ts2corrhist->SetTitle((std::string("TS2 to ToT Correlation Layer ")
                                        + char(i+49)).c_str());
        c = DrawTH2(tsresult.ts2corrhist, nullptr, "TS2", "ToT", "Counts");
        if(outputprefix != "" && c != nullptr)
            c->SaveAs((outputprefix + "_TS2_to_ToT_corr_L" + char(i+49) + ".pdf").c_str());

        //Timestamp:
        TGraph* gr = new TGraph(0);
        for(auto& it : *layerdata[i])
            gr->SetPoint(gr->GetN(),gr->GetN(), it.ts);
        if(gr->GetN() > 0)
        {
            gr->SetTitle((std::string("Time Stamps in the data for layer ") + char(i+49)).c_str());
            c = DrawTGraph(gr, nullptr, "Entry", "TS (in 25ns)");
            if(outputprefix != "" && c != nullptr)
                c->SaveAs((outputprefix + "_Timestamps_L" + char(i+49) + ".pdf").c_str());
        }

        //Package IDs:
        //TGraph* grpid = new TGraph(0);
        //for(auto& it : *layerdata[i])
        //    grpid->SetPoint(grpid->GetN(), grpid->GetN(), it.packageid);
        //if(grpid->GetN() > 0)
        //{
        //    grpid->SetTitle((std::string("PackageIDs of the data layer ") + char(i+49)).c_str());
        //    c = DrawTGraph(grpid, nullptr, "Entry", "PackageID");
        //    if(outputprefix != "" && c != nullptr)
        //        c->SaveAs((outputprefix + "_Packages_L" + char(i+49) + ".pdf").c_str());
        //}

        //correlations:
        for(int j = i + 1; j < 4 && performcorrelation; ++j)
        {
            std::string ending = std::string("L") + char(i+49) + "_L" + char(j+49);
            CorrRes result = Correlate(layerdata[i], layerdata[j], 150e-6*15, 1875e-9, generatedebuggraphs); //1250e-9);

            if(result.spacecorrelationX != nullptr)
                result.spacecorrelationX->SetTitle(("X correlation " + ending).c_str());
            if(result.spacecorrelationY != nullptr)
                result.spacecorrelationY->SetTitle(("Y correlation " + ending).c_str());
            if(result.timedistance != nullptr)
                result.timedistance->SetTitle(("Time difference between correlated points "
                                                + ending).c_str());
            if(result.failhistT != nullptr)
            {
                result.failhistT->SetTitle(("Time Distance for rejection between "
                                            + ending).c_str());
                //include underflow and overflow bins:
                result.failhistT->GetXaxis()->SetRange(0, result.failhistT->GetNbinsX() + 2);
            }
            if(result.failhistX != nullptr)
            {
                result.failhistX->SetTitle(("Space Distance for rejection between "
                                            + ending).c_str());
                //include underflow and overflow bins:
                result.failhistX->GetXaxis()->SetRange(0, result.failhistX->GetNbinsX() + 2);
            }
            if(generatedebuggraphs && result.iteratorone != nullptr)
            {
                result.iteratorone->SetTitle((std::string("Algorithm Visualisation ")
                                                + ending).c_str());
                result.iteratorone->GetYaxis()->SetRange(0,
                                        ((layerdata[i]->size() < layerdata[j]->size())?
                                                layerdata[j]->size():layerdata[i]->size()));
            }
            TCanvas* c1 = DrawTH2(result.spacecorrelationX, nullptr,
                                  std::string("Column on layer ") + char(i + 49),
                                  std::string("Column on layer ") + char(j + 49),
                                  "Counts","colz");
            TCanvas* c2 = DrawTH2(result.spacecorrelationY, nullptr,
                                  std::string("Row on layer ") + char(i + 49),
                                  std::string("Row on layer ") + char(j + 49),
                                  "Counts","colz");
            TCanvas* c3 = DrawTH1(result.timedistance, nullptr, "Time Difference (in ns)",
                                  "Counts");
            TCanvas* c4 = DrawTH1(result.failhistT, nullptr, "Time Difference (ns)", "Counts");
            TCanvas* c5 = DrawTH1(result.failhistX, nullptr, "Space Distance (in um)", "Counts");
            TCanvas* c6 = nullptr;
            if(generatedebuggraphs)
            {
                c6 = DrawTGraph(result.iteratorone, nullptr, "Step", "Iterator Position", "AL");
                DrawTGraph(result.iteratortwo, c6,"","","Lsame");
                DrawTGraph(result.startingpoint, c6, "","","Lsame");
                result.iteratorone->SetLineColor(2);
                result.iteratorone->SetLineWidth(2);
                result.iteratortwo->SetLineColor(3);
                result.iteratortwo->SetLineWidth(2);
                result.startingpoint->SetLineColor(4);
                TLegend* leg = new TLegend(0.7,0.2,0.9,0.4);
                leg->AddEntry(result.iteratorone, (std::string("entry layer ")
                                                    + char(i+49)).c_str(), "l");
                leg->AddEntry(result.iteratortwo, (std::string("entry layer ")
                                                    + char(j+49)).c_str(), "l");
                leg->AddEntry(result.startingpoint, "startpoint", "l");
                leg->Draw("same");
            }
            if(outputprefix != "")
            {
                if(c1 != nullptr)
                    c1->SaveAs((outputprefix + "_CorrelationX_" + ending + ".pdf").c_str());
                if(c2 != nullptr)
                    c2->SaveAs((outputprefix + "_CorrelationY_" + ending + ".pdf").c_str());
                if(c3 != nullptr)
                    c3->SaveAs((outputprefix + "_Correlation_Time_" + ending + ".pdf").c_str());
                if(c4 != nullptr)
                    c4->SaveAs((outputprefix + "_Correlation_failT_" + ending + ".pdf").c_str());
                if(c5 != nullptr)
                    c5->SaveAs((outputprefix + "_Correlation_failX_" + ending + ".pdf").c_str());
                if(c6 != nullptr)
                    c6->SaveAs((outputprefix + "_Correlation_algo_" + ending + ".pdf").c_str());
            }
        }
        
        
        //Clustering:
        auto clusters = Clusterise(layerdata[i], 300e-6, 300e-9);
        ClusteringResult resultcluster = AnalyseClusters(clusters);
        if(resultcluster.sizehist != nullptr)
        {
        	resultcluster.sizehist->SetTitle((std::string("Cluster Size Distribution Layer ") + char(49+i)).c_str());
        	c = DrawTH1(resultcluster.sizehist, nullptr, "Clustersize (# pixels)", "Counts");
        	c->SetLogy(1);
        	if(outputprefix != "")
        		c->SaveAs((outputprefix + "_clustersize_L" + char(49+i) + ".pdf").c_str());
        }
        if(resultcluster.delayhist != nullptr)
        {
        	resultcluster.delayhist->SetTitle((std::string("Time Difference in a cluster for Layer ") + char(49+i)).c_str());
        	c = DrawTH1(resultcluster.delayhist, nullptr, "Time Difference (in 25ns)", "Counts");
        	c->SetLogy(1);
        	if(outputprefix != "")
        		c->SaveAs((outputprefix + "_clustertimedifference_L" + char(49+i) + ".pdf").c_str());
        }
    }

    //to be extended...

    return;
}

double InvToT(double* value, double* pars)
{
    double result = pars[0] / pars[2] * utl::LambertW(0, pars[2] / pars[0] * pars[1]
            * exp((value[0] - pars[3] - pars[2] * pars[1])/pars[0])) + pars[1];
    return result;
}

bool sortvector(const std::pair<Dataset, TF1*> lhs, const std::pair<Dataset, TF1*> rhs)
{
    return lhs.first.column < rhs.first.column
            || (lhs.first.column == rhs.first.column && lhs.first.row < rhs.first.row);
}



std::map<Dataset, TF1*> LoadToTCalibration(std::string filename)
{
    if(filename == "")
        return std::map<Dataset, TF1*>();

    std::fstream f;
    f.open(filename.c_str(), std::ios::in);

    if(!f.is_open())
        return std::map<Dataset, TF1*>();

    std::vector<std::pair<Dataset, TF1*> > data;
    data.reserve(49110);

    std::string text;
    Dataset pix;
    TF1* fitfunc = nullptr;

    f >> text;

    while(!f.eof())
    {
        if(text[0] == '#')
        {
            f >> text;
            //header line (not used):
            if(text.find("Fit") != std::string::npos)
                std::getline(f, text);
            else if(text.find("Pixel") != std::string::npos)
            {
                if(fitfunc != nullptr)
                    data.push_back(std::make_pair(pix, fitfunc));

                if((data.size() % 100) == 0)
                    std::cout << "loaded fits for " << data.size() << " pixels\r" << std::flush;

                std::getline(f, text);
                int start = text.find('(');
                int ende  = text.find(')');
                text = text.substr(start, ende - start + 1);
                pix = Dataset(text);

                static int cnt = 0;
                std::stringstream s("");
                s << "totcal_" << ++cnt;
                //fitfunc = new TF1(s.str().c_str(), "[0]*log((x-[1])/[1]) + [2] * x + [3]", 0, 2);
                //inverse ToT function
                fitfunc = new TF1(s.str().c_str(), InvToT, 0, 128, 4);
            }
        }
        else
        {
            double value, valerror;
            std::string plusminus;
            f >> value >> plusminus >> valerror;
            if(text.find("x0") != std::string::npos)
            {
                fitfunc->SetParameter(1, value);
                fitfunc->SetParError(1, valerror);
                fitfunc->SetRange(value, 2);
            }
            else if(text.find("offset") != std::string::npos)
            {
                fitfunc->SetParameter(3, value);
                fitfunc->SetParError(3, valerror);
            }
            else if(text.find("lnscale") != std::string::npos)
            {
                fitfunc->SetParameter(0, value);
                fitfunc->SetParError(0, valerror);
            }
            else if(text.find("linear") != std::string::npos)
            {
                fitfunc->SetParameter(2, value);
                fitfunc->SetParError(2, valerror);
            }
        }
        f >> text;
    }
    std::cout << std::endl;

    f.close();

    if(fitfunc != nullptr)
        data.push_back(std::make_pair(pix, fitfunc));

    std::sort(data.begin(), data.end(), sortvector);

    std::map<Dataset, TF1*> datamap;

    datamap.insert(data.begin(), data.end());

    return datamap;
}
 
 

bool WriteToFile(std::string filename, std::string data)
{
    std::fstream f;
    f.open(filename.c_str(), std::ios::out | std::ios::app);

    if(!f.is_open())
        return false;

    f << data;
    f.flush();
    f.close();

    return true;
}

void EqualisedToT(std::string filename, std::string totcal, std::string outputprefix = "",
                  bool dataoutput = false)
{
    std::list<Dataset>* fullset = LoadFile(filename);

    if(fullset == nullptr || fullset->size() == 0)
    {
        std::cout << "no data loaded. Aborting" << std::endl;
        return;
    }

    TFile rootfileoutput;
    if(outputprefix != "")
        rootfileoutput.Open((outputprefix + "_plots.root").c_str(), "UPDATE");
    //TFile rootfileoutput((outputprefix + "_plots.root").c_str(), "UPDATE");

    std::list<Dataset>* layerdata[4];

    for(int i = 0; i < 4; ++i)
        layerdata[i] = GetLayerData(fullset, i+1);

    int missingpackages = 0;
    int lastid = fullset->front().packageid;
    for(auto& it : *fullset)
    {
        if(it.packageid != lastid)
        {
                missingpackages += it.packageid - lastid - 1;
                lastid = it.packageid;
        }
    }
    std::cout << "Missing packages: " << missingpackages << "/" << (fullset->back().packageid - fullset->front().packageid + 1)
                  << std::endl;

    //Load ToTCalibration:
    std::map<Dataset, TF1*> totcalibration = LoadToTCalibration(totcal);

    TCanvas* c = nullptr;

    const int i = 0;

    //ToT:
    TimestampPlots result = DecodeToT(layerdata[i], 0, 1, 8, 1);
    TH1I* histtot = result.tothist;
    histtot->SetTitle((std::string("ToT Layer ") + char(i + 49)).c_str());
    c = DrawTH1(histtot, nullptr, "ToT (in ns)", "Counts");
    if(outputprefix != "" && c != nullptr)
    {
        c->SaveAs((outputprefix + "_bareToT_L" + char(i+49) + ".pdf").c_str());
        c->Write();
        histtot->Write();

        if(dataoutput)
            WriteToFile(outputprefix + "_bareToT_L" + char(i+49) + ".data",
                        ConvertTH1ToData(histtot));
    }

    //calibrated ToT:
    TimestampPlots calresult = DecodeToT(layerdata[i], 0, 1, 8, 1, 0.25/*3.2/2*/, 5463, &totcalibration);
    TH1* histtotcal = calresult.tothist;
    histtotcal->SetTitle((std::string("calibrated ToT Layer ") + char(i + 49)).c_str());
    TF1* langau = fitLandauGaussToHistogram(histtotcal, "", false, 3200, 10000);
    TF1* langauext = new TF1(*langau);
    langauext->SetRange(histtotcal->GetBinLowEdge(1),
                        histtotcal->GetBinLowEdge(histtotcal->GetNbinsX())
                            + histtotcal->GetBinWidth(1));
    langauext->SetLineStyle(2);

    c = DrawTH1(histtotcal, nullptr, "ToT (in e-)", "Counts");
    langau->Draw("Lsame");
    langauext->Draw("Lsame");
    if(outputprefix != "" && c != nullptr)
    {
        c->SaveAs((outputprefix + "_bareToT_L" + char(i+49) + "_cal.pdf").c_str());
        histtotcal->Write();
        langau->Write();
        langauext->Write();
        c->Write();

        if(dataoutput)
        {
            std::stringstream s("");

            s << ConvertTH1ToData(histtotcal) << "\n"
              << ConvertTF1ToData(langau, 1000, true) << "\n"
              << ConvertTF1ToData(langauext, 4000, true);

            WriteToFile(outputprefix + "_berToT_L" + char(i+49) + "_cal.data", s.str());
        }
    }


    std::map<Dataset, int> pixelcount;

    for(auto& it : (*layerdata[i]))
    {
        auto rit = pixelcount.find(it);
        if(rit != pixelcount.end())
            ++(rit->second);
        else
            pixelcount.insert(std::make_pair(it, 1));
    }

//    std::list<Dataset> onepixel;
//    Dataset pix;
//    pix.column  = 0;
//    pix.row     = 0;
//    int maxhits = 0;
//    for(auto& it : pixelcount)
//    {
//        if(it.second > maxhits)
//        {
//            pix     = it.first;
//            maxhits = it.second;
//        }
//    }

//    for(auto& it : (*layerdata[i]))
//    {
//        if(it == pix)
//            onepixel.push_back(it);
//    }


//    //single pixel ToT:
//    TH1* histtot_op = DecodeToT(&onepixel, 0, 1, 8, 8);
//    histtot_op->SetTitle((std::string("ToT Layer ") + char(i + 49) + " Pixel "
//                          + pix.ToString()).c_str());
//    c = DrawTH1(histtot_op, nullptr, "ToT (in ns)", "Counts");
//    if(outputprefix != "" && c != nullptr)
//        c->SaveAs((outputprefix + "_bareToT_L" + char(i+49) + "_onepix.pdf").c_str());

//    //single pixel calibrated ToT:
//    TH1* histtotcal_op = DecodeToT(&onepixel, 0, 1, 8, 8, 0.25/*3.2/2*/, 5463, &totcalibration);
//    histtotcal_op->SetTitle((std::string("calibrated ToT Layer ") + char(i + 49) + " Pixel "
//                          + pix.ToString()).c_str());
//    TF1* langau_op = fitLandauGaussToHistogram(histtotcal_op);
//    c = DrawTH1(histtotcal_op, nullptr, "ToT (in e-)", "Counts");
//    langau_op->Draw("Lsame");
//    if(outputprefix != "" && c != nullptr)
//        c->SaveAs((outputprefix + "_bareToT_L" + char(i+49) + "_onepix_cal.pdf").c_str());


    //repeat with clusterised events:
    std::list<std::pair<Extent, std::list<Dataset> > >* clusters = Clusterise(layerdata[i],
                                                                             150e-6, 50e-9);

    std::cout << "Clusters / Hits: " << clusters->size() << " / " << (layerdata[i])->size()
              << std::endl;

    const int timescale = 25;
    const int binscale = 8;
    static int histcnt = 0;
    std::stringstream sname("");
    sname << "tot_cluster_hist_" << ++histcnt;
    //create hist, assuming 25 ns time stamps:
    TH1* hist_cluster = new TH1I(sname.str().c_str(), "", 1024 / binscale, -0.5 * timescale,
                         1023.5 * timescale);
    sname.str("");
    sname << "tot_cluster_hist_cal_" << histcnt;
    TH1* hist_clustercal = new TH1I(sname.str().c_str(), "", 2048 / binscale, -0.5 * timescale,
                         99999.5);
    for(auto& it : *clusters)
    {
        double charge    = 0;
        double chargecal = 0;
        for(auto& hit : it.second)
        {
            int tot = CalculateToT(hit.ts % 1024, hit.ts2 % 128, 1, 8, 1024);
            charge += tot; //EvalToT(hit, tot, 0.25, nullptr);
            chargecal += EvalToT(hit, tot, 0.25, &totcalibration);
        }

        hist_cluster->Fill(charge * timescale);
        hist_clustercal->Fill(chargecal * 5463.); //timescale);
    }
    TF1* clustercalfit = fitLandauGaussToHistogram(hist_clustercal, "", false, 3200, 10000);
    TF1* clustercalfitext = new TF1(*clustercalfit);
    clustercalfitext->SetRange(hist_clustercal->GetBinLowEdge(1),
                                     hist_clustercal->GetBinLowEdge(hist_clustercal->GetNbinsX())
                                        + hist_clustercal->GetBinWidth(1));
    clustercalfitext->SetLineStyle(2);

    hist_cluster->SetTitle("Cluster ToT");
    c = DrawTH1(hist_cluster, nullptr, "ToT (in ns)", "Counts");
    if(outputprefix != "" && c != nullptr)
    {
        c->SaveAs((outputprefix + "_clusterToT_L" + char(i+49) + ".pdf").c_str());
        hist_cluster->Write();
        c->Write();

        if(dataoutput)
            WriteToFile(outputprefix + "_clusterToT_L" + char(i+49) + ".data",
                        ConvertTH1ToData(hist_cluster));
    }

    hist_clustercal->SetTitle("Calibrated Cluster ToT");
    c = DrawTH1(hist_clustercal, nullptr, "Charge (in e-)", "Counts");
    clustercalfit->Draw("Lsame");
    clustercalfitext->Draw("Lsame");
    if(outputprefix != "" && c != nullptr)
    {
        c->SaveAs((outputprefix + "_clusterToT_L" + char(i+49) + "_cal.pdf").c_str());
        hist_clustercal->Write();
        clustercalfit->Write();
        clustercalfitext->Write();
        c->Write();

        if(dataoutput)
        {
            std::stringstream s("");
            s << ConvertTH1ToData(hist_clustercal) << "\n"
              << ConvertTF1ToData(clustercalfit, 1000, true) << "\n"
              << ConvertTF1ToData(clustercalfitext, 4000, true);

            WriteToFile(outputprefix + "_clusterToT_L" + char(i+49) + "_cal.data", s.str());
        }
    }

    //write fit results to file:
    if(outputprefix != "")
    {
        std::fstream  f;

        f.open((outputprefix + "_fitparameters.dat").c_str(), std::ios::out | std::ios::app);

        if(f.is_open())
        {
            std::stringstream s("");
            s << "# Calibrated ToTs all pixels:\n"
              << "#  Parameter; Value; Error\n";
            for(int i = 0; i < langau->GetNpar(); ++i)
                s << "  " << langau->GetParName(i) << "\t" << langau->GetParameter(i)
                  << "\t" << langau->GetParError(i) << "\n";

//            s << "\n# Calibrated ToTs pixel " << pix.ToString() << ":\n"
//              << "#  Parameter; Value; Error\n";
//            for(int i = 0; i < langau_op->GetNpar(); ++i)
//                s << "  " << langau_op->GetParName(i) << "\t" << langau_op->GetParameter(i)
//                  << "\t" << langau_op->GetParError(i) << "\n";

            s << "\n# Clustered and Calibrated ToTs all pixels:\n"
              << "#  Parameter; Value; Error\n";
            for(int i = 0; i < clustercalfit->GetNpar(); ++i)
                s << "  " << clustercalfit->GetParName(i) << "\t" << clustercalfit->GetParameter(i)
                  << "\t" << clustercalfit->GetParError(i) << "\n";

            f << s.str() << std::flush;

            f.close();
        }
        else
            std::cerr << "Could not write fit parameters to \"" << outputprefix
                      << "_fitparameters.dat\"" << std::endl;
    }

    //clear memory:
    for(auto& it : totcalibration)
    {
        delete it.second;
        it.second = nullptr;
    }
    totcalibration.clear();

    //if(outputprefix != "")
    //    rootfileoutput.Close();
}



#endif
