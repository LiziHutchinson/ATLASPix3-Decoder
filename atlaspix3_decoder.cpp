/**********************************************************
 * Decoding Program for UDP readout files for ATLASPix    *
 * and MuPix8 readout with data concentrator readout      *
 *                                                        *
 * Version: 1.0 (17.07.19)                                *
 *          2.0 (19.07.19)                                *
 *              added data decoding                       *
 *          3.0 (19.07.19)                                *
 *          added data alignment                          *
 *          4.0 (09.12.19)                                *
 *              adapted to ATLASPix3 with DataMux on      *
 *          4.1 (14.01.20)                                *
 *              bugfix: the `lastoffset` variable was not *
 *                  set resulting in wrong decoding       *
 *          4.2 (16.01.20)                                *
 *              added support for both datamux and no     *
 *                  datamux via a parameter               *
 *          4.3 (18.03.20)                                *
 *              added triggered readout data decoding     *
 *          4.4 (19.03.20)                                *
 *              fixed data alignment issues               *
 * Author:  Rudolf Schimassek                             *
 **********************************************************/

//#define DEBUG
//#define MOREDEBUG //to stop the program after each dataset

#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <vector>

#include "fileoperations.h"
#include "atlaspix3.h"

int main(int argc, char** argv)
{
    std::cout << "┌────────────────────────────────────────────────────┐" << std::endl
              << "│ ATLASPix3 UDP package file decoder V4.7 (13.10.21) │" << std::endl
              << "└────────────────────────────────────────────────────┘" << std::endl;

//    if(argc < 3 || argc > 11)
//    {
//        std::cout << "Not all parameters passed!" << std::endl
//                  << " call \"" << argv[0] << " [`mode`] `input file` `outputfile`\""
//                  << std::endl << std::endl
//                  << "Available modes are:\n"
//                  << "    --triggered \tfor data acquired with triggered readout\n"
//                  << "    --nomux \tfor data acquired without datamux compression\n"
//                  << "    --datamux \tfor data acquired with datamux compression\n"
//                  << "      \t\t    default behaviour is `--datamux`\n"
//                  << "    --splitlayers \tto write hits into one file per layer\n"
//                  << "      \t\t    a suffix \"_l`x`\" is added to identify the layer\n"
//                  << "    --tsformat1 \tfor setting triggered readout hit format to\n"
//                  << "      \t\t    format 1 (TS2)\n"
//                  << "    --tsformat2 \tfor setting triggered readout hit format to\n"
//                  << "      \t\t    format 2 (TS1)\n"
//                  << "    --noudpbug \tassume that the UDP bug causing double sent\n"
//                  << "      \t\t    bytes is fixed" << std::endl;
//        return -1;
//    }
    if(argc != 2)
    {
        std::cout << "Not all call parameters passed:\n"
                  << " call \"" << argv[0] << " [config file path]\"" << std::endl;
        return -1;
    }
		
    for(int i = 0; i < argc; ++i)
        std::cout << "arg " << i << ": \"" << argv[i] << "\"" << std::endl;

    StringPairs config = LoadParameterFile(argv[1], "Config");
    StringPairs offsets = LoadParameterFile(argv[1], "Offset");

    int romode = 1;  //0 - nomux, 1 - datamux, 2 - triggered
    if(FindKey(config,"romode","").compare("nomux") == 0)
        romode = 0;
    else if(FindKey(config, "romode", "").compare("datamux") == 0)
        romode = 1;
    else if(FindKey(config, "romode", "").compare("triggered") == 0)
        romode = 2;

    bool splitlayers = FindKeyBool(config, "splitlayers", false); // false;
    int triggeredtsformat = FindKeyInt(config, "tsformat", 0);
    bool udpbug = FindKeyBool(config, "udpbug", true);

    bool cleanup =  FindKeyBool(config, "cleanup", false);
       if (cleanup) {
         std::cout << "Will attempt to correct for packageid and TS glitches." << std::endl;
       }


    std::string inputfile = FindKey(config, "input", "");
    std::string outputfile = FindKey(config, "output", "");

    if(inputfile == "" || outputfile == "")
    {
        std::cerr << "input or output file missing. Aborting" << std::endl;
        return -2;
    }

    int tsoffsets[4] = {0};
    tsoffsets[0] = FindKeyInt(offsets, "l1ts1", 0);
    tsoffsets[1] = FindKeyInt(offsets, "l2ts1", 0);
    tsoffsets[2] = FindKeyInt(offsets, "l3ts1", 0);
    tsoffsets[3] = FindKeyInt(offsets, "l4ts1", 0);
    int ts2offsets[4] = {0};
    ts2offsets[0] = FindKeyInt(offsets, "l1ts2", 0);
    ts2offsets[1] = FindKeyInt(offsets, "l2ts2", 0);
    ts2offsets[2] = FindKeyInt(offsets, "l3ts2", 0);
    ts2offsets[3] = FindKeyInt(offsets, "l4ts2", 0);

    //int inputindexoffset = 0;
    //int triggeredtsformat = 0;
    //bool udpbug = true;
//    for(int i = 1; i < argc; ++i)
//    {
//        if(std::string(argv[i]).compare("--nomux") == 0)
//        {
//            romode = 0;
//            ++inputindexoffset;
//        }
//        else if(std::string(argv[i]).compare("--triggered") == 0)
//        {
//            romode = 2;
//            ++inputindexoffset;
//        }
//        else if(std::string(argv[i]).compare("--splitlayers") == 0)
//        {
//            splitlayers = true;
//            ++inputindexoffset;
//        }
//        else if(std::string(argv[i]).compare("--tsformat1") == 0)
//        {
//            triggeredtsformat = 1;
//            ++inputindexoffset;
//        }
//        else if(std::string(argv[i]).compare("--tsformat2") == 0)
//        {
//            triggeredtsformat = 2;
//            ++inputindexoffset;
//        }
//        else if(std::string(argv[i]).compare("--noudpbug") == 0)
//        {
//            udpbug = false;
//            ++inputindexoffset;
//        }
//        else if(std::string(argv[i]).find("--") == 0)
//            ++inputindexoffset;
//    }

//    std::cout << "mode parameter passed: " << inputindexoffset << std::endl;

    std::fstream fin;
    std::fstream fout[5];

    fin.open(inputfile.c_str() /*argv[1 + inputindexoffset]*/, std::ios::in | std::ios::binary);
	if(!fin.is_open())
	{
        std::cout << "Could not open input file \"" << inputfile /*argv[1+inputindexoffset]*/
                  << "\"" << std::endl;
		return -2;
	}
	
    //open output file(s):
    if(splitlayers)
    {
        std::string filename = outputfile; //std::string(argv[2 + inputindexoffset]);
        int endingpos = filename.rfind('.');
        if(endingpos == int(std::string::npos))
            endingpos = -1;

        for(int i = 1; i <= 4; ++i)
        {
            std::string file = "";
            if(endingpos > 0)
                file = filename.substr(0,endingpos) + "_l" + char(i+48)
                                    + filename.substr(endingpos);
            else
                file = filename + "_" + char(i+48);
            fout[i].open(file.c_str(), std::ios::out | std::ios::app);
            if(!fout[i].is_open())
            {
                for(int j = 1; j < i; ++j)
                    fout[j].close();
                fin.close();
                std::cout << "Could not open output file \"" << file << "\"" << std::endl;
                return -3;
            }
        }
    }
    else
    {
        fout[0].open(outputfile.c_str() /*argv[2 + inputindexoffset]*/,
                     std::ios::out | std::ios::app);
        if(!fout[0].is_open())
        {
            fin.close();
            std::cout << "Could not open output file \"" << outputfile
                         /*argv[2 + inputindexoffset]*/ << "\"" << std::endl;
            return -3;
        }
    }
	
	//buffer to reduce number of HDD accesses:
    std::stringstream sout[5]; //0 - single layer setup, 1-4 - telescope layers
    for(int i = ((splitlayers)?1:0); i < ((splitlayers)?5:1); ++i)
    {
        sout[i] << std::stringstream("");
        sout[i] << Dataset::GetHeader(romode == 2) << std::endl;
    }

    char package[1280]; //[1024];
    //std::string text;
	
	int packageid = -1;
    const int outputstep = 100;
    int idcnt = 0;
    int datasetcount[4] = {0};
		
	//read the first package:
    fin.read(package, (udpbug)?1280:1024);
	        
#ifdef DEBUG
    int positioninfile = 0;
#endif

    atlaspix3_decoder           dec;
    atlaspix3_decoder_nomux     decnomux;
    atlaspix3_decoder_triggered dectrig;

    std::vector<Dataset> hitcollection;

    dec.SetUDPBugSetting(udpbug);
    decnomux.SetUDPBugSetting(udpbug);
    dectrig.SetUDPBugSetting(udpbug);

    dec.ResetDecoder();
    decnomux.ResetDecoder();
    dectrig.ResetDecoder();

    if(triggeredtsformat == 1)
        dectrig.SetTSFormat(true);
    else if(triggeredtsformat == 2)
        dectrig.SetTSFormat(false);

    for(int i = 0; i < 4; ++i)
    {
        dec.SetTSOffset(i, tsoffsets[i]);
        decnomux.SetTSOffset(i, tsoffsets[i]);
        dectrig.SetTSOffset(i, tsoffsets[i]);

        dec.SetTS2Offset(i, ts2offsets[i]);
        decnomux.SetTS2Offset(i, ts2offsets[i]);
        dectrig.SetTS2Offset(i, ts2offsets[i]);
    }

    int previous_packageid(-2), npackage_fixed(0), nts_fixed(0), layer0(0);
        long long previous_ts(-2),nl0(0),nts2(0);
        bool first_hit(true);


	while(!fin.eof())
	{
        packageid = (int(package[6]) & 255) * 256 + (int(package[7]) & 255);

#ifndef DEBUG
        if(++idcnt >= outputstep)
#else
        (void) outputstep;
        (void) idcnt;
#endif
        {
            idcnt = 0;
            std::cout << "\rDecoding package " << packageid << "...     "; // << std::endl;
            std::cout.flush();
        }

#ifdef MOREDEBUG
        std::string userinput = "";
        std::cin >> userinput;
        if(userinput.compare("quit") == 0)
            break;
#endif

        std::vector<Dataset> newhits;

        switch(romode)
        {
        case(0):
            newhits = decnomux.DecodePackage(package, (udpbug)?1280:1024);
            break;
        case(1):
            newhits = dec.DecodePackage(package, (udpbug)?1280:1024);
            break;
        case(2):
            newhits = dectrig.DecodePackage(package, (udpbug)?1280:1024);
            break;
        default:
            newhits = std::vector<Dataset>();
            break;
        }

        hitcollection.insert(hitcollection.end(), newhits.begin(), newhits.end());

        if(hitcollection.size() > 2000)
        {

            // clean up
                  if (cleanup) {
                    double mytot(0);

                   // int num_hit=0;
                    for (auto& it : hitcollection) {
                      mytot = it.CalculateToT(0,1);
                      if(it.layer==0){
                          it.Print();
                          nl0++;
                      }
                      if(it.ts2>18e+6){
                          it.Print();
                          nts2++;
                      }

                     // if (mytot >= 255) {
                    // remove this hit
                   // std::cout << it.layer << " " << mytot << std::endl;
                    //hitcollection.erase(it); //ill-defined?
                    //continue;
                     // }
                      if (first_hit) { // only works if first packageid and TS are sensible values
                    previous_packageid = it.packageid;
                    previous_ts = it.ts;
                      }
                      // try to fix packageid's == -1
                      if (it.packageid == -1) {
                    it.packageid = previous_packageid;
                    npackage_fixed++;
                      }
                      previous_packageid = it.packageid;

                      // try to fix unfeasible jumps in TS
                      bool high_ratio = (!first_hit) && (it.ts > (previous_ts * 2));
                      bool low_ratio  = (!first_hit) && (previous_ts > (it.ts * 2));
                      if (high_ratio || low_ratio) {
                         std::cout << it.ts << " " << previous_ts << std::endl;
                    it.ts = previous_ts;
                   // hitcollection.erase(hitcollection.begin()+num_hit);
                    nts_fixed++;
                      }
                      previous_ts = it.ts;

                      first_hit = false;
                      if (it.layer == 0) {layer0++;}
                    }
                    //num_hit++;
                  }



            if(splitlayers)
            {
                for(auto& it : hitcollection)
                {
                    sout[it.layer] << it.ToString() << std::endl;
                    ++datasetcount[it.layer];
                }
            }
            else
            {
                for(auto& it : hitcollection)
                {
                    sout[0] << it.ToString() << std::endl;
                    ++datasetcount[0];
                }
            }

            hitcollection.clear();

            //write data to HDD in bunches of 2000 datasets:
            for(int i = ((splitlayers)?1:0); i < ((splitlayers)?5:1); ++i)
            {
                if(datasetcount[i] > 2000)
                {
                    datasetcount[i] = 0;
                    fout[i] << sout[i].str();
                    fout[i].flush();
                    sout[i].str("");
                    //static int id[4] = {0};
                    //std::cout << i << ": " << (id[i])++ << std::endl;
                }
            }

        }

       // dec.ResetDecoder();
       // decnomux.ResetDecoder();
       // dectrig.ResetDecoder();

        fin.read(package, (udpbug)?1280:1024); //1024);

#ifdef DEBUG
        positioninfile += (udpbug)?1280:1024; //1024;
#endif
    }

    //add a newline after the carriage returns in the loop
    std::cout << std::endl;

    //write the remaining data to the output file(s) and close the files:
    if(splitlayers)
    {
        for(auto& it : hitcollection)
        {
            sout[it.layer] << it.ToString() << std::endl;
            ++datasetcount[it.layer];
        }
    }
    else
    {
        for(auto& it : hitcollection)
        {
            sout[0] << it.ToString() << std::endl;
            ++datasetcount[0];
        }
    }
    for(int i = ((splitlayers)?1:0); i < ((splitlayers)?5:1); ++i)
    {
        fout[i] << sout[i].str();
        fout[i].flush();
        fout[i].close();
    }
    fin.close();

    std::cout << "TS fixed " << nts_fixed << std::endl;
    std::cout << "TS2 problem " << nts2 << std::endl;
    std::cout << "Layer 0 " << nl0 << std::endl;


#ifdef DEBUG
    std::cout << "finished decoding" << std::endl;
#endif

    if(romode == 2)
        std::cout << "Hit Format Errors: " << dectrig.GetFormatErrors() << std::endl;

    return 0;
}

