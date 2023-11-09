#ifndef ATLASPIX3_DECODER_H
#define ATLASPIX3_DECODER_H

#include <sstream>
#include <stdio.h>
#include <deque>

#include "decoder.h"

//#define DEBUG

class atlaspix3_decoder : public decoder
{
public:
    atlaspix3_decoder();
    void ResetDecoder();
    Dataset DecodeData(char* package, int packageid);
    std::vector<Dataset> DecodePackage(char* package, int length = 1024);
private:
    int AlignData(char* package, int datalength, int start, int stop = -1000, bool ending = false);

    std::vector<Dataset> datasets;
};

class atlaspix3_decoder_nomux : public decoder
{
public:
    atlaspix3_decoder_nomux();
    void ResetDecoder();
    Dataset DecodeData(char* package, int packageid);
    std::vector<Dataset> DecodePackage(char* package, int length = 1024);
private:
    int AlignData(char* package, int datalength, int start, int stop = -1000, bool ending = false);
    std::vector<Dataset> datasets;
};

class atlaspix3_decoder_triggered : public decoder
{
public:
    atlaspix3_decoder_triggered();
    void ResetDecoder();
    Dataset DecodeData(char* package, int packageid);
    std::vector<Dataset> DecodePackage(char* package, int length = 1024);
    void SetTSFormat(bool format1);
    int GetFormatErrors();
private:
    bool ValidStartByte(uint character, int wrongtsformat = 17);
    int AlignData(char* package, int datalength, int start, int stop = -1000, bool ending = false);

    std::vector<Dataset> datasets;
    int lastoffset;

    std::vector<std::deque<Dataset> > incompletehits;
    std::vector<std::deque<Dataset> > completehits;

    bool lastwasdoublebyte;

    int tsformat1;
    int tsformat2;
    int tsformaterror;
    const int formatdecision = 50;
    int notexpectedtsformat;
};

#endif // ATLASPIX3_DECODER_H
