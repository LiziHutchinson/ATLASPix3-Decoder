/********************************************************
 * Former "plotmatrix.cpp" script now adapted for       *
 *   updated data fomat and for timing optimisation     *
 *   aka "Timing Trimming"                              *
 * Now changed to be compatible for usage without ROOT  *
 * Removed everything not used for generating TDAC      *
 *   pattern files                                      *
 *                                                      *
 * Version  1.0 (02.10.19)                              *
 *          1.1 (11.10.19)                              *
 *          2.0 (31.01.20)                              *
 *          2.1 (04.02.20)                              *
 *              - timing trimming is now "complete"     *
 *          2.2 (06.03.20)                              *
 *              - added methods to create sparsely      *
 *                  activated matrix patterns           *
 *          2.3 (06.03.20)                              *
 *              - stripped all unnecessary code for     *
 *                  generating TDAC pattern files       *
 *              - made script compilable without ROOT   *
 ********************************************************/
 
#define useROOT

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef useROOT
#include "TCanvas.h"
#include "TAxis.h"
#include "TH1I.h"
#include "TH2D.h"
#include "TF1.h"
#include "TList.h"
#include "TColor.h"
#include "TStyle.h"
#include "TPaletteAxis.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TLegend.h"
#endif

/**
 * @brief The Matrix class is a container for the TDAC settings in the matrix.
 *          it also stores the corresponding delay values for the setting to ease
 *          optimisation.
 */
class Matrix{
public:
    Matrix(bool deactivate = true){
        std::vector<int> idummy;
        std::vector<double> ddummy;
        for(int i = 0; i < 372; ++i)
        {
            idummy.push_back((deactivate)?8:7);
            ddummy.push_back(1e10);
        }

        tdac.clear();
        delay.clear();
        for(int i = 0; i < 132; ++i)
        {
            tdac.push_back(idummy);
            delay.push_back(ddummy);
        }
    }
    ~Matrix(){
        for(int i = 0; i < 132; ++i)
        {
            tdac[i].clear();
            delay[i].clear();
        }

        tdac.clear();
        delay.clear();
    }

    void Init(bool deactivate = true){
        std::vector<int> idummy;
        std::vector<double> ddummy;
        for(int i = 0; i < 372; ++i)
        {
            idummy.push_back((deactivate)?8:7);
            ddummy.push_back(1e10);
        }

        tdac.clear();
        delay.clear();
        for(int i = 0; i < 132; ++i)
        {
            tdac.push_back(idummy);
            delay.push_back(ddummy);
        }
    }

    Matrix(const Matrix& ref){
        Init(true);
        for(int i = 0; i < 132; ++i)
        {
            for(int j = 0; j < 372; ++j)
            {
                tdac[i][j]  = ref.tdac[i][j];
                delay[i][j] = ref.delay[i][j];
            }
        }
    }

    bool SaveXML(std::string filename);

    std::vector<std::vector<int> >    tdac;
    std::vector<std::vector<double> > delay;
};

/**
 *  SaveXML() statically converts the TDAC settings from the entries of `pixels` into
 *		the XML format used by the Qt readout software. Pixels without entry in `pixels`
 *		are deactivated (TDAC = 8)
 *
 * @param filename		- file name to write the XML to
 * @return                      - true if the file was written, false if not
 */
bool Matrix::SaveXML(std::string filename)
{
    std::fstream f;
    f.open(filename.c_str(), std::ios::out);

    if(!f.is_open())
        return false;

    std::stringstream sdata("");
    sdata << "<?xml version=\"1.0\"?>\n"
          << "<TDACs name=\"TDAC_values\" rows=\"372\" columns=\"132\" maxvalue=\"15\">\n";

    for(int row = 0; row < 372; ++row)
    {
        sdata << "\t<Row addr=\"" << row << "\">\n";

        for(int col = 0; col < 132; ++col)
        {
            sdata << "\t\t<pix addr=\"" << col << "\" val=\"" << tdac[col][row] << "\"/>\n";
        }
        sdata << "\t</Row>\n";
    }
    sdata << "</TDACs>\n";

    f << sdata.str();
    f.flush();

    f.close();

    return true;
}

///this method assumes the file contents to be just "col row tdac threshold"
Matrix LoadThrData(std::string filename)
{
    std::fstream f;
    f.open(filename.c_str(), std::ios::in);
    if(!f.is_open())
        return Matrix(true);

    Matrix tdacs(true);
    int col, row, tdac;
    double thr;
    while(!f.eof())
    {
        if(f >> col >> row >> tdac >> thr)
        {
            if(col >= 0 && col < 132 && row >= 0 && row < 372)
            {
                tdacs.tdac[col][row]  = tdac;
                tdacs.delay[col][row] = thr;
            }
            else
                std::cout << "This should not happen: col=" << col << " and row=" << row << std::endl;
        }
        else
            break;
    }

    f.close();
    return tdacs;
}

#ifdef useROOT
/**
 * @brief FormatTH2Axes applies a usable formatting to the axes for a 1000x700 px² window
 * @param hist          - the histogram to format the axes
 * @param xaxis         - title for the x axis
 * @param yaxis         - title for the y axis
 * @param zaxis         - title for the colour code (aka z axis)
 */
void FormatTH2Axes(TH2* hist, std::string xaxis = "Column", std::string yaxis = "Row", std::string zaxis = "Delay (s)")
{
        if(hist == nullptr)
                return;

        hist->GetXaxis()->SetTitle(xaxis.c_str());
        hist->GetXaxis()->SetTitleSize(0.05);
        hist->GetXaxis()->SetLabelSize(0.05);
        hist->GetYaxis()->SetTitle(yaxis.c_str());
        hist->GetYaxis()->SetTitleSize(0.05);
        hist->GetYaxis()->SetLabelSize(0.05);
        hist->GetYaxis()->SetTitleOffset(1.1);
        hist->GetZaxis()->SetTitle(zaxis.c_str());
        hist->GetZaxis()->SetTitleSize(0.05);
        hist->GetZaxis()->SetLabelSize(0.05);
        hist->GetZaxis()->SetTitleOffset(1.2);
}

/**
 * Draws a 2D histogram with a 255 step colour code. Axis formatting is not done.
 *
 * @param hist		- the histogram to plot
 * @return			- the canvas on which the histogram was drawn
 */
TCanvas* DrawTH2(TH2* hist)
{
//    ROOT5 rainbow palette with 255 steps:
//    static bool setup = true; //false;
//    if(!setup)
//    {
//        setup = true;
//        const Int_t NRGBs = 5;
//        const Int_t NCont = 255;

//        Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
//        Double_t red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
//        Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
//        Double_t blue[NRGBs]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
//        TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
//        gStyle->SetNumberContours(NCont);
//    }

//  ROOT6 style 255 step palette:
    hist->SetContour(255);

    TCanvas* c = new TCanvas();
    c->SetWindowSize(1080, 700);
    c->SetLeftMargin(0.12/1.08);
    c->SetRightMargin((0.09+/*c->GetRightMargin()*/0.08)/1.08);
    c->Update();

    hist->SetStats(false);
    hist->Draw("COLZ");

    gPad->Update();
    TPaletteAxis *palette = (TPaletteAxis*)hist->GetListOfFunctions()
                                                            ->FindObject("palette");
    palette->SetX2NDC(palette->GetX1NDC()+0.03);
    if(hist->GetXaxis()->GetBinLowEdge(hist->GetNbinsX()) < 1e-3)
        palette->SetY1NDC(0.17);    //avoid overlap with the power label of the x axis
    palette->Draw();

    return c;
}
#endif


void CreateSparseMatrix(int columnstep, int rowstep, int tdacvalue, std::string outputfilename, bool print = false)
{
    Matrix tdacs = Matrix(true);
    int numactivepixels = 0;

    for(int row = 0; row < 372; row += rowstep)
    {
        for(int col = ((row/rowstep) % columnstep); col < 132; col += columnstep)
        {
            tdacs.tdac[col][row] = tdacvalue;
            ++numactivepixels;
        }
    }

    std::cout << "Active Fraction: " << numactivepixels << "/49104 -> " << numactivepixels/49104. << std::endl;

    if(outputfilename != "")
    {
        if(!tdacs.SaveXML(outputfilename))
            std::cout << "Writing configuration to \"" << outputfilename << "\" failed." << std::endl;
        else
            std::cout << "Configuration written to \"" << outputfilename << "\"" << std::endl;
    }

    if(print)
    {
#ifdef useROOT
        static int histcnt = 0;
        std::stringstream s("");
        s << "sparsehist_" << ++histcnt;
        TH2I* hist = new TH2I(s.str().c_str(), "TDAC Distribution", 132, -0.5, 131.5, 372, -0.5, 371.5);

        for(int col = 0; col < 132; ++col)
            for(int row = 0; row < 372; ++row)
                hist->Fill(col, row, tdacs.tdac[col][row]);

        hist->Sumw2(false);
        hist->GetZaxis()->SetRangeUser(-0.5,8.5);

        FormatTH2Axes(hist, "Column", "Row", "TDAC Setting");
        DrawTH2(hist);
#else
        std::cout << "Drawing requires ROOT/CLING" << std::endl;
#endif
    }
}

///origin in the middle of the chip for centerx/centery
void CreateMatrixWithHole(double centerx, double centery, double radius, int tdac, std::string outputfilename, bool print = false)
{
    Matrix tdacs = Matrix(true);
    int numactivepixels = 0;

    centerx += 19.8/2.;
    centery += 18.6/2;

    for(int col = 0; col < 132; ++col)
    {
        for(int row = 0; row < 372; ++row)
        {
            if(pow(centerx - col * 0.150, 2) + pow(centery - row * 0.050, 2) > pow(radius, 2))
            {
                tdacs.tdac[col][row] = tdac;
                ++numactivepixels;
            }
        }
    }

    std::cout << "Active Fraction: " << numactivepixels << "/49104 -> " << numactivepixels/49104. << std::endl;

    if(outputfilename != "")
    {
        if(!tdacs.SaveXML(outputfilename))
            std::cout << "Writing configuration to \"" << outputfilename << "\" failed." << std::endl;
        else
            std::cout << "Configuration written to \"" << outputfilename << "\"" << std::endl;
    }

    if(print)
    {
#ifdef useROOT
        static int histcnt = 0;
        std::stringstream s("");
        s << "holehist_" << ++histcnt;
        TH2I* hist = new TH2I(s.str().c_str(), "TDAC Distribution", 132, -0.5, 131.5, 372, -0.5, 371.5);

        for(int col = 0; col < 132; ++col)
            for(int row = 0; row < 372; ++row)
                hist->Fill(col, row, tdacs.tdac[col][row]);

        hist->Sumw2(false);
        hist->GetZaxis()->SetRangeUser(-0.5,8.5);

        FormatTH2Axes(hist, "Column", "Row", "TDAC Setting");
        DrawTH2(hist);
#else
        std::cout << "Drawing requires ROOT/CLING" << std::endl;
#endif
    }
}

int main(int argc, char* argv[])
{
    if(argc == 5)
    {
        std::stringstream s("");
        s << argv[1] << " " << argv[2] << " " << argv[3];
        int col, row, tdac;
        s >> col >> row >> tdac;
        CreateSparseMatrix(col,row, tdac, argv[4], false);
    }
    else if(argc == 6)
    {
        std::stringstream s("");
        s << argv[1] << " " << argv[2] << " " << argv[3] << " " << argv[4];
        int tdac;
        double centerx, centery, radius;
        s >> centerx >> centery >> radius >> tdac;
        CreateMatrixWithHole(centerx, centery, radius, tdac, argv[5], false);
    }
    else
        std::cout << "Usage: \n"
                  << "  for sparse matrix:\n    "
                  << argv[0] << " `col step` `row step` `active TDAC value` `outputfilename`\n"
                  << "  for matrix with hole:\n    "
                  << argv[0] << " `center x (mm)` `center y (mm)` `Radius (mm)` `active TDAC value` `outputfilename`\n"
                  << "     where the origin is the middle of the matrix"
                  << std::endl;

    return 0;
}
