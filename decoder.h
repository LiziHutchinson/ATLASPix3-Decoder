#ifndef DECODER_H
#define DECODER_H

#include <string>
#include <iostream>
#include <vector>

#include "dataset.h"

typedef unsigned char uchar;
typedef long long llong;

class decoder
{
public:
    decoder();
    bool GetUDPBugSetting();
    void SetUDPBugSetting(bool active);

    //virtual ~decoder();
    virtual void ResetDecoder() = 0;
    virtual Dataset DecodeData(char* package, int packageid) = 0;
    virtual std::vector<Dataset> DecodePackage(char* package, int length = 1024) = 0;
    int GrayDecode(int graycode, int numbits);

    int GetTSOffset(int index) const;
    bool SetTSOffset(int index, int offset);
    int GetTS2Offset(int index) const;
    bool SetTS2Offset(int index, int offset);

private:
protected:
    std::string CharToHex(char);
    bool Compare(const char* strl, const char* strr, int length);

    bool withudpbug;

    std::vector<int> tsoffset;
    std::vector<int> ts2offset;
};

#endif // DECODER_H
