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
 *                                                      *
 ********************************************************/
#include "TH2I.h"
#include "TF1.h"
#include "TF2.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TPaveStats.h"
#include "TVirtualFitter.h"
#include "TMinuit.h"
#include "TFitResult.h"
#include "TFile.h"
#include "TPaletteAxis.h"

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <list>
#include <deque>
#include <algorithm>
#include <utility>
#include <math.h>

typedef long long longlong;

/**
 * evaluates the global minuit instance for the result of the last fit performed
 *
 * @param verbose        - outputs the error status of gMinuit o an error if set to true
 *
 * @return                - true if the status of gMinuit was "Converged" or "OK", false otherwise
 */
bool TestFitSuccess(bool verbose = false)
{
    std::string minuitstatus = std::string(gMinuit->fCstatu);

    if(minuitstatus.compare("CONVERGED ") != 0 && minuitstatus.compare("OK        ") != 0)
        //the spaces are important!
    {
        if(verbose)
            cout << "  Minimization did not converge! (Minuit status: \"" << minuitstatus << "\")" << endl;
        return false;
    }
    else
        return true;
}

int GrayDecode(int graycode, int numbits)
{
    int normal;
    normal = 0;
    normal = graycode & (1 << (numbits - 1));
    for(int i = numbits - 2; i >= 0; --i)
        normal |= (graycode ^ (normal >> 1)) & (1 << i);

    return normal;
}

class Dataset
{
    public:
    Dataset() : column(-1), row(-1), triggerindex(-1), triggerts(-1), ts(-1), timestamp(-1),
        tot(-1), timestamp2(-1), packageid(-1), layer(-1), valid(false)  {}

    static std::string GetHeader()
    {
        return "# PackageID; Layer; Column; Row; TS; ToT; TriggerTS; TriggerID; ext. TS; ext. TS2";
    }

    std::string ToString()
    {
        std::stringstream s("");
        s << packageid << "\t" << layer << "\t" << column << "\t" << row << "\t" << ts << "\t"
          << tot << "\t" << triggerts << "\t" << triggerindex << "\t" << timestamp << "\t"
          << timestamp2;

        return s.str();
    }

    bool is_valid() {return valid;}
    short     column;
    short     row;
    int       triggerindex;
    long long triggerts;
    int       ts;
    long long timestamp;
    int       tot;
    long long timestamp2;
    int       packageid;
    short     layer;
    bool      valid;
};

/**
 * @brief Construct fills the entries of the Dataset object from the text passed
 *      assuming the data field order and count from `order`
 * @param line              - the data to extract from
 * @param entriesperline    - total number of data fields in the line
 * @param order             - Dataset containing the field indices (starting at 0)
 *                                for the fields to extract (-1 for do not use)
 * @return                  - a reconstructed Dataset or an empty Dataset on an error
 */
Dataset Construct(std::string line, const int entriesperline, Dataset order)
{
    std::stringstream s(line);

    Dataset dat;
    //std::string dummy;

    long long data[entriesperline];

    bool success = true;
    for(int i = 0; i < entriesperline; ++i)
        success &= bool(s >> data[i]);

    if(!success)
        return Dataset();

    if(order.column       != -1) dat.column       = short(data[order.column]);
    if(order.row          != -1) dat.row          = short(data[order.row]);
    if(order.triggerindex != -1) dat.triggerindex = int(data[order.triggerindex]);
    if(order.triggerts    != -1) dat.triggerts    = data[order.triggerts];
    if(order.ts           != -1) dat.ts           = data[order.ts];
    if(order.timestamp    != -1) dat.timestamp    = data[order.timestamp];
    if(order.tot          != -1) dat.tot          = data[order.tot];
    if(order.timestamp2   != -1) dat.timestamp2   = data[order.timestamp2];
    if(order.packageid    != -1) dat.packageid    = int(data[order.packageid]);
    if(order.layer        != -1) dat.layer        = short(data[order.layer]);
    dat.valid = true;


    //if(!(s >> dat.packageid >> dat.layer >> dat.column >> dat.row >> dummy/*TS*/ >> dummy/*ToT*/
    //  >> dat.triggerts >> dat.triggerindex >> dat.ts >> dat.tot >> dummy/*FIFOoverflow*/))
    //    return Dataset();
    //else
        return dat;
}

///converts the passed string to lower case: [A-Z] -> [a-z]
std::string ToLowerCase(std::string text)
{
    std::string lctext = "";

    for(auto& it : text)
    {
        if(it >= 'A' && it <= 'Z')
            lctext = lctext + char(it - ('A' - 'a'));
        else
            lctext = lctext + it;
    }

    return lctext;
}

///removes spaces and tabs from beginning and end of the string
std::string StripSpaces(std::string text)
{
    unsigned int start = 0;
    for(start = 0; start < text.length(); ++start)
        if(text[start] != ' ' && text[start] != '\t')
            break;

    unsigned int end = text.length() - 1;
    for(end = end; end > start; --end)
        if(text[end] != ' ' && text[end] != '\t')
            break;
    ++end;

    return text.substr(start, end - start);
}

class InputFile{
public:
    InputFile() : f(std::fstream()), datacnt(0), fieldorder(Dataset()), numfields(0) {}

    bool Open(std::string filename)
    {
        if(filename != "")
        {
            f.open(filename.c_str(), std::ios::in);
            if(f.is_open())
            {
                datacnt = 0;

                char text[200];
                f.getline(text, 200, '\n');

                return LoadFieldorder(std::string(text));
            }
            else
                return false;
        }
        else
            return false;
    }
    bool is_open(){
        return f.is_open();
    }

    void Close(){
        f.close();
        return;
    }

    std::list<Dataset>* Read(int numhits, bool print = false, bool graydecode = false);

    bool Finished(){
        if(f.is_open())
            return f.eof();
        else
            return true;
    }

private:
    bool LoadFieldorder(std::string headerline);

    std::fstream f;
    unsigned long long datacnt;
    Dataset fieldorder;
    int numfields;
};

bool InputFile::LoadFieldorder(std::string headerline)
{
    if(headerline[0] == '#')
    {
        std::string line = headerline.substr(2);

         numfields = std::count(line.begin(), line.end(), ';');
        if(*line.rbegin() != ';')
        {
            //std::cout << "last character: \"" << *line.rbegin() << "\"" << std::endl;
            ++numfields;
        }

        int start = 0;
        int end = 0;
        int fieldcounter = 0;
        while(end != int(std::string::npos))
        {
            end = int(line.find(';', start));
            std::string field = ToLowerCase(StripSpaces(line.substr(start, end - start)));
            if(field.compare("packageid") == 0)
                fieldorder.packageid    = fieldcounter;
            else if(field.compare("layer") == 0)
                fieldorder.layer        = fieldcounter;
            else if(field.compare("column") == 0)
                fieldorder.column       = fieldcounter;
            else if(field.compare("row") == 0)
                fieldorder.row          = fieldcounter;
            else if(field.compare("triggerts") == 0)
                fieldorder.triggerts    = fieldcounter;
            else if(field.compare("triggerid") == 0)
                fieldorder.triggerindex = fieldcounter;
            else if(field.compare("ext. ts") == 0)
                fieldorder.timestamp    = fieldcounter;
            else if(field.compare("ext. tot") == 0)
                fieldorder.timestamp2   = fieldcounter;
            else if(field.compare("ts") == 0)
                fieldorder.ts           = fieldcounter;
            else if(field.compare("tot") == 0)
                fieldorder.tot          = fieldcounter;
            //else if(field.compare("fifo overflow") == 0)
                //not used
            else
                std::cout << "Warning: unknown field \"" << field << "\" found." << std::endl;

            start  = end + 1;
            ++fieldcounter;
        }

        std::cout << "decoded field list (" << numfields << " entries):\n" << fieldorder.GetHeader()
                  << std::endl << fieldorder.ToString() << std::endl;

        return true;
    }
    else
        return false;
}

std::list<Dataset>* InputFile::Read(int numhits, bool print, bool graydecode)
{
    if(!is_open())
        return nullptr;

    if(numhits == 0)
        return new std::list<Dataset>();

    std::list<Dataset>* hits = new std::list<Dataset>();
    long long counter = 0;

    while(!f.eof() && (counter < numhits || numhits == 0))
    {
        std::string line = "";
        std::getline(f, line, '\n');
        Dataset dat = Construct(line, numfields, fieldorder);
        if(dat.is_valid())
        {
            if(graydecode)
            {
                dat.ts = GrayDecode(dat.ts, 10);
                dat.tot = GrayDecode(~dat.tot, 7);
            }
            hits->push_back(dat);
        }
        ++counter;
        datacnt += line.length();
        if((counter % 1000) == 0 && print)
        {
            if(datacnt < 1e3)
                std::cout << "\rread " << datacnt << " B   ";
            else if(datacnt < 1e6)
                std::cout << "\rread " << datacnt/1.e3 << " kB   ";
            else if(datacnt < 1e9)
                std::cout << "\rread " << datacnt/1.e6 << " MB   ";
            else
                std::cout << "\rread " << datacnt/1.e9 << " GB   ";
            std::cout.flush();
        }
    }
    std::cout << std::endl;

    return hits;
}

bool MoveStatsBox(TCanvas* c, double x1ndc = -1, double y1ndc = -1,
                  double x2ndc = -1, double y2ndc = -1, std::string name = "")
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

TCanvas* DrawTGraph(TGraph* gr, TCanvas* c = nullptr, std::string xtitle = "",
                 std::string ytitle = "", std::string drawoptions = "AP")
{
    if(gr == nullptr)
        return nullptr;

    //select passed canvas for drawing:
    if(c != nullptr)
        c->cd();

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1000,700);
        c->SetLeftMargin(0.12);
        c->SetRightMargin(0.08);
        c->Update();
    }
    //else if(drawoptions.find("same") == std::string::npos)
    //    std::cout << "Warning: drawing to existing canvas without \"same\" option" << std::endl;

    gr->GetXaxis()->SetTitle(xtitle.c_str());
    gr->GetXaxis()->SetTitleSize(0.05);
    gr->GetXaxis()->SetLabelSize(0.05);
    gr->GetYaxis()->SetTitle(ytitle.c_str());
    gr->GetYaxis()->SetTitleSize(0.05);
    gr->GetYaxis()->SetLabelSize(0.05);
    gr->GetYaxis()->SetTitleOffset(1.3);
    gr->SetMarkerStyle(8);
    gr->SetMarkerColor(2);

    c->cd();

    gr->Draw(drawoptions.c_str());
    c->Update();

    return c;
}

TCanvas* DrawTH1(TH1* hist, TCanvas* c = nullptr, std::string xtitle = "",
                 std::string ytitle = "", std::string drawoptions = "")
{
    if(hist == nullptr)
        return nullptr;

    //selected passed canvas for drawing:
    if(c != nullptr)
        c->cd();

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1000,700);
        c->SetLeftMargin(0.12);
        c->SetRightMargin(0.08);
        c->Update();
    }
    //else if(drawoptions.find("same") == std::string::npos)
    //    std::cout << "Warning: drawing to existing canvas without \"same\" option" << std::endl;

    hist->GetXaxis()->SetTitle(xtitle.c_str());
    hist->GetXaxis()->SetTitleSize(0.05);
    hist->GetXaxis()->SetLabelSize(0.05);
    hist->GetYaxis()->SetTitle(ytitle.c_str());
    hist->GetYaxis()->SetTitleSize(0.05);
    hist->GetYaxis()->SetLabelSize(0.05);
    hist->GetYaxis()->SetTitleOffset(1.3);

    hist->Draw(drawoptions.c_str());
    c->Update();

    return c;
}

TCanvas* DrawTH2(TH2* hist, TCanvas* c = nullptr, std::string xtitle = "", std::string ytitle = "",
                 std::string ztitle = "", std::string drawoptions = "colz", bool stats = true)
{
    if(hist == nullptr)
        return nullptr;

    //select passed canvas for drawing:
    if(c != nullptr)
        c->cd();

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1080,700);
        c->SetLeftMargin(0.12/1.08);
        c->SetRightMargin(0.08/1.08+0.08);
        c->Update();
    }
    //else if(drawoptions.find("same") == std::string::npos)
        //std::cout << "Warning: drawing to existing canvas without \"same\" option" << std::endl;

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

    hist->SetStats(stats);

    hist->Draw(drawoptions.c_str());
    c->Update();

    //avoid overlap of the power index for X axis:
    if(hist->GetXaxis()->GetBinCenter(hist->GetNbinsX()) > 100000)
    {
        TPaletteAxis *palette = (TPaletteAxis*)hist->GetListOfFunctions()->FindObject("palette");
        if(palette != nullptr)
        {
            palette->SetY1NDC(0.168);
            c->Update();
        }
        else
            std::cout << "moving palette failed: not found" << std::endl;
    }

    return c;
}

TH2I* DrawHitMap(std::list<Dataset>* liste, int layer = 0, int groupPixX = 1, int groupPixY = 1,
                  std::string title = "", TH2I* hitmap = nullptr)
{
    if(liste == nullptr)
        return nullptr;

    TH2I* hist = hitmap;
    if(hist == nullptr)
    {
        static int indexcnt = 0;
        std::stringstream sname("");
        sname << "spothist_" << ++indexcnt;
        hist = new TH2I(sname.str().c_str(),title.c_str(), 132 / groupPixX, -0.5,131.5,
                                                            372 / groupPixY, -0.5, 371.5);
    }

    for(auto& it : *liste)
    {
        if(layer == 0 || it.layer == layer)
            hist->Fill(it.column, it.row);
    }

    //identify single cluster hitmaps:
    if(hist->GetBinContent(hist->GetMaximumBin()) == 1)
        hist->GetZaxis()->SetRangeUser(0, 10);

    return hist;
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

    auto it = liste->begin();

    std::deque<long long> tss;
    double avgdelay = -1e10;

    bool del = false;
    while(it != liste->end())
    {
        if(hard)
        {
            //form local moving average and remove hits not close (>2.5e9) to it
            if(tss.size() < 100)
                tss.push_back(it->timestamp);
            else
            {
                if(avgdelay > -1e5 && std::abs(it->timestamp - avgdelay) > 2.5e9)
                    del = true;
                else
                {
                    tss.pop_front();
                    tss.push_back(it->timestamp);

                    avgdelay = 0;
                    for(auto& del : tss)
                        avgdelay += del;
                    avgdelay /= 100;
                }
            }
        }
        if(it->column < 0 || it->column > 131 || it->row < 0 || it->row > 371 || del)
        {
            it = liste->erase(it);
            del = false;
        }
        else
            ++it;
    }

    return;
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
TH1I* DecodeToT(std::list<Dataset>* liste, int layer = 0, int tsstepdown = 1, int totstepdown = 8,
               int binscale = 1, TH1I* histtot = nullptr)
{
    if(liste == nullptr)
        return nullptr;

    const int timescale = 160; //25;
                //minimum time in ns per TS bin

    //create hist, assuming 25 ns time stamps:
    TH1I* hist = histtot;
    if(hist == nullptr)
    {
        int histwidth;
        double end = 1023.5;
        int numbins = 1024;
        if(128 * totstepdown < 1024 * tsstepdown)
        {
            histwidth = timescale * totstepdown;
            end = 127.5;
            numbins = 128;
        }
        else
            histwidth = timescale * tsstepdown;

        static int histcnt = 0;
        std::stringstream sname("");
        sname << "tothist_" << ++histcnt;
        hist = new TH1I(sname.str().c_str(), "", numbins / binscale, -0.5 * histwidth,
                         end * histwidth * 2);
                         //end * histwidth);
    }

    int range = 128 * totstepdown;
    if(range > 1024 * tsstepdown)
        range = 1024 * tsstepdown;

    for(auto& it : *liste)
    {
        if(it.layer == layer || layer == 0)
        {
            int ts1 = (it.ts % 1024) * tsstepdown;
            int ts2 = (it.tot % 128) * totstepdown;
            ts1 = ts1 % (128 * totstepdown);
            ts2 = ts2 % (1024 * tsstepdown);
            int tot = ts2 - ts1;
            if(tot < 0)
                tot += range;
            if(tot * timescale < 60000) //debug
                tot += 128 * totstepdown;//debug
            hist->Fill(tot * timescale);
        }
    }

    return hist;
}

enum axes{
    X = 1,
    Y = 2
};

struct Extent{
    int startcol;
    int endcol;
    int startrow;
    int endrow;
    long long time;
    int previoussize;

    void Fill(const Dataset& dat, int tolerance){
        startcol = dat.column - tolerance / 3;
        endcol   = dat.column + tolerance / 3;
        startrow = dat.row    - tolerance;
        endrow   = dat.row    + tolerance;
        time     = dat.timestamp;
        previoussize = 0;
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
        return Inside(dat.column, dat.row, dat.timestamp, deltat);
    }

    bool LongAgo(Dataset dat, long long longago = 200){
        return ((dat.timestamp - time) > longago);
    }
};

struct AlignParams{
    AlignParams() : spaces(""), lastlength(0), lastrange(0) {}
    std::string spaces;
    unsigned int lastlength;
    unsigned long long lastrange;
};

std::string Rightalign(unsigned long long number, unsigned int length, AlignParams* format = nullptr)
{
    static AlignParams* defaultpars = new AlignParams();

    if(format == nullptr)
        format = defaultpars;

    if(number < format->lastrange || number >= 10 * format->lastrange
            || length != format->lastlength)
    {
        //count the number of digits in 10s system:
        format->lastrange = 0;
        unsigned int cnt = 0;
        unsigned long long tmpnumber = number;
        while(tmpnumber > 0)
        {
            ++cnt;
            if(format->lastrange == 0)
                format->lastrange = 1;
            else
                format->lastrange *= 10;
            tmpnumber /= 10;
        }

        format->spaces = "";
        for(unsigned int i = cnt; i <= length; ++i)
            format->spaces = format->spaces + " ";

        format->lastlength = length;
    }

    std::stringstream s("");
    s << format->spaces << number;

    return s.str();
}

std::list<std::pair<Extent, std::list<Dataset> > >* Clusterise(std::list<Dataset>* liste, 
                        double spacedist = 150e-6, double timedist = 75e-9, bool direct = false,
                        std::list<std::pair<Extent, std::list<Dataset> > >* lastclusters = nullptr)
{
    if(liste == nullptr || liste->size() == 0)
            return nullptr;

    //container to remove data from:
    std::list<Dataset>* rawhits = (direct)?liste:new std::list<Dataset>();
    if(direct)
        rawhits = liste;
    else
    {
        rawhits = new std::list<Dataset>();
        rawhits->insert(rawhits->end(), liste->begin(), liste->end());
    }

    bool checkoldclusters = lastclusters != nullptr;

    unsigned int listsize = liste->size();
    unsigned int sizewidth = log10(listsize);
    //parameter for right alignment of number output (progress):
    AlignParams clusterpars;    //for clusters
    AlignParams hitpars;        //for unprocessed hits

    auto doneclusters = new std::list<std::pair<Extent, std::list<Dataset> > >();
    auto clusters = new std::list<std::pair<Extent, std::list<Dataset> > >();

    const int deltat = int(ceil(timedist / 25e-9));
    const int deltax = int(ceil(spacedist / 50e-6));
    double deltaxsquared = pow(deltax, 2);
    double deltat100 = deltat * 100.;
    double deltat200 = deltat * 200.;

    Extent newcluster;
    newcluster.Fill(rawhits->front(), deltax);

    std::list<Dataset> neueliste;
    neueliste.push_back(rawhits->front());
    clusters->push_back(std::make_pair(newcluster, neueliste));
    rawhits->pop_front();

    auto it = rawhits->begin();
    int addedone = 0;

    std::cout << "Clusters  /  remaining hits:\n"
              << Rightalign(clusters->size() + doneclusters->size(),
                             sizewidth, &clusterpars)
              << " / " << Rightalign(rawhits->size(), sizewidth, &hitpars) << "\r";

    while(rawhits->size() > 0)
    {
        bool markasold = false;
        auto oldcluster = clusters->front();
        if(checkoldclusters)
        {
            bool alllongago = true;
            for(auto cit = lastclusters->rbegin(); cit != lastclusters->rend(); ++cit)
            {
                if(cit->first.Inside(*it, deltat))
                {
                    for(auto& hit : cit->second)
                    {
                        if(deltaxsquared <= pow(hit.column - it->column, 2) * 9
                                                + pow(hit.row - it->row, 2))
                        {
                            markasold = true;
                            oldcluster = *cit;
                            break;
                        }
                    }

                    alllongago = false;
                }
                //abort the search if the clusters are too early for the hits
                else if(cit->first.LongAgo(*it, deltat200))
                {
                //    alllongago = false;
                    break;
                }
            }
            if(alllongago)
            {
                checkoldclusters = false;
                std::cout << std::endl << "ended checking old clusters" << std::endl;
            }
        }

        bool aborted = false;
        bool found   = false;
        auto cit = clusters->rbegin();
        while(cit != clusters->rend())
        {
            bool foundone = false;
            found = false;
            //coarse check for being closeby:
            while(cit->first.Inside(*it, deltat))
            {
                found = false;
                //precise check with the hits inside the cluster:
                for(auto& hit : cit->second)
                {
                    //take care of 150x50µm² pixels of ATLASPix3:
                    if(deltaxsquared <= pow(hit.column - it->column, 2) * 9 + pow(hit.row - it->row, 2))
                    {
                        cit->second.push_back(*it);		//add to cluster
                        if(markasold)
                        {
                            if(cit->first.previoussize == 0)
                            {
                                for(auto& ohit : oldcluster.second)
                                {
                                    cit->second.push_back(ohit);
                                    cit->first.Extend(ohit, deltax);
                                }
                                cit->first.previoussize = oldcluster.second.size();
                            }
                        }
                        cit->first.Extend(*it, deltax);  //update "closeby" area
                        it = rawhits->erase(it);			//remove from unclustered list
                        ++addedone;				//store that a hit was added in this cycle
                        found = true;					//to restart the search with the next hit
                        foundone = true;

                        //print every 50 processed hits the numbers:
                        if((rawhits->size() % 50) == 0)
                        {
                            std::cout << Rightalign(clusters->size() + doneclusters->size(),
                                                    sizewidth, &clusterpars)
                                      << " / " << Rightalign(rawhits->size(), sizewidth, &hitpars)
                                      << "\r";
                            std::cout.flush();
                        }

                        break;
                    }
                }

                if(!found)
                    break;
            }
            if(foundone)
            {
                cit = clusters->rbegin();
                continue;
            }
            //abort the search if the clusters are too early for the hits
            //else if(cit->first.LongAgo(*it, deltat100))
            if(cit->first.LongAgo(*it, deltat100))
            {
                aborted = true;
                break;
            }

            ++cit;
        }

        if(rawhits->size() == 0)
            break;

        //retry first hits as some other hits were added to clusters:
        if(addedone > 5 && it != rawhits->begin())
        {
            it = rawhits->begin();
            addedone = 0;
        }
        else
            ++it;

        if(it == rawhits->end() || aborted)
        {
            //create a new cluster if no hit could be added to an existing cluster:
            if(addedone == 0)
            {
                doneclusters->insert(doneclusters->end(), clusters->begin(), clusters->end());
                clusters->clear();

                Extent newcluster;
                newcluster.Fill(rawhits->front(), deltax);
                std::list<Dataset> neueliste;
                neueliste.push_back(rawhits->front());
                if(checkoldclusters)
                {
                    bool alllongago = true;
                    for(auto cit = lastclusters->rbegin();  cit != lastclusters->rend(); ++cit)
                    {
                        if(cit->first.Inside(rawhits->front(), deltat))
                        {
                            for(auto& hit : cit->second)
                            {
                                alllongago = false;
                                if(deltaxsquared <= pow(hit.column - rawhits->front().column, 2) * 9
                                                        + pow(hit.row - rawhits->front().row, 2))
                                {
                                    markasold = true;
                                    oldcluster = *cit;
                                    break;
                                }
                            }
                            alllongago = false;
                        }
                        else if(cit->first.LongAgo(*it, deltat200))
                        {
                            //alllongago = false;
                            break;
                        }
                    }
                    if(alllongago)
                    {
                        checkoldclusters = false;
                        std::cout << std::endl << "ended checking old clusters" << std::endl;
                    }

                    if(markasold)
                    {
                        for(auto& ohit : oldcluster.second)
                        {
                            neueliste.push_back(ohit);
                            newcluster.Extend(ohit, deltat);
                        }
                        newcluster.previoussize = oldcluster.second.size();
                    }
                }

                clusters->push_back(std::make_pair(newcluster, neueliste));

                rawhits->pop_front();
            }
            else
                addedone = 0;

            it = rawhits->begin();

            //print every 50 processed hits the numbers:
            if((rawhits->size() % 50) == 0)
            {
                std::cout << Rightalign(clusters->size() + doneclusters->size(),
                                        sizewidth, &clusterpars)
                          << " / " << Rightalign(rawhits->size(), sizewidth, &hitpars) << "\r";
                std::cout.flush();
            }

            std::cout.flush();
        }
    }

    std::cout << std::endl;

    if(clusters->size() > 0)
        doneclusters->insert(doneclusters->end(), clusters->begin(), clusters->end());
    delete clusters;
    clusters = nullptr;

    if(!direct)
        delete rawhits;

    return doneclusters;
}

///this function is a gaussian revolved around a point (par[0]|par[1]) with
///  radius par[2] with a sigma of 15um
/// x is supposed to be given in indices of column and row
double Circle(double* x, double* par)
{
    //values in um
    //par[0] = meanx
    //par[1] = meany
    //par[2] = radius
    //par[3] = 0 whole ring
    //       > 1 upper half
    //       < -1 lower half
    static const double sigma = 15;
    static const double prefactor = 1500. / (2 * 3.14159265 * pow(sigma, 2));

    double distance = 9 * pow(x[0] - par[0], 2) + pow(x[1] - par[1], 2);
    distance = 50 * sqrt(distance);

    if((par[3] > 1 && x[1] < par[1]) || (par[3] < -1 && x[1] > par[1]))
        return 0;
    else //if(distance > par[2]) // to have a bias towards smaller radius
        return prefactor * exp(-0.5*pow((distance - par[2])/sigma, 2));
    //else
    //    return prefactor * exp(-0.5*pow((distance - par[2])/(0.25*sigma), 2));
}

TCanvas* DrawCircle(TCanvas* c, double meanx, double meany, double radius)
{
    TGraph* gr = new TGraph(0);

    const int steps = 200;
    const double prefactor = 2 * 3.14159265 / steps;

    for(int i = 0; i <= steps; ++i)
        gr->SetPoint(gr->GetN(), meanx - 0.333 * radius * sin(prefactor * i),
                                meany - radius * cos(prefactor * i));

    gr->SetLineColor(kRed);
    gr->SetLineWidth(2);

    if(c == nullptr)
    {
        c = new TCanvas();
        c->SetWindowSize(1000,700);
        c->Update();

        gr->SetTitle("Cluster Dimension");
        gr->GetXaxis()->SetTitle("Column");
        gr->GetXaxis()->SetTitleSize(0.05);
        gr->GetXaxis()->SetLabelSize(0.05);
        gr->GetYaxis()->SetTitle("Row");
        gr->GetYaxis()->SetTitleSize(0.05);
        gr->GetYaxis()->SetLabelSize(0.05);
    }
    else
    {
        c->cd();
        c->Update();
    }

    gr->Draw("Lsame");
    c->Update();

    return c;
}

struct clusterpos{
    clusterpos() : cmsx(-1), cmsy(-1), radius(0), fit(false), sizeestimate(-1) {}
    double cmsx;    //center of the cluster in x direction in pixel indices
    double cmsy;    //center of the cluster in y direction in pixel indices
    double radius;  //radius of the cluster in um
    bool fit;       //true if a fit was used to obtain the values, fals if not
    double sizeestimate;    //estimates number of pixels the cluster would have if
                            //  it was read out completely as area inside the circle fitted
};

/**
 * @brief FindSize evaluates the size of a cluster from the shape of the border
 *     of the cluster
 * @param pixels        - the pixels belonging to the cluster
 * @param corrhist      - pointer to a correlation histogram between cluster size
 *                          from number of pixels read out and size estimate from
 *                          the fit. Set as `nullptr` to disable filling to the histogram
 * @param print         - if set to true it creates a plot of the cluster, its border and the fits
 *                          of it. The ring distribution is shown in magenta with the starting
 *                          parameters and in red as result in the border plot. The ring is also
 *                          drawn on the distribution. In addition, output on fits and results from
 *                          all estimates are written to standard output.
 * @param resetdisplaycounter
 *                      - sets the number of large clusters to draw if print is false.
 *                          the value is stored in a static variable and has to be set only once.
 *                          repeatedly setting the value resets the counting. Keep it -1 to disable
 *                          writing to the counter. The default setting for the counter is 0.
 * @return              - a struct containing the cluster size, position and whether the result
 *                          has been obtained with a fit or not. If the fit was used, also an
 *                          estimate of the pixel count for the whole cluster would have been.
 */
clusterpos FindSize(std::list<Dataset>& pixels, TH2I* corrhist, bool print = false,
                    int resetdisplaycounter = -1)
{
    const int reconstructthreshold = 20;

    static int limit = 0;
    if(resetdisplaycounter != -1)
        limit = resetdisplaycounter;

    if(pixels.size() == 0)
        return clusterpos();

    //find extents of cluster and mean position:
    double meanx = 0;
    double meany = 0;
    double minx = 1e10;
    double maxx = -1e10;
    double miny = 1e10;
    double maxy = -1e10;

    for(auto& it : pixels)
    {
        meanx += it.column;
        meany += it.row;

        if(it.column > maxx)
            maxx = it.column;
        if(it.column < minx)
            minx = it.column;
        if(it.row > maxy)
            maxy = it.row;
        if(it.row < miny)
            miny = it.row;
    }

    meanx /= pixels.size();
    meany /= pixels.size();

    double radius = ((maxx - minx + 1) * 150 + (maxy - miny + 1) * 50) * 0.25;
                                                                    // /2 /2; //radius and mean

    //cluster size assumed to be read out completely:
    if(pixels.size() < reconstructthreshold)
    {
        clusterpos pos;
        pos.cmsx = meanx;
        pos.cmsy = meany;
        pos.radius = radius;
        pos.fit = false;
        pos.sizeestimate = -1;
        return pos;
    }

    //larger clusters:

    static int histcnt = 0;
    std::stringstream sname("");
    sname << "clusterreconstruct_" << ++histcnt;
    TH2I* hist = new TH2I(sname.str().c_str(), "Cluster", 31, -15.5, 15.5, 81, -40.5, 40.5);
    for(auto& it : pixels)
        hist->Fill(it.column - meanx, it.row - meany);

    TCanvas* c = nullptr;
    if(print)
        c = DrawTH2(hist, nullptr, "Column", "Row", "Hit", "colz", true);

    sname.str("");
    sname << "clusterdiff_" << histcnt;
    TH2D* diffhist = new TH2D(sname.str().c_str(), "Diff of cluster hits", 31, -15.5, 15.5, 81, -40.5, 40.5);
    diffhist->Sumw2(false);

    for(int x = 1; x <= diffhist->GetNbinsX(); ++x)
    {
        for(int y = 1; y <= diffhist->GetNbinsY(); ++y)
        {
            double diff = 0;
            for(int i = -1; i <= 1; ++i)
            {
                for(int j = -1; j <= 1; ++j)
                {
                    if(i != 0 || j != 0)
                    {
                        diff += std::abs(hist->GetBinContent(x,y) - hist->GetBinContent(x+i,y+j))
                                / sqrt(pow(i, 2) * 9. + pow(j, 2));
                    }
                }
            }
            diffhist->SetBinContent(x, y, diff);
        }
    }

    if(print)
        DrawTH2(diffhist, nullptr, "Column", "Row", "Diff", "colz", false);

    sname.str("");
    sname << "fitfunc_" << histcnt;
    TF2* fitfunc = new TF2(sname.str().c_str(), Circle, -15.5,15.5,-40.5,40.5, 4);
    fitfunc->SetParameter(0, 0);
    fitfunc->SetParameter(2, ((maxx - minx) * 150)/2.); //no "+1" to not overestimate small clusters
    if(meany < 186)
    {
        fitfunc->SetParameter(1, maxy - meany - (maxx - minx) * 1.5); // /2 * 3);
        fitfunc->FixParameter(3, 2); //upper half of ring
    }
    else
    {
        fitfunc->SetParameter(1, (miny - meany) + (maxx - minx) * 1.5);
        fitfunc->FixParameter(3, -2); //lower half of ring
    }
    fitfunc->SetNpx(200);
    fitfunc->SetNpx(200);
    if(print)
    {
        fitfunc->SetLineColor(kMagenta);
        fitfunc->DrawClone("same");
        fitfunc->SetLineColor(kRed);
    }

    if(print)
    {
        diffhist->Fit(fitfunc, "0");
        fitfunc->Draw("same");
    }
    else
        diffhist->Fit(fitfunc, "Q0");

//    //draw resulting TF2 on extra plot for crosschecking:
//    TCanvas* c2 = new TCanvas();
//    c2->Update();
//    fitfunc->SetNpx(400);
//    fitfunc->SetNpy(400);
//    fitfunc->Draw("colz");

    double pixelestimate = 3.14159265 * pow(fitfunc->GetParameter(2), 2) / (150. * 50);

    if(print)
        std::cout <<   "total pixels:                 " << pixels.size()
                  << "\n Radius from addresses:       " << radius
                  << "\n Radius only Column:          " << (maxx - minx + 1)*75.
                  << "\n Radius from Fit:             " << fitfunc->GetParameter(2)
                  << "\n pixel estimate full cluster: " << pixelestimate
                  << std::endl;

    //draw the clusters with more than 40% missing pixels
    if(!print && pixels.size() < 0.6 * pixelestimate && pixels.size() > 3 * reconstructthreshold)
    {
        if(limit > 0)
        {
            c = DrawTH2(hist, nullptr, "Column", "Row", "Hit", "colz", true);
            DrawCircle(c, fitfunc->GetParameter(0), fitfunc->GetParameter(1),
                       fitfunc->GetParameter(2) * 0.02);
            --limit;
            std::cout << "Large missing fraction found: " << pixels.size() / pixelestimate
                      << "\n in histogram \"" << hist->GetName() << "\" with " << pixels.size()
                      << " pixels instead of " << pixelestimate << " pixels" << std::endl
                      << "   resulting radius: " << fitfunc->GetParameter(2) << " um" << std::endl;
        }
    }

    if(corrhist != nullptr)
        corrhist->Fill(radius, fitfunc->GetParameter(2));

    if(print)
        DrawCircle(c, fitfunc->GetParameter(0), fitfunc->GetParameter(1),
                   fitfunc->GetParameter(2) * 0.02); // / 50.);

    clusterpos pos;
    pos.cmsx         = fitfunc->GetParameter(0) + meanx; //as Pixel index
    pos.cmsy         = fitfunc->GetParameter(1) + meany; //as pixel index
    pos.radius       = fitfunc->GetParameter(2);         //in um
    pos.fit          = true;
    pos.sizeestimate = pixelestimate;                    //as number of pixels

    if(!print)
    {
        delete fitfunc;
        fitfunc = nullptr;
        delete diffhist;
        diffhist = nullptr;
        delete hist;
        hist = nullptr;
    }

    return pos;
}

struct ClusteringResult{
    ClusteringResult() : pixelcnthist(nullptr), sizehist(nullptr), sizecorrelation(nullptr),
        delayhist(nullptr), averageclustershape(nullptr),
        maxcluster(std::make_pair(clusterpos(), nullptr)), maxclustershape(nullptr),
        maxclusterpars(clusterpos()), maxsize(0) {}
    TH1I* pixelcnthist;
    TH1I* sizehist;
    TH2I* sizecorrelation;
    TH1I* delayhist;
    TH2I* averageclustershape;
    std::pair<clusterpos, std::list<Dataset>*> maxcluster;
    TH2I* maxclustershape;
    clusterpos maxclusterpars;
    unsigned int maxsize;
    std::vector<std::pair<clusterpos, std::list<Dataset>*> > exampleclusters;
};

ClusteringResult AnalyseClusters(std::list<std::pair<Extent, std::list<Dataset> > >* clusters,
                                 ClusteringResult* lastresult = nullptr, bool sqrtpixels = true,
                                 bool print = false)
{
    ClusteringResult result;

    if(lastresult == nullptr)
        result = ClusteringResult();
    else
        result = *lastresult;

    if(result.sizehist == nullptr)
    {
        static int sizecnt = 0;
        std::stringstream ssize("");
        ssize << "hist_clustersize_" << ++sizecnt;
        result.pixelcnthist = new TH1I(ssize.str().c_str(),"",40, 0.5, 40.5);
    }

    bool maxwaschanged = result.maxcluster.second == nullptr;
    if(result.maxcluster.second == nullptr)
        result.maxcluster = std::make_pair(FindSize(clusters->front().second, nullptr),
                                           &(clusters->front().second));

    if(result.delayhist == nullptr)
    {
        static int sizecnt = 0;
        std::stringstream stime("");
        stime << "hist_timedistr_" << ++sizecnt;
        result.delayhist = new TH1I(stime.str().c_str(), "", 41, -20.5, 20.5);
    }

    for(auto& it : *clusters)
    {
        if(sqrtpixels)
            result.pixelcnthist->Fill(sqrt(it.second.size()));
        else
            result.pixelcnthist->Fill(it.second.size());
        if(it.second.size() > result.maxsize)
        {
            //calculate aspect ratio of the cluster to exclude the bands at the bottom from
            //  readout / detector overload:
            Extent* xtd = &it.first;
            double aspectratio = (xtd->endcol - xtd->startcol + 1) * 3.
                                    / (xtd->endrow - xtd->startrow + 1);

            if(aspectratio < 3 && aspectratio > 0.333)
            {
                result.maxsize = it.second.size();
                //delete extra container:
                if(maxwaschanged == false)
                    delete result.maxcluster.second;
                result.maxcluster = std::make_pair(FindSize(it.second, nullptr), &(it.second));
                maxwaschanged = true;
            }
        }
        for(auto& pit : it.second)
            result.delayhist->Fill(it.first.time - pit.timestamp);
    }
    //replace list of hits for largest cluster by a copy of the list as clusters will be deleted:
    if(maxwaschanged)
    {
        std::list<Dataset>* hits = new std::list<Dataset>();

        hits->insert(hits->end(), result.maxcluster.second->begin(),
                     result.maxcluster.second->end());

        if(result.maxcluster.second->size() != hits->size())
            std::cout << "max cluster size changed during copying!!!" << std::endl;

        result.maxcluster.second = hits;

        //create a new EMPTY histogram (somehow clear() did not actually clear the histogram
        //  bins as it should...)
        if(result.maxclustershape != nullptr)
            delete result.maxclustershape;
        static int sizecnt = 0;
        std::stringstream ssize("");
        ssize << "hist_maxcluster_" << ++sizecnt;
        result.maxclustershape = new TH2I(ssize.str().c_str(),"",132, -0.5,131.5,372,-0.5,371.5);
        result.maxclustershape->SetTitle("Maximum Sized Cluster");

        for(auto& it : *(result.maxcluster.second))
            result.maxclustershape->Fill(it.column, it.row);

        result.maxclusterpars = FindSize(*(result.maxcluster.second), nullptr, false);
        if(print)
        {
            TCanvas* c = DrawTH2(result.maxclustershape, nullptr, "Column", "Row", "");
            if(result.maxclusterpars.fit)
            {
                DrawCircle(c, result.maxclusterpars.cmsx, result.maxclusterpars.cmsy,
                           result.maxclusterpars.radius);
                std::cout << "Maximum Cluster Size: " << result.maxcluster.second->size()
                          << "\n  but it should have been: " << result.maxclusterpars.sizeestimate
                          << std::endl;
            }
        }
    }

    //evaluate cluster shape
    if(result.averageclustershape == nullptr)
    {
        static int sizecnt = 0;
        std::stringstream ssize("");
        ssize << "hist_averagecluster_" << ++sizecnt;
        result.averageclustershape = new TH2I(ssize.str().c_str(), "", 31, -15.5, 15.5,
                                                                       161, -80.5, 80.5);
    }
    for(auto& it: *clusters)
    {
        //calculate "center of mass" of cluster:
        double meanx = 0;
        double meany = 0;
        for(auto& pit : it.second)
        {
            meanx += pit.column; //TODO: add ToT as weight?
            meany += pit.row;
        }
        meanx /= it.second.size();
        meany /= it.second.size();
        //split between matrix halves to separate upper and lower half moons:
        if(meany < 186)
            meany += 40; //lower rows shifted down (minus sign below)
        else
            meany -= 40; //upper rows shifted up (minus sign below)

        for(auto& pit : it.second)
            result.averageclustershape->Fill(pit.column - meanx, pit.row - meany);
    }


    //better Cluster Size estimator:
    if(result.sizehist == nullptr)
    {
        static int sizecnt = 0;
        std::stringstream ssize("");
        ssize << "newestimatehist_" << ++sizecnt;
        result.sizehist = new TH1I(ssize.str().c_str(), "Size Histogram", 50, 0.5, 5000.5);
    }
    if(result.sizecorrelation == nullptr)
    {
        static int sizecnt = 0;
        std::stringstream ssize("");
        ssize << "sizecorrelationhist_" << ++sizecnt;
        result.sizecorrelation = new TH2I(ssize.str().c_str(), "Size Correlation",
                                          100, 0.5, 2000.5, 100, 0.5, 2000.5);
    }
    //std::vector<std::pair<clusterpos, std::list<Dataset>*> > exampleclusters;
    result.exampleclusters.clear(); //added to fix empty cluster entries...
    int index = 0;
    const int numclusters = clusters->size();
    const int sizewidth = log10(numclusters)+1;
    std::cout << "Processed by now:" << std::endl;
    AlignParams indexpars;
    for(auto& it : *clusters)
    {
        clusterpos size = FindSize(it.second, result.sizecorrelation);
        result.sizehist->Fill(size.radius);
        if(size.fit)
        {
            //reject clusters with less than 10% of the hits read out:
            if(it.second.size() < 0.6 * size.sizeestimate && it.second.size() > 0.1 * size.sizeestimate)
                result.exampleclusters.push_back(std::make_pair(size, &(it.second)));
        }
        //writing of the total number is only necessary for the first time as the line is reused:
        if(++index == 100)
            std::cout << Rightalign(index, sizewidth, &indexpars) << " / " << numclusters << "\r";
        else if((index % 100) == 0)
            std::cout << Rightalign(index, sizewidth, &indexpars) << "\r";
    }
    std::cout << std::endl;

    if(print)
    {
        TCanvas* c = DrawTH1(result.sizehist, nullptr, "Radius (in um)", "Counts"); //debug
        c->SetLogy(1);
    }

    if(print)
    {
        DrawTH2(result.sizecorrelation, nullptr, "pixel address estimate", "circle fit estimate", "Counts");
        //equality line as reference
        TGraph* grtest = new TGraph(0);
        grtest->SetPoint(0, 0, 0);
        grtest->SetPoint(1, 2000, 2000);
        grtest->SetLineColor(kRed);
        grtest->SetLineStyle(7); //dashed
        grtest->Draw("Lsame");
    }

    return result;
}

TH1I* GenerateRateHist(std::list<Dataset>* data, double timescale, double binwidth,
                       TH1I* lasthist = nullptr, bool clearhistory = false)
{
    if(data == nullptr || data->size() == 0)
        return nullptr;

    double start = 0;
    double end   = 0;

    auto it = data->begin();

    std::vector<long long> timestamps;
    timestamps.reserve(data->size());
    static std::deque<long long> tss;
    double avgdelay = -1e10;

    while(it != data->end())
    {
        //form local moving average and remove hits not close (>2.5e9) to it
        if(tss.size() < 100)
        {
            tss.push_back(it->timestamp);
            timestamps.push_back(it->timestamp);
        }
        else
        {
            if(!(avgdelay > -1e5 && std::abs(it->timestamp - avgdelay) > 2.5e9))
            {
                //count TS overflows:
                if(it->timestamp < timestamps.back())
                    if(it->timestamp + (longlong(1) << 38) < timestamps.back())
                        end += (longlong(1) << 40) * timescale;

                static long long sum = 0;

                sum += it->timestamp - tss.front();

                tss.pop_front();
                tss.push_back(it->timestamp);
                timestamps.push_back(it->timestamp);

                if(avgdelay < 0)
                {
                    sum = 0;
                    for(auto& del : tss)
                        sum += del;
                }

                avgdelay = 0.01 * sum;
            }
        }
        ++it;
    }

    start = timestamps.front() * timescale;
    end   += timestamps.back() * timescale;

    double oldlength = 0;
    if(lasthist != nullptr)
        oldlength = lasthist->GetBinCenter(lasthist->GetNbinsX()) + lasthist->GetBinWidth(1)/2.;

    //limit the covered interval to 2.8h (such a long measurement was not taken):
    if(oldlength + end - start > 10000)
        end = start + 10000 - oldlength;

    //std::cout << "oldlength: " << oldlength << std::endl
    //          << "start:     " << start << std::endl
    //          << "end:       " << end << std::endl
    //          << "timescale: " << timescale << std::endl
    //          << "binwidth:  " << binwidth << std::endl
    //          << "  -> nbins: " << (oldlength + end - start) / binwidth << std::endl;

    static int histcnt = 0;
    std::stringstream s("");
    s << "ratehist_" << ++histcnt;
    TH1I* hist = new TH1I(s.str().c_str(),"Rate Histogram",
                          (oldlength + end - start) / binwidth, 0, oldlength + end - start);

    if(lasthist != nullptr)
    {
        for(int i = 0; i <= lasthist->GetNbinsX(); ++i)
            hist->SetBinContent(i, lasthist->GetBinContent(i));

        delete lasthist;
    }

    start -= oldlength;

    for(unsigned int i = 0; i < timestamps.size(); ++i)
    {
        if(i > 0 && timestamps[i] < timestamps[i-1])
            if(timestamps[i] + (longlong(1) << 38) < timestamps[i-1])
                start -= (longlong(1) << 40) * timescale;
        hist->Fill(timestamps[i] * timescale - start);
    }

    if(clearhistory)
        tss.clear();

    return hist;
}

void FullToTHist(std::string filename, int tsstepdown, int ts2stepdown, int binscale,
                 bool graydecode = false)
{
    InputFile input;
    input.Open(filename);

    TH1I* hist = nullptr;
    TCanvas* c = nullptr;

    bool endreached = false;
    while(!endreached)
    {
        auto data = input.Read(1000000, true, graydecode);

        if(data->size() < 1000000 && input.Finished())
            endreached = true;

        hist = DecodeToT(data, 0, tsstepdown, ts2stepdown, binscale, hist);

        c = DrawTH1(hist, c, "ToT (in ns)", "Counts");

        delete data;
    }

    input.Close();
}

/// creates a 2D histogram showing the read out row indices over time
TH2I* GenerateAvgRow(std::list<Dataset>* data, double timescale, double binwidth,
                     double measurementlength, double combinerows = 1, TH2I* lasthist = nullptr)
{
    if(data == nullptr || data->size() == 0)
        return nullptr;

    double start = data->front().timestamp * timescale;
    double end   = data->back().timestamp * timescale;
    //TS overflow:
    if(end < start)
        end += (longlong(1) << 40) * timescale;

    double oldlength = 0;
    if(lasthist != nullptr)
        oldlength = lasthist->GetXaxis()->GetBinCenter(lasthist->GetNbinsX())
                      + lasthist->GetXaxis()->GetBinWidth(1);

    //limit the covered interval to a reasonable length taken from the rate hist:
    if(oldlength + end - start + 10 > measurementlength + binwidth && measurementlength > 1)
        end = measurementlength - oldlength + start;

    static int histcnt = 0;
    std::stringstream s("");
    s << "rowhist_" << ++histcnt;
    TH2I* hist = new TH2I(s.str().c_str(), "Row readout over time",
                          (oldlength + end-start)/binwidth, 0, oldlength + end - start,
                          372 / combinerows, -0.5, 371.5);

    if(lasthist != nullptr)
    {
        for(int x = 0; x <= lasthist->GetNbinsX(); ++x)
            for(int y = 0; y <= lasthist->GetNbinsY(); ++y)
                hist->SetBinContent(x,y, lasthist->GetBinContent(x,y));

        delete lasthist;
    }

    start -= oldlength;

    for(auto& it : *data)
        hist->Fill(it.timestamp * timescale - start, it.row);

    return hist;
}

TH2I* GenerateTSHist(std::list<Dataset>* data, bool chipts, TH2I* tshist = nullptr)
{
    if(data == nullptr || data->size() == 0)
        return nullptr;

    static int histcnt = 0;
    std::stringstream s("");
    s << "tshist_" << ++histcnt;

    //x axis scaling:
    int entries = data->size();
    if(tshist != nullptr)
        entries += tshist->Integral();
    int xbins = 1000;
    if(entries < xbins)
        xbins = entries;
    //y axis scaling:
    double maxts = 0xffffffffff + 0.5;
    double ybins = 4096;
    if(chipts)
    {
        maxts = 1023.5;
        ybins = 256;
    }
    int index = 0;

    TH2I* hist = new TH2I(s.str().c_str(), "", xbins, -0.5, entries - 0.5, ybins, -0.5, maxts);

    //add the old data to the new histogram:
    if(tshist != nullptr)
    {
        for(int x = 0; x <= tshist->GetNbinsX(); ++x)
            for(int y = 0; y <= tshist->GetNbinsY(); ++y)
                hist->Fill(tshist->GetXaxis()->GetBinCenter(x),
                           tshist->GetYaxis()->GetBinCenter(y), tshist->GetBinContent(x,y));
        index = tshist->Integral();
        delete tshist;
    }

    if(chipts)
    {
        for(auto& it : *data)
            hist->Fill(index++, it.ts);
    }
    else
    {
        for(auto& it : *data)
            hist->Fill(index++, it.timestamp);
    }

    return hist;
}

TH2I* GeneratePIDHist(std::list<Dataset>* data, TH2I* lasthist = nullptr)
{
    if(data == nullptr || data->size() == 0)
        return nullptr;

    static int histcnt = 0;
    std::stringstream s("");
    s << "pidhist_" << ++histcnt;

    //x axis scaling:
    int entries = data->size();
    if(lasthist != nullptr)
        entries += lasthist->Integral();
    int xbins = 1000;
    if(entries < xbins)
        xbins = entries;
    //y axis scaling:
    double maxts = 0xffff + 0.5;
    double ybins = 1024;

    int index = 0;

    TH2I* hist = new TH2I(s.str().c_str(), "", xbins, -0.5, entries - 0.5, ybins, -0.5, maxts);

    if(lasthist != nullptr)
    {
        for(int x = 0; x <= lasthist->GetNbinsX(); ++x)
            for(int y = 0; y <= lasthist->GetNbinsY(); ++y)
                hist->Fill(lasthist->GetXaxis()->GetBinCenter(x),
                           lasthist->GetYaxis()->GetBinCenter(y), lasthist->GetBinContent(x,y));
        index = lasthist->Integral();
        delete lasthist;
    }

    for(auto& it : *data)
        hist->Fill(index++, it.packageid);

    return hist;
}

bool WriteToFile(std::string filename, std::string data)
{
    std::fstream f;
    f.open(filename.c_str(), std::ios::out | std::ios::app);
    if(!f.is_open())
        return false;
    f << data << std::endl;
    f.close();

    return true;
}

TCanvas* KeepClean(bool keepclean, TCanvas* c)
{
    if(keepclean && c != nullptr)
    {
        delete c;
        return nullptr;
    }
    else
        return c;
}

TCanvas* Save(TCanvas* c, std::string filename,
          TFile* outfile, std::string roottitle, bool keepclean)
{
    if(filename != "" && c != nullptr)
    {
        c->SaveAs(filename.c_str());
        if(outfile != nullptr)
            outfile->WriteObject(c, roottitle.c_str());
    }
    return KeepClean(keepclean, c);
}

/**
 * @brief Analysis analysis method to call all other methods to perform everything in one go
 * @param filename           - input data to analyse
 * @param restrictive        - puts stricter restrictions on rejecting hits for the analysis
 * @param outputprefix       - prefix for storing plots, nothing will be saved on an empty string
 * @param saveram            - deletes large structures after plotting if saving is turned on
 * @param keepclean          - deletes all plots after saving them to HDD
 */
void Analysis(std::string filename, bool restrictive, std::string outputprefix = "",
              bool saveram = false, bool keepclean = false, bool sqrtpixels = true)
{
    InputFile datasource;
    if(!datasource.Open(filename))
    {
        datasource.Close();
        return;
    }

    TFile* outfile = nullptr;

    if(outputprefix != "")
    {
        std::stringstream s("");
        s << "#############################################\n"
          << "# Analysis of \"" << filename << "\"\n"
          << "#############################################\n";

        s << "\n  call parameters:"
          << "\n      filename:     \"" << filename << "\""
          << "\n      restrictive:  " << ((restrictive)?"true":"false")
          << "\n      outputprefix: \"" << outputprefix << "\""
          << "\n      saveram:      " << ((saveram)?"true":"false")
          << "\n      keepclean:    " << ((keepclean)?"true":"false")
          << std::endl;

        WriteToFile(outputprefix + "_pars.dat", s.str());

        //open root file for the outputs:
        outfile = new TFile((outputprefix + ".root").c_str(), "RECREATE", outputprefix.c_str());
    }

    //objects used to accumulate the data:
    unsigned long long missingpackages  =  0;
    int                lastid           = -1;
    unsigned long long totalpackages    =  0;

    unsigned long long totalhits         =  0;
    TH2I*              hitmap            = nullptr;
    TCanvas*           chitmap           = nullptr;

    TH1I*              histtot           = nullptr;
    TCanvas*           ctot              = nullptr;

    TH2I*              htschip           = nullptr;
    TCanvas*           ctschip           = nullptr;
    TH2I*              htsfpga           = nullptr;
    TCanvas*           ctsfpga           = nullptr;

    TH1I*              histrate          = nullptr;
    TCanvas*           crate             = nullptr;

    TH2I*              histtimerow       = nullptr;
    TCanvas*           ctimerow          = nullptr;

    TH2I*              histpid           = nullptr;
    TCanvas*           cpid              = nullptr;

    unsigned long long numberofclusters  = 0;
    std::list<std::pair<Extent, std::list<Dataset> > >* clusters = nullptr;
    std::list<std::pair<Extent, std::list<Dataset> > >* lastclusters = nullptr;
    ClusteringResult   fullclusterresult = ClusteringResult();
    TCanvas*           cpxcnt            = nullptr;
    TCanvas*           csize             = nullptr;
    TCanvas*           csizecorr         = nullptr;
    std::stringstream  ssizeest("");
    TCanvas*           cdel              = nullptr;
    TCanvas*           cavgshape         = nullptr;
    TCanvas*           cmaxcluster       = nullptr;
    std::pair<clusterpos, std::list<Dataset>*> mostmissingcluster = std::make_pair(clusterpos(),
                                                                                        nullptr);
    TCanvas*           cmostmissing      = nullptr;
    unsigned int       missingpixelhits  = 0;
    TGraph*            grquotient        = new TGraph(0);
    unsigned int       maxrealhits       = 0;
    unsigned int       maxestimatehits   = 0;
    TCanvas*           cquotient         = nullptr;
    TGraph*            grlimit           = new TGraph(0);
    TLatex*            limitlabel        = nullptr;


    //start a loop until end of file i.e. less than requested hits were read
    bool endreached = false;

    while(!endreached)
    {
        //read next bunch of data:
        std::list<Dataset>* fullset = datasource.Read(1000000, true);

        if(fullset == nullptr)
        {
            std::cout << "no data loaded. Aborting" << std::endl;
            break;
        }
        else if(fullset->size() < 1000000 && datasource.Finished())
            endreached = true;

        //package ID scanning:
        if(lastid == -1)
            lastid = fullset->front().packageid;
        int pidoverflows = 0;
        for(auto& it : *fullset)
        {
            if(it.packageid != lastid)
            {
                if(lastid < it.packageid)
                    missingpackages += it.packageid - lastid - 1;
                else
                {
                    missingpackages += it.packageid + 65535 - lastid;
                    ++pidoverflows;
                }
                lastid = it.packageid;
            }
        }
        if(fullset->size() > 0)
            totalpackages += fullset->back().packageid - fullset->front().packageid + 1
                              + 65535 * pidoverflows;
        totalhits += fullset->size();

        RemoveInvalidHits(fullset, restrictive);

        std::stringstream sp("");
        sp << "# General:\n"
           << "   Missing packages:      " << missingpackages << "/" << totalpackages
           << "\n   Total number of Hits:  " << fullset->size();
        std::cout << sp.str() << std::endl;


        //Beam Spot:
        hitmap  = DrawHitMap(fullset, 0, 1, 1, "Beam Spot", hitmap);
        chitmap = DrawTH2(hitmap, chitmap, "Column (in 150 um)", "Row (in 50 um)", "Counts",
                          "colz", false);
        if(endreached && outputprefix != "")
            chitmap = Save(chitmap, outputprefix + "_hitmap.pdf", outfile, "HitMap", keepclean);

        //ToT:
        histtot = DecodeToT(fullset, 0, 2, 1, 2, histtot);
        if(ctot == nullptr)
            histtot->SetTitle("ToT Distribution");
        ctot    = DrawTH1(histtot, ctot, "ToT (in ns)", "Counts");
        if(endreached && outputprefix != "")
            ctot = Save(ctot, outputprefix + "_ToT.pdf", outfile, "ToT", keepclean);

        //Timestamp:
        htschip = GenerateTSHist(fullset, true, htschip);
        htschip->SetTitle("Timestamps in the Data");
        htschip->GetYaxis()->SetTitleOffset(1.1);
        ctschip = DrawTH2(htschip, ctschip, "Hit Index", "TS (in 25ns)", "Entries", "colz", true);
        MoveStatsBox(ctschip, 0.62, 0.15,0.82,0.39, "tschip_stats_");
        if(endreached && outputprefix != "")
            ctschip = Save(ctschip, outputprefix + "_TimeStampsChip.pdf", outfile,
                           "Timestamps_Chip", keepclean);
        ctschip = KeepClean(keepclean, ctschip);
        htsfpga = GenerateTSHist(fullset, false, htsfpga);
        htsfpga->SetTitle("Time Stamps (FPGA) in the Data");
        ctsfpga = DrawTH2(htsfpga, ctsfpga, "Hit Index", "TS (in 25ns)", "Entries", "colz", true);
        MoveStatsBox(ctsfpga, 0.62, 0.15,0.82,0.39, "tsfpga_stats_");
        if(endreached && outputprefix != "")
            ctsfpga = Save(ctsfpga, outputprefix + "_TimestampsFPGA.pdf", outfile,
                           "Timestamps_FPGA", keepclean);

        //rate histogram:
        histrate = GenerateRateHist(fullset, 5e-8, 0.02, histrate, endreached); //5s are about 2.5e9 time steps
        crate = DrawTH1(histrate, crate, "Time (in s)", "Rate (in Hits/time bin)");
        if(endreached && outputprefix != "")
            crate = Save(crate, outputprefix + "_HitRate.pdf", outfile, "HitRate", keepclean);

        //correlation of row with time:
        double measurementlength = 0;
        if(histrate != nullptr)
            measurementlength = histrate->GetBinCenter(histrate->GetNbinsX());
        histtimerow = GenerateAvgRow(fullset, 5e-8, 0.05, measurementlength, 6, histtimerow);
        ctimerow = DrawTH2(histtimerow, ctimerow, "Time (in s)", "Row", "Number of Hits", "colz",
                           false);
        if(endreached && outputprefix != "")
            ctimerow = Save(ctimerow, outputprefix + "_RowsOverTime.pdf", outfile, "RowsOverTime",
                            keepclean);

        //Package IDs:
        histpid = GeneratePIDHist(fullset, histpid);
        histpid->SetTitle("PackageIDs of the Data");
        cpid    = DrawTH2(histpid, cpid, "Hit Index", "Package ID", "Entries", "colz", true);
        histpid->GetYaxis()->SetTitleOffset(1.2);
        MoveStatsBox(cpid, 0.62, 0.15,0.82,0.39, "pid_stats_");
        if(endreached && outputprefix != "")
            cpid = Save(cpid, outputprefix + "_packages.pdf", outfile, "PackageIDs", keepclean);

        //Clustering:
        int lastclusterssize = 0;
        if(lastclusters != nullptr)
            delete lastclusters;
        if(clusters != nullptr)
        {
            lastclusters = clusters;
            lastclusterssize = clusters->size();
        }

        clusters = Clusterise(fullset, 200e-6, 300e-9, saveram, lastclusters);  //300e-6, 300e-9);
        for(auto& it : *clusters)
            if(it.first.previoussize == 0)
                ++numberofclusters;
        std::cout << "Clustering done. now analysing them..." << std::endl;
        fullclusterresult = AnalyseClusters(clusters, &fullclusterresult, sqrtpixels);

        //clustering result general numbers:
        {
            std::stringstream s("");
            s <<   "   number of Clusters:    " << numberofclusters
              << "\n     -> average # pixels per cluster: "
                    << double(totalhits) / numberofclusters;

            std::cout << s.str() << std::endl;
        }

        if(fullclusterresult.pixelcnthist != nullptr)
        {
            fullclusterresult.pixelcnthist->SetTitle("Cluster Size Distribution (from # pixels)");
            cpxcnt = DrawTH1(fullclusterresult.pixelcnthist, cpxcnt,
                        (sqrtpixels)?"Cluster Radius (sqrt(# pixels))":"Cluster Radius (# pixels)",
                             "Counts");
            cpxcnt->SetLogy(1);
            cpxcnt->Update();
            if(endreached && outputprefix != "")
                cpxcnt = Save(cpxcnt, outputprefix + "_pixelspercluster.pdf", outfile,
                              "PixelsPerCluster", keepclean);
        }
        if(fullclusterresult.sizehist != nullptr)
        {
            //only evaluate if there are actually clusters of sufficient size:
            if(fullclusterresult.sizehist->Integral(300, 5000) > 0)
            {
                //fit the decrease of events with size:
                static int fitcnt = 0;
                std::stringstream sn("");
                sn << "fitdec_" << ++fitcnt;
                TF1* fitfunc = new TF1(sn.str().c_str(), "[0]*exp(-(x-[1])/[2])", 300, 5000);
                fitfunc->SetParameter(0, fullclusterresult.sizehist->GetBinContent(3));
                fitfunc->SetParameter(1, 200);
                fitfunc->SetParameter(2, 3);
                fullclusterresult.sizehist->Fit(fitfunc, "Q0R");

                bool fitsuccess = TestFitSuccess();

                fullclusterresult.sizehist->SetTitle("Cluster Radius Distribution (from border fit)");
                csize = DrawTH1(fullclusterresult.sizehist, csize, "Cluster Radius (in um)", "Counts");
                csize->SetLogy(1);
                if(fitsuccess)
                    fitfunc->Draw("Lsame");
                csize->Update();
                if(endreached && outputprefix != "")
                    csize = Save(csize, outputprefix + "_clustersize.pdf", outfile, "ClusterSize",
                                 keepclean);

                double meansize = fullclusterresult.sizehist->GetMean();
                int binatmean = fullclusterresult.sizehist->FindBin(meansize);
                int binat3mean = fullclusterresult.sizehist->FindBin(3 * meansize);
                int binat10mean = fullclusterresult.sizehist->FindBin(10 * meansize);
                double stepdown3 = fullclusterresult.sizehist->GetBinContent(binat3mean)
                                    / fullclusterresult.sizehist->GetBinContent(binatmean);
                double stepdown10 = fullclusterresult.sizehist->GetBinContent(binat10mean)
                                    / fullclusterresult.sizehist->GetBinContent(binatmean);

                ssizeest.str("");
                ssizeest <<   "# Cluster Size Distribution Parameters:"
                         << "\n   average cluster size:   " << meansize << " um"
                         << "\n   fraction avg to  3x avg: " << stepdown3
                         << "\n   fraction avg to 10x avg: " << stepdown10;

                if(fitsuccess)
                {
                    ssizeest << "\n  Fit: f(x) = [0] * exp(-(x-[1])/[2])"
                     << "\n     [0] " << fitfunc->GetParameter(0) << " +- " << fitfunc->GetParError(0)
                     << "\n     [1] " << fitfunc->GetParameter(1) << " +- " << fitfunc->GetParError(1)
                     << "\n     [2] " << fitfunc->GetParameter(2) << " +- " << fitfunc->GetParError(2);
                }
                else
                    ssizeest << "\n  Fit failed";


                std::cout << ssizeest.str() << std::endl;
                if(keepclean)
                    delete fitfunc;
            }
        }
        if(fullclusterresult.sizecorrelation != nullptr)
        {
            fullclusterresult.sizecorrelation->SetTitle(
                                    "Cluster Size Estimate Correlation with Hit Number");
            fullclusterresult.sizecorrelation->GetYaxis()->SetTitleOffset(1.1);
            csizecorr = DrawTH2(fullclusterresult.sizecorrelation, csizecorr,
                                "Cluster Radius from outermost Hits (in um)",
                                "Cluster Radius from Border Fit (in um)", "Counts", "colz", true);
            csizecorr->Update();
            MoveStatsBox(csizecorr, 0.62, 0.15, 0.82, 0.39, "stats_est_");

            //add line for unity relation:
            TGraph* grline = new TGraph(0);
            grline->SetPoint(0, 0, 0);
            grline->SetPoint(1, 2000, 2000);
            grline->SetLineColor(kRed);
            grline->SetLineStyle(7);
            grline->Draw("Lsame");

            if(endreached && outputprefix != "")
                csizecorr = Save(csizecorr, outputprefix + "_sizecorrelation.pdf", outfile,
                                 "SizeCorrelation", keepclean);

            if(keepclean)
                delete grline;
        }
        if(fullclusterresult.delayhist != nullptr)
        {
            fullclusterresult.delayhist->SetTitle("Time Difference in a cluster");
            cdel = DrawTH1(fullclusterresult.delayhist, cdel, "Time Difference (in 25ns)",
                           "Counts");
            cdel->SetLogy(1);
            cdel->Update();
            if(endreached && outputprefix != "")
                cdel = Save(cdel, outputprefix + "_clustertimedifference.pdf", outfile,
                     "ClusterTimeDifference", keepclean);
        }

        if(fullclusterresult.averageclustershape != nullptr)
        {
            fullclusterresult.averageclustershape->SetTitle("Average Cluster Shape");
            cavgshape = DrawTH2(fullclusterresult.averageclustershape, cavgshape, "Column", "Row",
                                "Count", "colz", false);
            cavgshape->SetLogz(1);
            cavgshape->Update();
            if(endreached && outputprefix != "")
                cavgshape = Save(cavgshape, outputprefix + "_clustershape.pdf", outfile,
                                 "ClusterShape", keepclean);
        }
        if(fullclusterresult.maxclustershape != nullptr)
        {
            fullclusterresult.maxclustershape->SetTitle("Largest Cluster Found");
            fullclusterresult.maxclustershape->GetZaxis()->SetRangeUser(0, 10);
            cmaxcluster = DrawTH2(fullclusterresult.maxclustershape, cmaxcluster, "Column", "Row",
                                  "Hits", "colz", true);
            DrawCircle(cmaxcluster, fullclusterresult.maxclusterpars.cmsx,
                       fullclusterresult.maxclusterpars.cmsy,
                       fullclusterresult.maxclusterpars.radius * 0.02);

            cmaxcluster->Update();
            if(fullclusterresult.maxclusterpars.cmsx + fullclusterresult.maxclusterpars.radius
                    * 0.02 / 3. < 80
                || fullclusterresult.maxclusterpars.cmsy - fullclusterresult.maxclusterpars.radius
                    * 0.02 > 150)
                MoveStatsBox(cmaxcluster, 0.62, 0.15, 0.82, 0.39, "stats_largest_");
            else
                MoveStatsBox(cmaxcluster, 0.16, 0.15, 0.36, 0.39, "stats_largest_");


            if(endreached && outputprefix != "")
                cmaxcluster = Save(cmaxcluster, outputprefix + "_maxcluster.pdf", outfile,
                                   "MaxCluster", keepclean);

            std::stringstream s("");
            s <<   "# Stats on largest Cluster:"
              << "\n   number of pixel hits read out: " << fullclusterresult.maxsize
              << "\n   estimate of total hits:        "
                        << fullclusterresult.maxclusterpars.sizeestimate
              << "\n   cluster radius:                " << fullclusterresult.maxclusterpars.radius
              << " um";

            std::cout << s.str() << std::endl;
        }
        if(fullclusterresult.exampleclusters.size() > 0
                || (endreached && mostmissingcluster.second != nullptr))
        {
            grquotient->SetTitle("Size to Size-Estimate Correlation");
            bool maxwaschanged = mostmissingcluster.second == nullptr;
            if(mostmissingcluster.second == nullptr)
                mostmissingcluster = fullclusterresult.exampleclusters.front();
            auto max = &mostmissingcluster;

            //auto max = fullclusterresult.exampleclusters.front();
            for(auto& it : fullclusterresult.exampleclusters)
            {
                //calculate aspect ratio of the hit to exclude the bands at the bottom of the
                //  matrix due to detector / readout overload:

                //  find cluster extent:
                int minx = 131, miny = 371, maxx = 0, maxy = 0;
                for(auto& pit : *(it.second))
                {
                    if(pit.column < minx)
                        minx = pit.column;
                    if(pit.column > maxx)
                        maxx = pit.column;
                    if(pit.row < miny)
                        miny = pit.row;
                    if(pit.row > maxy)
                        maxy = pit.row;
                }

                double aspectratio = (maxx - minx + 1) * 3. / (maxy - miny + 1);

                //reject everything wider than 3 times the height:
                if(aspectratio < 3 && aspectratio > 0.333)
                {

                    if(pow(max->first.sizeestimate, 2) / max->second->size()
                        < pow(it.first.sizeestimate, 2) / it.second->size())
                    {
                        if(!maxwaschanged)
                            delete max->second;
                        *max = it;
                        maxwaschanged = true;
                    }

                    if(it.second->size() > maxrealhits)
                        maxrealhits = it.second->size();
                    if(it.first.sizeestimate > maxestimatehits)
                        maxestimatehits = it.first.sizeestimate;

                    if(it.first.sizeestimate > it.second->size())
                        missingpixelhits += it.first.sizeestimate - it.second->size();
                    else
                        std::cout << "Error: overcomplete cluster found:" << std::endl
                                  << "   estimate: " << it.first.sizeestimate << std::endl
                                  << "   size:     " << it.second->size() << std::endl;

                    grquotient->SetPoint(grquotient->GetN(), it.first.sizeestimate, it.second->size());
                }
            }
            //replace content of cluster with most missing pixels by a persistent list:
            if(maxwaschanged)
            {
                std::list<Dataset>* liste = new std::list<Dataset>();

                liste->insert(liste->end(), max->second->begin(), max->second->end());
                max->second = liste;
            }

            grquotient->GetYaxis()->SetRangeUser(0, 1.1 * maxrealhits);
            cquotient = DrawTGraph(grquotient, cquotient, "Size Estimate (in Pixels)",
                                   "Read Out Hits", "AP*");
            //TGraph* grlimit = new TGraph(0);
            if(grlimit->GetN() == 0)
            {
                grlimit->SetPoint(0, 10000, 1000);
                grlimit->SetPoint(1, 200, 20);
                grlimit->SetPoint(2, 33.333, 20);
                grlimit->SetPoint(3, 10000, 6000);
                grlimit->SetLineColor(kBlack);
                grlimit->SetLineWidth(2);
                grlimit->SetLineStyle(7);
            }
            grlimit->Draw("Lsame");
            if(limitlabel != nullptr)
                delete limitlabel;
            limitlabel = new TLatex(0.7 * maxestimatehits, 0.7 * maxrealhits, "allowed area");
            limitlabel->SetTextSize(0.05);
            limitlabel->Draw();

            if(endreached && outputprefix != "")
                cquotient = Save(cquotient, outputprefix + "_missingfraction.pdf", outfile,
                                 "MissingFraction", keepclean);

            TH2I* maxhist = DrawHitMap(max->second);
            maxhist->SetTitle("Largest Cluster from Reconstruction");
            maxhist->GetZaxis()->SetRangeUser(0, 10);
            cmostmissing = DrawTH2(maxhist, cmostmissing, "Column", "Row", "Hit", "colz", true);
            DrawCircle(cmostmissing, max->first.cmsx, max->first.cmsy, max->first.radius * 0.02);
            cmostmissing->Update();
            if(max->first.cmsx + max->first.radius * 0.02 / 3. < 80
                || max->first.cmsy - max->first.radius * 0.02 > 150)
                MoveStatsBox(cmostmissing, 0.62, 0.15,0.82,0.39, "stats_rec_");
            else
                MoveStatsBox(cmostmissing, 0.16, 0.15, 0.36,0.39, "stats_rec_");

            if(endreached && outputprefix != "")
                cmostmissing = Save(cmostmissing, outputprefix + "_maxrecluster.pdf", outfile,
                                    "MaxReCluster", keepclean);

            std::stringstream s("");
            s << "# Missing Hits from cluster fits:\n"
              << "    missing hits: " << missingpixelhits << std::endl
              << "        -> " << double(missingpixelhits) / totalhits * 100  << " %" << std::endl;
            s <<   "# Stats on largest Cluster with large missing fraction:"
              << "\n   number of pixel hits read out: " << max->second->size()
              << "\n   estimate of total hits:        " << max->first.sizeestimate
              << "\n   cluster radius:                " << max->first.radius << " um";

            std::cout << s.str() << std::endl;
        }

        delete fullset;
    }
    if(clusters != nullptr)
        delete clusters;
    if(lastclusters != nullptr)
        delete lastclusters;
    //end of loop area
    datasource.Close();

    //final output generation:
    if(outputprefix != "")
    {
        //  general numbers:
        std::stringstream s("");
        s <<   "# General:"
          << "\n   Missing packages:      " << missingpackages << "/" << totalpackages
          << "\n   Total number of Hits:  " << totalhits;
        std::cout << s.str() << std::endl;
        WriteToFile(outputprefix + "_pars.dat", s.str());

        //  clustering:
        s.str("");
        s <<   "   number of Clusters:    " << numberofclusters
          << "\n     -> average # pixels per cluster: "
                << double(totalhits) / numberofclusters;

        std::cout << s.str() << std::endl;
        WriteToFile(outputprefix + "_pars.dat", s.str());

        //  size fit:
        std::cout << ssizeest.str() << std::endl;
        WriteToFile(outputprefix + "_pars.dat", ssizeest.str());

        //  largest cluster:
        s.str("");
        s <<   "# Stats on largest Cluster:"
          << "\n   number of pixel hits read out: " << fullclusterresult.maxsize
          << "\n   estimate of total hits:        "
                    << fullclusterresult.maxclusterpars.sizeestimate
          << "\n   cluster radius:                " << fullclusterresult.maxclusterpars.radius
          << " um";

        std::cout << s.str() << std::endl;
        WriteToFile(outputprefix + "_pars.dat", s.str());

        //  largest cluster with largest missing fraction:
        s.str("");
        s << "# Missing Hits from cluster fits:\n"
          << "    missing hits: " << missingpixelhits << std::endl
          << "        -> " << double(missingpixelhits) / totalhits * 100  << " %" << std::endl;
        s <<   "# Stats on largest Cluster with large missing fraction:"
          << "\n   number of pixel hits read out: " << mostmissingcluster.second->size()
          << "\n   estimate of total hits:        " << mostmissingcluster.first.sizeestimate
          << "\n   cluster radius:                " << mostmissingcluster.first.radius << " um";

        std::cout << s.str() << std::endl;
        WriteToFile(outputprefix + "_pars.dat", s.str());
    }


    if(keepclean)
    {
        delete outfile;
        delete hitmap;
        delete chitmap;
        delete histtot;
        delete ctot;
        delete htschip;
        delete ctschip;
        delete htsfpga;
        delete ctsfpga;
        delete histrate;
        delete crate;
        delete histtimerow;
        delete ctimerow;
        delete histpid;
        delete cpid;

        delete fullclusterresult.pixelcnthist;
        fullclusterresult.pixelcnthist = nullptr;
        delete cpxcnt;
        delete fullclusterresult.sizehist;
        fullclusterresult.sizehist = nullptr;
        delete csize;
        delete fullclusterresult.sizecorrelation;
        fullclusterresult.sizecorrelation = nullptr;
        delete csizecorr;
        delete fullclusterresult.delayhist;
        fullclusterresult.delayhist = nullptr;
        delete cdel;
        delete fullclusterresult.averageclustershape;
        fullclusterresult.averageclustershape = nullptr;
        delete cavgshape;
        delete fullclusterresult.maxcluster.second;
        delete fullclusterresult.maxclustershape;
        fullclusterresult.maxclustershape = nullptr;
        delete cmaxcluster;
        fullclusterresult.exampleclusters.clear(); //content is already deleted from `delete clusters;`

        delete cmostmissing;
        delete grquotient;
        delete cquotient;
        delete grlimit;
        delete limitlabel;

        outfile->Close();
    }

    return;
}

/**
 * @brief FastAnalysis analysis method to call all other methods to perform everything in one go
 *      Analysis() is called with input and output strings generated from the identifier given
 * @param identifier         - part of the filename and output prefix as well as folder name:
 *                                  filename = "hit_" + identifier + "_decoded.dat"
 *                                  outputprefix = (output)?identifier + "/" + identifier : ""
 * @param restrictive        - puts stricter restrictions on rejecting hits for the analysis
 * @param output             - generate output or not
 * @param saveram            - deletes large structures after plotting if saving is turned on
 * @param keepclean          - deletes all plots after saving them to HDD
 */
void FastAnalysis(std::string identifier, bool restrictive, bool output = false,
                  bool saveram = false, bool keepclean = false)
{
    std::stringstream input("");
    input << "hit_" << identifier << "_decoded.dat";
    std::stringstream outputstr("");
    outputstr << identifier << "/" << identifier;

    std::cout << "Analysing \"" << input.str() << "\" ..." << std::endl;

    Analysis(input.str(), restrictive, (output)?outputstr.str():"", saveram, keepclean);
}


/***********************************************************
 * Starting from here, Leakage Current measurement methods
 ***********************************************************/

struct Datum{
    int year;
    int dayofyear;
    int hour;
    int minute;
    int second;
    int millisecond;

    Datum() : year(0), dayofyear(0), hour(0), minute(0), second(0), millisecond(0) {}

    Datum operator-(const Datum& ref) const
    {
        Datum diff;
        diff.millisecond = millisecond - ref.millisecond;
        if(diff.millisecond < 0)
        {
            diff.millisecond += 1000;
            diff.second = -1;
        }
        diff.second += second - ref.second;
        if(diff.second < 0)
        {
            diff.second += 60;
            diff.minute = -1;
        }
        diff.minute += minute - ref.minute;
        if(diff.minute < 0)
        {
            diff.minute += 60;
            diff.hour = -1;
        }
        diff.hour += hour - ref.hour;
        if(diff.hour < 0)
        {
            diff.hour += 24;
            diff.dayofyear = -1;
        }
        diff.dayofyear += dayofyear - ref.dayofyear;
        if(diff.dayofyear < 0)
        {
            diff.dayofyear += 365;
            diff.year = -1;
        }
        diff.year += year - ref.year;

        return diff;
    }

    double ToSeconds()
    {
        return 60 * (60 * (24 * (365 * year + dayofyear) + hour) + minute)
                + second + millisecond * 0.001;
    }

    ///assume "dd.mm.yy" as format
    bool FillDate(std::string date)
    {
        if(date.length() != 8)
            return false;

        year = (date[6]-48) * 10 + (date[7] - 48);
        dayofyear = (date[0] - 48) * 10 + (date[1] - 48);
        int month = (date[3] - 48) * 10 + (date[4] - 48);
        for(int i = month; i > 1; --i)
        {
            switch(i)
            {
            case(1):
            case(3):
            case(5):
            case(7):
            case(8):
            case(10):
            case(12):
                dayofyear += 31;
                break;
            case(4):
            case(6):
            case(9):
            case(11):
                dayofyear += 30;
                break;
            case(2):
                dayofyear += 28;
                break;
            default:
                std::cout << "Error: month should not be <= 0" << std::endl;
                break;
            }
        }

        return true;
    }

    bool FillTime(std::string time)
    {
        if(time.length() != 12)
            return false;

        hour = (time[0]- 48) * 10 + (time[1] - 48);
        minute = (time[3] - 48) * 10 + (time[4] - 48);
        second = (time[6] - 48) * 10 + (time[7] - 48);
        millisecond = ((time[9] - 48) * 10 + (time[10] - 48)) * 10 + (time[11] - 48);

        return true;
    }

    std::string ToString()
    {
        std::stringstream s("");
        int day = dayofyear;
        int month = 1;
        bool run = true;
        while(run)
        {
            switch(month)
            {
            case(1):
            case(3):
            case(5):
            case(7):
            case(8):
            case(10):
            case(12):
                if(day > 31)
                {
                    day -= 31;
                    ++month;
                }
                else
                    run = false;
                break;
            case(2):
                if(day > 28)
                {
                    day -= 28;
                    ++month;
                }
                else
                    run = false;
                break;
            case(4):
            case(6):
            case(9):
            case(11):
                if(day > 30)
                {
                    day -= 30;
                    ++month;
                }
                else
                    run = false;
                break;
            default:
                std::cout << "This should not happen (month < 0 | month > 12)" << std::endl;
                run = false;
                break;
            }
        }

        s << ((day < 10)?"0":"") << day << ((month < 10)?".0":".") << month
          << ((year < 10)?".0":".") << year
          << ((hour < 10)?" 0":" ") << hour << ((minute < 10)?":0":":") << minute
          << ((second < 10)?":0":":") << second
          << ((millisecond < 10)?".00":((millisecond < 100)?".0":".")) << millisecond;

        return s.str();
    }
};

TGraph* LoadLeakageFile(std::string filename)
{
    if(filename == "")
        return nullptr;
    std::fstream f;
    f.open(filename.c_str(), std::ios::in);
    if(!f.is_open())
        return nullptr;

    std::vector<Datum> date;
    std::vector<double> current;
    double value = 0;

    //read header line:
    std::string datestr, timestr;
    std::getline(f, timestr);

    f >> datestr >> timestr >> value;
    while(!f.eof())
    {
        Datum dat;
        if(!dat.FillDate(datestr))
            std::cout << "set Date failed! -> \"" << datestr << "\"" << std::endl;
        if(!dat.FillTime(timestr))
            std::cout << "set Time failed! -> \"" << timestr << "\"" << std::endl;
        date.push_back(dat);
        current.push_back(value);

        f >> datestr >> timestr >> value;
    }

    f.close();

    if(date.size() == 0)
        return nullptr;

    Datum offset = date.front();

    TGraph* gr = new TGraph(0);
    auto curit = current.begin();
    for(auto& it : date)
    {
        it = it - offset;
        gr->SetPoint(gr->GetN(), it.ToSeconds(), *curit);
        ++curit;
    }

    date.clear();
    current.clear();

    gr->SetTitle(("Leakage after " + offset.ToString()).c_str());

    return gr;
}

TGraph* Smoothen(TGraph* gr, unsigned int numpoints)
{
    if(gr == nullptr)
        return nullptr;
    if(numpoints == 0)
    {
        TGraph* gr2 = new TGraph();
        for(int i = 0; i < gr->GetN(); ++i)
            gr2->SetPoint(gr2->GetN(), gr->GetX()[i], gr->GetY()[i]);
        return gr2;
    }
    TGraph* grsmooth = new TGraph(0);

    grsmooth->SetTitle((gr->GetTitle() + std::string(" Smoothed")).c_str());

    for(int i = 0; i < gr->GetN(); ++i)
    {
        int start = i - numpoints;
        if(start < 0)
            start = 0;
        int ende = i + numpoints;
        if(ende >= gr->GetN())
            ende = gr->GetN()-1;
        double value = 0;
        for(int pos = start; pos <= ende; ++pos)
            value += gr->GetY()[pos];
        value /= ende - start + 1;
        grsmooth->SetPoint(grsmooth->GetN(), gr->GetX()[i], value);
    }

    return grsmooth;
}

TGraph* Derive(TGraph* gr)
{
    if(gr == nullptr || gr->GetN() < 2)
        return nullptr;

    TGraph* grdiff = new TGraph();

    grdiff->SetTitle((gr->GetTitle() + std::string(" diff")).c_str());

    for(int i = 0; i < gr->GetN() - 1; ++i)
    {
        const double x1 = gr->GetX()[i];
        const double x2 = gr->GetX()[i+1];

        grdiff->SetPoint(grdiff->GetN(), x1, (gr->GetY()[i+1] - gr->GetY()[i]) / (x2 - x1));
    }

    return grdiff;
}

TGraph* Absolute(TGraph* gr)
{
    if(gr == nullptr)
        return nullptr;

    TGraph* grabs = new TGraph();

    for(int i = 0; i < gr->GetN(); ++i)
        grabs->SetPoint(grabs->GetN(), gr->GetX()[i], std::abs(gr->GetY()[i]));

    return grabs;
}

std::vector<double> FindSteps(TGraph* grabsdiff, unsigned int baselength = 50, double sigmas = 3,
                              int offset = 0)
{
    if(grabsdiff == nullptr)
        return std::vector<double>();

    std::vector<double> steps;

    std::deque<double> baseline;

    for(int i = 0; i < grabsdiff->GetN(); ++i)
    {
        if(baseline.size() >= baselength)
        {
            //calculate mean and RMS:
            double bl = 0;  //mean
            double bl2 = 0; //RMS
            for(auto& it : baseline)
            {
                bl += it;
                bl2 += pow(it, 2);
            }
            bl /= baseline.size();
            bl2 /= baseline.size();
            bl2 = sqrt(bl2 - pow(bl, 2));

            //add a minimum value for detecting a step:
            if(bl + sigmas * bl2 < 5e-9)
                bl = 5e-9 - sigmas * bl2;

            if(bl + sigmas * bl2 < grabsdiff->GetY()[i])
            {
                int maxoffset = 0;
                for(int j = -10; j <= 10; ++j)
                    if(grabsdiff->GetY()[i+j] > grabsdiff->GetY()[i+maxoffset])
                        maxoffset = j;

                steps.push_back(grabsdiff->GetX()[i + maxoffset + offset]);
                //std::cout << "step found @ " << grabsdiff->GetX()[i] << std::endl;
                baseline.clear();

                //skip data until "end" of the pulse:
                for(int j = 0; j < 10; ++j)
                {
                    if(grabsdiff->GetY()[i+maxoffset+j] < 0.5 * grabsdiff->GetY()[i+maxoffset])
                    {
                        i = i + maxoffset + j;
                        break;
                    }
                }
            }
        }

        while(baseline.size() >= baselength)
            baseline.pop_front();

        baseline.push_back(grabsdiff->GetY()[i]);
    }

    return steps;
}

TGraph* FindAverage(TGraph* gr, std::vector<double>& steps)
{
    if(gr == nullptr)
        return nullptr;

    TGraph* gravg = new TGraph();

    double start = gr->GetX()[0];
    double avg = 0;
    int values = 0;

    unsigned int stepindex = 0;

    for(int i = 0; i < gr->GetN(); ++i)
    {
        if(stepindex >= steps.size() || gr->GetX()[i] < steps[stepindex])
        {
            ++values;
            avg += gr->GetY()[i];
        }
        else
        {
            avg   /= values;

            gravg->SetPoint(gravg->GetN(), start, avg);
            start  = gr->GetX()[i];
            gravg->SetPoint(gravg->GetN(), start, avg);

            //std::cout << "finished plateau at " << gravg->GetX()[i] << " with avg "
            //          << avg << " from " << values << " points." << std::endl;

            ++stepindex;
            avg    = 0;
            values = 0;
        }
    }

    avg /= values;
    gravg->SetPoint(gravg->GetN(), start, avg);
    gravg->SetPoint(gravg->GetN(), gr->GetX()[gr->GetN()-1], avg);

    return gravg;
}

void LeakageAnalysis(std::string filename, int smooth, double sigmas, int baselength, std::string outputprefix = "")
{
    TCanvas* c = nullptr;
    TGraph* gr = LoadLeakageFile(filename);

    //gr->SetMarkerStyle(8);
    gr->SetLineColor(kBlack);
    gr->SetLineWidth(2);
    c = DrawTGraph(gr, nullptr, "Time (in s)", "Leakage Current (in A)", "AL");

    if(outputprefix != "")
        c->SaveAs((outputprefix + "_leakage.pdf").c_str());

    TGraph* grs4 = Smoothen(gr, smooth);
    //grs4->SetLineColor(kAzure+7);
    //grs4->SetLineWidth(2);
    //grs4->Draw("Lsame");

    TGraph* grdiff4 = Derive(grs4);
    delete grs4; //comment when drawing the smoothed curve
    TGraph* grabsdiff4 = Absolute(grdiff4);
    delete grdiff4;

    DrawTGraph(grabsdiff4, nullptr, "Time (in s)", "Derivative (in A/s)", "AL");

    std::vector<double> steps = FindSteps(grabsdiff4, baselength, sigmas, smooth);
    std::cout << "Found " << steps.size() << " steps" << std::endl;

    TGraph* grcurrents = FindAverage(gr, steps);
    grcurrents->SetLineColor(kGreen-3);
    grcurrents->SetLineWidth(2);

    DrawTGraph(grcurrents, c, "Time (in s)", "Leakage Current (in A)", "Lsame");

    TLegend* leg = new TLegend(0.61,0.17,0.9,0.28);
    leg->AddEntry(gr, "Leakage Current", "l");
    leg->AddEntry(grcurrents, "Averaged Leakage Current", "l");
    leg->Draw();

    if(outputprefix != "")
        c->SaveAs((outputprefix + "_avgleakage.pdf").c_str());

    //save found plateaus to a file:
    if(outputprefix != "")
    {
        std::fstream f;
        f.open((outputprefix + "_currents.dat").c_str(), std::ios::out | std::ios::app);

        if(f.is_open())
        {
            f << "# Leakage Currents for \"" << filename << "\"" << std::endl;

            for(int i = 0; i < grcurrents->GetN(); i += 2)
                f << grcurrents->GetX()[i] << "\t" << grcurrents->GetY()[i] << std::endl;

            f.flush();
            f.close();
        }
    }
}

// ==============================================
//    from here old methods
// ==============================================

TH2* correlate(std::string filename, int firstlayer, int secondlayer, bool x, int xdist, int tdist, 
                bool ethernet = true)
{
    std::vector<Dataset> database[4];   //datasets for all layers separated
    
    std::fstream f;
    f.open(filename.c_str(), std::ios::in);
    
    if(!f.is_open())
    {
        std::cerr << "could not open file \"" << filename << "\"" << std::endl;
        return nullptr;
    }
    
    char text[200];
    //get rid of comment line at the beginning:
    f.getline(text, 200, '\n'); 
    
    //load datasets:
    while(!f.eof())
    {
        Dataset dat;
        int layer;
        int dummy;

        if(ethernet)        
            f >> dat.packageid >> layer >> dat.column >> dat.row >> dummy >> dummy
              >> dat.triggerts >> dat.triggerindex >> dat.ts >> dat.tot >> dummy;
        else
            f >> layer >> dat.column >> dat.row >> dummy >> dummy
              >> dat.triggerts >> dat.triggerindex >> dat.ts >> dat.tot;

        
        if(!f.eof())
            database[layer - 1].push_back(dat);
    }
    
    f.close();
    
    std::cout << "loading finished" << std::endl;
    
    static int histcnt = 0;
    std::stringstream si("");
    si << "hist_" << ++histcnt;
    
    TH2* corhist = new TH2I(si.str().c_str(), "", (x)?132:372,-0.5,(x)?131.5:371.5, 
                                (x)?132:372,-0.5, (x)?131.5:372.5);
                                
    static int tsdiffcnt = 0;
    std::stringstream sits("");
    sits << "hist_ts_" << ++tsdiffcnt;
    
    TH1* tshist = new TH1I(sits.str().c_str(), "", 10000,-5000.5,4999.5);
                                
    int length = database[firstlayer].size();
    
    int lastmatch = 0;
    int newmatch = -1;
    
    for(unsigned int i = 0; i < database[firstlayer].size(); ++i)
    {
        for(unsigned int j = lastmatch; j < database[secondlayer].size(); ++j)
        {
            if(std::abs(database[firstlayer][i].ts - database[secondlayer][j].ts) < tdist)
            {
                if(sqrt(pow(database[firstlayer][i].column - database[secondlayer][j].column, 2)
                        + pow(database[firstlayer][i].row - database[secondlayer][j].row, 2)) < xdist)
                {
                    if(x)
                        corhist->Fill(database[firstlayer][i].column, database[secondlayer][j].column);
                    else
                        corhist->Fill(database[firstlayer][i].row, database[secondlayer][j].row);
                        
                    tshist->Fill(/*std::abs*/(database[firstlayer][i].ts - database[secondlayer][j].ts));
                        
                    newmatch = j;
                }
            }
            else if(database[firstlayer][i].ts - database[secondlayer][j].ts < -3*tdist)
                break;
        }

        for(unsigned int j = lastmatch - 1; j < database[secondlayer].size(); --j)  //ok because of unsigned int (-1 <-> max_value which is larger than the number of entries)
        {
            if(std::abs(database[firstlayer][i].ts - database[secondlayer][j].ts) < tdist)
            {
                if(sqrt(pow(database[firstlayer][i].column - database[secondlayer][j].column, 2)
                        + pow(database[firstlayer][i].row - database[secondlayer][j].row, 2)) < xdist)
                {
                    if(x)
                        corhist->Fill(database[firstlayer][i].column, database[secondlayer][j].column);
                    else
                        corhist->Fill(database[firstlayer][i].row, database[secondlayer][j].row);
                        
                    tshist->Fill(/*std::abs*/(database[firstlayer][i].ts - database[secondlayer][j].ts));
                }
            }
            else if(database[firstlayer][i].ts - database[secondlayer][j].ts > 3*tdist)
                break;
        }
        
        if(newmatch > 0)
            lastmatch = newmatch;
        
        std::cout << i << "/" << length << " done\r";
    }
    std::cout << std::endl;
    
    TCanvas* c = new TCanvas();
    c->SetWindowSize(1080,700);
    c->SetRightMargin(0.15);
    c->Update();
    
    std::stringstream stitle("");
    std::stringstream xtitle("");
    std::stringstream ytitle("");
    if(x)
    {
        stitle << "X correlation of layer " << firstlayer+1 << " and layer " << secondlayer + 1;
        xtitle << "Column on layer " << firstlayer + 1;
        ytitle << "Column on layer " << secondlayer + 1;
    }
    else
    {
        stitle << "Y correlation of layer " << firstlayer+1 << " and layer " << secondlayer + 1;
        xtitle << "Row on layer " << firstlayer + 1;
        ytitle << "Row on layer " << secondlayer + 1;
    }
    
    corhist->SetTitle(stitle.str().c_str());
    corhist->GetXaxis()->SetTitle(xtitle.str().c_str());
    corhist->GetXaxis()->SetTitleSize(0.05);
    corhist->GetXaxis()->SetLabelSize(0.05);
    corhist->GetYaxis()->SetTitle(ytitle.str().c_str());
    corhist->GetYaxis()->SetTitleSize(0.05);
    corhist->GetYaxis()->SetLabelSize(0.05);
    corhist->GetYaxis()->SetTitleOffset(0.95);
    corhist->GetZaxis()->SetTitle("Counts");
    corhist->GetZaxis()->SetTitleSize(0.05);
    corhist->GetZaxis()->SetLabelSize(0.05);
    corhist->Draw("colz");
    
    
    c = new TCanvas();
    c->SetWindowSize(1000,700);
    c->Update();
    
    tshist->Draw();
    
    return corhist;
}
