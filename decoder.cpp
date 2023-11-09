#include "decoder.h"

decoder::decoder() : withudpbug(true)
{

}

bool decoder::GetUDPBugSetting()
{
    return withudpbug;
}

void decoder::SetUDPBugSetting(bool active)
{
    withudpbug = active;
}

int decoder::GrayDecode(int graycode, int numbits)
{
    int normal;
    normal = 0;
    normal = graycode & (1 << (numbits - 1));
    for(int i = numbits - 2; i >= 0; --i)
        normal |= (graycode ^ (normal >> 1)) & (1 << i);

    return normal;
}

int decoder::GetTSOffset(int index) const
{
    if(index < 0 || index >= int(tsoffset.size()))
        return 0;
    else
        return tsoffset[index];
}

bool decoder::SetTSOffset(int index, int offset)
{
    if(index < 0)
        return false;
    else if(index >= int(tsoffset.size()))
        tsoffset.resize(index + 1, 0);

    tsoffset[index] = offset;
    return true;
}

int decoder::GetTS2Offset(int index) const
{
    if(index < 0 || index >= int(ts2offset.size()))
        return 0;
    else
        return ts2offset[index];
}

bool decoder::SetTS2Offset(int index, int offset)
{
    if(index < 0)
        return false;
    else if(index >= int(ts2offset.size()))
        ts2offset.resize(index + 1, 0);

    ts2offset[index] = offset;
    return true;
}

std::string decoder::CharToHex(char c)
{
    std::string result = "";
    switch((c >> 4) & 0x0f)
    {
        case(10): result = "a"; break;
        case(11): result = "b"; break;
        case(12): result = "c"; break;
        case(13): result = "d"; break;
        case(14): result = "e"; break;
        case(15): result = "f"; break;
        default:
            result = ((c >> 4) & 0x0f) + '0';
            break;
    }

    switch(c & 0x0f)
    {
        case(10): result += "a"; break;
        case(11): result += "b"; break;
        case(12): result += "c"; break;
        case(13): result += "d"; break;
        case(14): result += "e"; break;
        case(15): result += "f"; break;
        default:
            result += (c & 0x0f) + '0';
    }

    return result;
}

bool decoder::Compare(const char* strl, const char* strr, int length)
{
    for(int i = 0; i < length; ++i)
        if(strl[i] != strr[i])
            return false;

    return true;
}
