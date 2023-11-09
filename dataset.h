#ifndef __DATASET
#define __DATASET

#include <sstream>
#include <string>
#include <algorithm>
#include <iostream>

class Dataset
{
    public:
        Dataset() : layer(0), column(-1), row(-1), shortts(-1), shortts1(-1), shortts2(-1), ts(-1),
            ts2(-1), triggerindex(-1), triggerts(-1), fifowasfull(false), packageid(-1),
            triggertag(-1), fifofull(false), complete(0) {}
        
    Dataset(std::string address) : layer(0), shortts(-1), shortts1(-1), shortts2(-1), ts(-1),
    ts2(-1), triggerindex(-1), triggerts(-1), fifowasfull(false), packageid(-1),
    triggertag(-1), fifofull(false), complete(0)
    {
        //remove leading spaces:
        while(address != "" && address[0] == ' ')
            address = address.substr(1);
        //remove trailing spaces:
        while(address != "" && address[address.length()-1] == ' ')
            address = address.substr(0, address.length()-1);
        //require the brackets for identification:
        if(address[0] != '(' || address.find(')') == std::string::npos)
        {
            column = -1;
            row    = -1;
            return;
        }
        else
        {
            int start = address.find('(');
            int ende  = address.find('|');
            std::stringstream s("");
            s << address.substr(start + 1, ende - start - 1);
            //std::cout << "Column: \"" << text.substr(start + 1, ende - start - 1) << "\"\n";
            s >> column;
            start = ende;
            ende  = address.find(')');
            std::stringstream s2("");
            s2 << address.substr(start + 1, ende - start - 1);
            //std::cout << "Row: \"" << text.substr(start + 1, ende - start - 1) << "\"" << std::endl;
            s2 >> row;
        }
    }

    
        short     layer;
        short     column;
        short     row;
        short     shortts;
        short     shortts1; //TS individual from hit format without TS2
        short     shortts2;
        long long ts;
        long long ts2;
        long long triggerindex;
        long long triggerts;
        short     fifowasfull;  //FPGA readout FIFO (lost data on FPGA)
        int       packageid;
        short     triggertag;   //trigger tag from ATLASPix3
        short     fifofull;     //ATLASPix3 (trigger?) FIFO full
        
        short complete;
        
        static std::string GetHeader(bool triggeredRO = false)
        {
            std::string header = "# PackageID; Layer; Column; Row; TS; TS1; TS2;"
                                    + std::string(" TriggerTS; TriggerID; ext. TS; ext. TS2;")
                                    + std::string(" FIFO overflow");
            if(triggeredRO)
                header = header + "; triggertag; TriggerTable overflow";

            return header;
        }
        
        std::string ToString() const
        {
            std::stringstream s("");
            s << packageid << "\t" << layer << "\t" << column << "\t" << row << "\t" << shortts
              << "\t" << shortts1 << "\t" << shortts2 << "\t" << triggerts << "\t" << triggerindex
              << "\t" << ts << "\t" << ts2 << "\t" << ((fifowasfull)?"1":"0");

            if(triggertag != -1 || fifofull)
                s << "\t" << triggertag << "\t" << ((fifofull)?"1":"0");
            
            return s.str();
        }
        
        bool is_complete() const
        {
            return complete == 7 || complete == 0b111111100000;
        }

        bool is_valid() const
        {
            return is_complete();
        }

        bool operator<(const Dataset& rhs) const {
            return ts < rhs.ts
                    || (ts == rhs.ts && (column < rhs.column
                                         || (column == rhs.column && row < rhs.row)));
        }

        int CalculateToT(int ts1ckdiv, int ts2ckdiv) const {
              const int ts1_size = 1024;
              const int ts2_size = 128;

              int overflow = ts2_size * (ts2ckdiv + 1);

              if(overflow > ts1_size * (ts1ckdiv + 1)) {overflow = ts1_size * (ts1ckdiv + 1); }

              int ts1_corrected = (shortts * (ts1ckdiv + 1)) % (ts2_size * (ts2ckdiv + 1));
              int ts2_corrected = (shortts2 * (ts2ckdiv + 1)) % (ts1_size * (ts1ckdiv + 1));

              int tot = ts2_corrected - ts1_corrected;

              if (tot < 0) {tot += overflow;}

              return tot;
            }
        void Print(){

            std::cout << layer << " " << column << " " << row << " " << ts << " " << ts2 << std::endl;
        }
};

namespace DatasetFunctions {

    std::string ToLowerCase(std::string text);
    std::string StripSpaces(std::string text);

    Dataset FindOrder(std::string line);

    Dataset Construct(std::string line, Dataset order);

}


#endif //__DATASET
