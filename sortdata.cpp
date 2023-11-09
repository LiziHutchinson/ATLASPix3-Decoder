#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <algorithm>

#include "dataset.cpp"
#include "object_drawing.cpp"

std::list<Dataset> LoadFile(std::string filename)
{
    if(filename == "")
        return std::list<Dataset>();

    std::fstream f;
    f.open(filename.c_str(), std::ios::in);

    if(!f.is_open())
    {
        std::cerr << "Could not open \"" << filename << "\"" << std::endl;
        return std::list<Dataset>();
    }

    std::list<Dataset> data;

    std::string line;
    std::getline(f, line);
    Dataset fieldorder;

    while(f.good())
    {
        if(line[0] == '#')
        {
            fieldorder = DatasetFunctions::FindOrder(line);

            std::cout << Dataset::GetHeader(false) << "\n" << fieldorder.ToString() << std::endl;
        }
        else
        {
            Dataset dat = DatasetFunctions::Construct(line, fieldorder);
            if(dat.is_valid())
                data.push_back(dat);
        }

        std::getline(f, line);
    }

    f.close();

    return data;
}

bool SaveToFile(std::vector<Dataset>& data, std::string filename)
{
    if(filename == "")
        return false;

    std::fstream f;
    f.open(filename.c_str(), std::ios::out);

    if(!f.is_open())
        return false;

    f << Dataset::GetHeader(false) << std::endl;

    for(const auto& it : data)
        f << it.ToString() << "\n";

    f << std::flush;

    f.close();
    return true;
}

int RemoveOutliers(std::list<Dataset>& data, long long stepsize = 150e6)
{
    long long lasttime = -1;

    auto it = data.begin();
    if(it != data.end())
        lasttime = it->ts;

    unsigned int cnt = data.size();
    int index = 0;

    while(it != data.end())
    {
        if(std::abs(it->ts - lasttime) > stepsize)
            it = data.erase(it);
        else
        {
            lasttime = it->ts;
            ++it;
        }

        if(index++ % 100 == 0)
            std::cout << "\r processed " << index << "/" << data.size() << std::flush;
    }

    std::cout << std::endl;

    return cnt - data.size();
}

int RemoveSingleOutliers(std::list<Dataset>& data, int stepsize = 150e6)
{
    auto it = data.begin();
    auto itm1 = it;
    ++it;
    auto itp1 = it;
    ++itp1;

    int cnt = int(data.size());

    while(itp1 != data.end())
    {
        if(std::abs(itm1->ts - it->ts) > stepsize
                && std::abs(itm1->ts - itp1->ts) < stepsize / 2)
        {
            it = data.erase(it);
            itm1 = it;
            --itm1;
            itp1 = it;
            ++itp1;
        }
        else
        {
            ++itm1;
            ++it;
            ++itp1;
        }

    }

    return cnt - int(data.size());
}

void SortFile(std::string filename, std::string outfile, long long stepsize = 150e6,
              long long singlestepsize = 150e6, int ignorefirst = 0)
{
    if(filename == "")
    {
        std::cerr << "no filename passed" << std::endl;
        return;
    }

    std::list<Dataset> data = LoadFile(filename);
    std::cout << "Loaded " << data.size() << " hits" << std::endl;

    TGraph* grloaded = new TGraph();
    for(const auto& it : data)
        grloaded->SetPoint(grloaded->GetN(), grloaded->GetN(), it.ts);
    DrawTGraph(grloaded, nullptr, "loaded TSs", "Hit Index", "ext. TS");

    for(int i = 0; i < ignorefirst && data.size() > 0; ++i)
        data.pop_front();
    std::cout << "removed the first " << ignorefirst << " elements" << std::endl;

    int removed = RemoveSingleOutliers(data, singlestepsize);
    std::cout << "  removed " << removed << " outliers from single hit wrong TS" << std::endl;

    removed += RemoveOutliers(data, stepsize);
    std::cout << "  removed " << removed << " outliers in total" << std::endl;

    removed += RemoveSingleOutliers(data, singlestepsize);
    std::cout << "  removed " << removed << " outliers in total (after second single hit clean)"
              << std::endl;

    std::vector<Dataset> sorteddata;
    sorteddata.reserve(data.size());

    TGraph* grunsorted = new TGraph();
    for(const auto& it : data)
    {
        grunsorted->SetPoint(grunsorted->GetN(), grunsorted->GetN(), it.ts);
        sorteddata.push_back(it);
    }
    DrawTGraph(grunsorted, nullptr, "unsorted TSs", "Hit Index", "ext. TS");

    TGraph* grdiff = new TGraph();
    for(int i = 0; i < grunsorted->GetN(); ++i)
        if(i != 0)
            grdiff->SetPoint(grdiff->GetN(), grdiff->GetN(),
                             grunsorted->GetY()[i] - grunsorted->GetY()[i-1]);
    DrawTGraph(grdiff, nullptr, "TS differences", "Hit Index", "TS difference");

    std::sort(sorteddata.begin(), sorteddata.end());
    std::cout << "sorted data" << std::endl;

    TGraph* grsorted = new TGraph();
    for(const auto& it : sorteddata)
        grsorted->SetPoint(grsorted->GetN(), grsorted->GetN(), it.ts);
    DrawTGraph(grsorted, nullptr, "sorted data", "Hit Index", "ext. TS");

    if(outfile != "")
    {
        if(!SaveToFile(sorteddata, outfile))
            std::cerr << "Error saving data to \"" << outfile << "\"" << std::endl;
        else
            std::cout << "Wrote data to \"" << outfile << "\"" << std::endl;

        std::fstream f;
        f.open((outfile + ".call").c_str(), std::ios::out | std::ios::app);
        if(f.is_open())
        {
            f << "# function call of \"SortFile()\" from \"sortdata.cpp\"\n"
              << "    filename:       \"" << filename << "\"\n"
              << "    outfile:        \"" << outfile << "\"\n"
              << "    stepsize:        " << stepsize << "\n"
              << "    singlestepsize:  " << singlestepsize << "\n"
              << "    ignorefirst:     " << ignorefirst << std::endl;

            f.flush();
            f.close();
        }
    }
}
