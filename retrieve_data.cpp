#include <fstream>
#include <string>
#include <sstream>

#include "TH1.h"
#include "TGraph.h"
#include "TGraphErrors.h"


std::string ConvertTH1ToData(const TH1* hist)
{
    if(hist == nullptr)
        return "";

    std::stringstream s("");

    s << "# TH1 Histogram Data for \"" << hist->GetTitle() << "\"\n"
      << "#    X axis: \"" << hist->GetXaxis()->GetTitle() << "\"\n"
      << "#    Y axis: \"" << hist->GetYaxis()->GetTitle() << "\"\n"
      << "#    Bin Width: " << hist->GetBinWidth(0) << "\n"
      << "#    Data: X bin center; Y value\n";

    for(int i = 0; i <= hist->GetNbinsX() + 1; ++i)
        s << hist->GetBinCenter(i) << "\t" << hist->GetBinContent(i) << "\n";

    return s.str();
}

std::string ConvertTH2ToData(const TH2* hist)
{
    if(hist == nullptr)
        return "";

    std::stringstream s("");

    s << "# TH2 Histogram Data for \"" << hist->GetTitle() << "\"\n"
      << "#    X axis: \"" << hist->GetXaxis()->GetTitle() << "\"\n"
      << "#    Y axis: \"" << hist->GetYaxis()->GetTitle() << "\"\n"
      << "#    Z axis: \"" << hist->GetZaxis()->GetTitle() << "\"\n"
      << "#    X Bin Width: " << hist->GetXaxis()->GetBinWidth(1) << "\n"
      << "#    Y Bin Width: " << hist->GetYaxis()->GetBinWidth(1) << "\n"
      << "#    Data: X bin center; Y bin center; Z value\n";

    for(int x = 0; x <= hist->GetNbinsX() + 1; ++x)
    {
        for(int y = 0; y <= hist->GetNbinsY() + 1; ++y)
            s << hist->GetXaxis()->GetBinCenter(x) << "\t" << hist->GetYaxis()->GetBinCenter(y)
              << "\t" << hist->GetBinContent(x,y) << "\n";
    }

    return s.str();
}

std::string ConvertTGraphToData(const TGraph* gr)
{
    if(gr == nullptr)
        return "";

    std::stringstream s("");

    s << "# TGraph Data for \"" << gr->GetTitle() << "\"\n"
      << "#    X axis: \"" << gr->GetXaxis()->GetTitle() << "\"\n"
      << "#    Y axis: \"" << gr->GetYaxis()->GetTitle() << "\"\n"
      << "#    Data: X value; Y value\n";

    for(int i = 0; i < gr->GetN(); ++i)
        s << gr->GetX()[i] << "\t" << gr->GetY()[i] << "\n";

    return s.str();
}


std::string ConvertTGraphErrorsToData(const TGraphErrors* gr)
{
    if(gr == nullptr)
        return "";

    std::stringstream s("");

    s << "# TGraphErrors Data for \"" << gr->GetTitle() << "\"\n"
      << "#    X axis: \"" << gr->GetXaxis()->GetTitle() << "\"\n"
      << "#    Y axis: \"" << gr->GetYaxis()->GetTitle() << "\"\n"
      << "#    Data: X value; Y value; X error; Y error\n";

    for(int i = 0; i < gr->GetN(); ++i)
        s << gr->GetX()[i] << "\t" << gr->GetY()[i] << "\t" << gr->GetErrorX(i) << "\t"
          << gr->GetErrorY(i) << "\n";

    return s.str();
}

std::string ConvertTF1ToData(const TF1* func, int npx = 1000, bool skipformula = false)
{
    if(func == nullptr)
        return "";

    std::stringstream s("");

    double rangestart, rangeend;
    func->GetRange(rangestart, rangeend);

    s << "# TF1 Data for \"" << func->GetTitle() << "\"\n"
      << "#    X axis: \"" << func->GetXaxis()->GetTitle() << "\"\n"
      << "#    Y axis: \"" << func->GetYaxis()->GetTitle() << "\"\n"
      << "#    Data Range: " << rangestart << " - " << rangeend << "\n";

    if(!skipformula)
        s << "#    Function: \"" << std::string(func->GetFormula()->GetExpFormula()) << "\"\n";
    s << "#    Parameters:\n";
    for(int i = 0; i < func->GetNpar(); ++i)
        s << "#       [" << i << "] \"" << func->GetParName(i) << "\":\t" << func->GetParameter(i)
          << "\t+-\t" << func->GetParError(i) << "\n";

    s << "#    Data: X value; Y value\n";

    double factor = (rangeend - rangestart) / npx;

    for(int i = 0; i < npx; ++i)
    {
        double value = rangestart + factor * i;
        s << value << "\t" << func->Eval(value) << "\n";
    }

    return s.str();
}
