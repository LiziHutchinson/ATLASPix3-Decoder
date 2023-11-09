#include "object_drawing.h"

#define objectdrawingsources

TCanvas* DrawTF1(TF1* func, TCanvas* c, std::string title,
                 std::string xtitle, std::string ytitle,
                 std::string drawoptions)
{
    if(func == nullptr)
        return nullptr;

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1000,700);
        c->SetLeftMargin(0.12);
        c->SetRightMargin(0.08);
        c->Update();
    }
    else
    {
        //make sure to switch to the correct canvas:
        c->cd();
        c->Update();
        if(drawoptions.find("same") == std::string::npos)
            std::cout << "Warning: drawing to existing canvas without \"same\" option"
                      << std::endl;
    }

    func->SetTitle(title.c_str());
    func->GetXaxis()->SetTitle(xtitle.c_str());
    func->GetXaxis()->SetTitleSize(0.05);
    func->GetXaxis()->SetLabelSize(0.05);
    func->GetYaxis()->SetTitle(ytitle.c_str());
    func->GetYaxis()->SetTitleSize(0.05);
    func->GetYaxis()->SetLabelSize(0.05);
    func->GetYaxis()->SetTitleOffset(1.3);

    func->Draw(drawoptions.c_str());

    return c;
}

TCanvas* DrawTGraph(TGraph* gr, TCanvas* c, std::string title,
                    std::string xtitle, std::string ytitle,
                    std::string drawoptions)
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
    else
    {
        //make sure to switch to the correct canvas:
        c->cd();
        c->Update();
        if(drawoptions.find("same") == std::string::npos)
            std::cout << "Warning: drawing to existing canvas without \"same\" option"
                      << std::endl;
    }

    gr->SetTitle(title.c_str());
    gr->GetXaxis()->SetTitle(xtitle.c_str());
    gr->GetXaxis()->SetTitleSize(0.05);
    gr->GetXaxis()->SetLabelSize(0.05);
    gr->GetYaxis()->SetTitle(ytitle.c_str());
    gr->GetYaxis()->SetTitleSize(0.05);
    gr->GetYaxis()->SetLabelSize(0.05);
    gr->GetYaxis()->SetTitleOffset(1.3);
    gr->SetMarkerStyle(8);
    gr->SetMarkerColor(2);
    
    if(drawoptions.find("L") != std::string::npos || drawoptions.find("l") != std::string::npos)
        gr->SetLineColor(2);

    gr->Draw(drawoptions.c_str());

    return c;
}

TCanvas* DrawTGraphErrors(TGraphErrors* gr, TCanvas* c, std::string title,
                          std::string xtitle, std::string ytitle, std::string drawoptions)
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
    else
    {
        //make sure to switch to the correct canvas:
        c->cd();
        c->Update();
        if(drawoptions.find("same") == std::string::npos)
            std::cout << "Warning: drawing to existing canvas without \"same\" option"
                      << std::endl;
    }

    gr->SetTitle(title.c_str());
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

TCanvas* DrawTH1(TH1* hist, TCanvas* c, std::string title,
                 std::string xtitle, std::string ytitle, std::string drawoptions, bool stats)
{
    if(hist == nullptr)
        return nullptr;

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1000, 700);
        c->SetLeftMargin(0.12);
        c->SetRightMargin(0.08);
        c->Update();
    }
    else
    {
        //make sure to switch to the correct canvas:
        c->cd();
        c->Update();
        if(drawoptions.find("same") == std::string::npos)
            std::cout << "Warning: drawing to existing canvas without \"same\" option"
                      << std::endl;
    }

    hist->SetTitle(title.c_str());
    hist->GetXaxis()->SetTitle(xtitle.c_str());
    hist->GetXaxis()->SetTitleSize(0.05);
    hist->GetXaxis()->SetLabelSize(0.05);
    hist->GetYaxis()->SetTitle(ytitle.c_str());
    hist->GetYaxis()->SetTitleSize(0.05);
    hist->GetYaxis()->SetLabelSize(0.05);
    hist->GetYaxis()->SetTitleOffset(1.3);

    hist->SetStats(stats);

    hist->Draw(drawoptions.c_str());

    return c;
}

TCanvas* DrawTH2(TH2* hist, TCanvas* c, std::string title,
                 std::string xtitle, std::string ytitle, std::string ztitle,
                 std::string drawoptions, bool stats)
{
    if(hist == nullptr)
        return nullptr;

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1080, 700); //slightly wider canvas to accomodate the colour scale
        c->SetLeftMargin(0.12 / 1.08);
        c->SetRightMargin(0.08 / 1.08 + 0.08);
        c->Update();
    }
    else
    {
        //make sure to switch to the correct canvas:
        c->cd();
        c->Update();
        if(drawoptions.find("same") == std::string::npos)
            std::cout << "Warning: drawing to existing canvas without \"same\" option"
                      << std::endl;
    }

    hist->SetContour(255);  //make colour scale more smooth with 255 steps instead of 16

    hist->SetTitle(title.c_str());
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
    hist->GetZaxis()->SetTitleOffset(1);

    hist->SetStats(stats);

    hist->Draw(drawoptions.c_str());

    //avoid overlap of Z axis colour axis with exponent of X axis:
    if(hist->GetXaxis()->GetXmax() < 1e-3 || hist->GetXaxis()->GetXmax() > 1e5)
    {
        c->Update(); //the palette exists only after actually drawing the thing on the canvas
        TPaletteAxis *palette = (TPaletteAxis*)hist->GetListOfFunctions()->FindObject("palette");
        //the palette axis will only exist if at least one entry is in the histogram:
        if(hist->Integral() > 0 && palette != nullptr)
            palette->SetY1NDC(0.176); //move lower end of palette axis above the exponent of X axis
        c->Update();
    }

    return c;
}

bool MoveStatsBox(TCanvas* c, double x1ndc, double y1ndc,
                  double x2ndc, double y2ndc, std::string name)
{
    if(c == nullptr)
        return false;

    TPaveStats* st = static_cast<TPaveStats*>(c->GetPrimitive("stats"));
    if(st != nullptr)
    {
        if(name == "" || name[name.length()-1] == '_')
        {
            if(name == "")
                name = "stats_";
            static int index = 0;
            std::stringstream s("");
            s << name << ++index;
            name = s.str();
        }

        st->SetName(name.c_str());
        st->SetX1NDC((x1ndc < 0)?0.62:x1ndc);
        st->SetY1NDC((y1ndc < 0)?0.15:y1ndc);
        st->SetX2NDC((x2ndc < 0)?0.82:x2ndc);
        st->SetY2NDC((y2ndc < 0)?0.39:y2ndc);
        st->Draw();
        c->Update();

        return true;
    }
    else
        return false;
}

int GetROOTWarningLevel()
{
    return gErrorIgnoreLevel / 1000;
}

int SetROOTWarningLevel(int level)
{
    if(level > 6 || level < 0)
        return GetROOTWarningLevel();
    else
    {
        gErrorIgnoreLevel = level * 1000;
        return level;
    }
}

void SaveCanvas(TCanvas* c, std::string filename, bool noinfo)
{
    if(c == nullptr || filename == "")
        return;
    
    int rootwarning = 0;
    if(noinfo)
    {
        rootwarning = GetROOTWarningLevel();
        if(rootwarning < WarningLevels::WL_Warning)
            SetROOTWarningLevel(WarningLevels::WL_Warning);
    }
    c->SaveAs(filename.c_str());
    if(noinfo)
        SetROOTWarningLevel(rootwarning);
}

