#include "atlaspix3.h"

atlaspix3_decoder::atlaspix3_decoder()
{
    if(datasets.size() == 0)
    {
        for(int i = 0; i < 4; ++i)
        {
            datasets.push_back(Dataset());
            datasets[i].layer = i+1;
        }
    }
}

void atlaspix3_decoder::ResetDecoder()
{
    for(int i = 0; i < int(datasets.size()); ++i)
    {
        datasets[i] = Dataset();
        datasets[i].layer = i+1;
    }
}

Dataset atlaspix3_decoder::DecodeData(char *package, int packageid)
{


    Dataset finishedhit;

    int layer = (package[0] >> 4) & 15;
    int type = package[0] & 15;

    int index = ((layer > 0)?layer-1:0); //layer is 0 for single chip setup



#ifdef DEBUG

    std::cout << int(package[0]) << " -> " << "layer: " << layer << std::endl;

#endif

    if(layer > 0 && layer < 5 && type>=1 && type<=3) //0 - single layer, 1-4 telescope
    {
        switch(package[0] & 15)
        {
            case(1):
                if(datasets[index].is_complete()){
                    if(datasets[index].row < 372 && datasets[index].column < 132 && datasets[index].row > 0 && datasets[index].column >0 && datasets[index].ts2<18e+6)
                        finishedhit = datasets[index];
                    datasets[index] = Dataset();  //clear the spot
                    datasets[index].layer = layer;
                }
                datasets[index].triggerts = 0;
                for(int i = 1; i < 6; ++i)
                    datasets[index].triggerts = datasets[index].triggerts * 256
                                                    + (int(package[i]) & 255);
                datasets[index].triggerindex = 0;
                for(int i = 6; i < 8; ++i)
                    datasets[index].triggerindex = datasets[index].triggerindex * 256
                                                    + (int(package[i]) & 255);
                datasets[index].complete |= 1;
            break;
            case(2):
                datasets[index].ts2 = 0;
                for(int i = 1; i < 6; ++i){
                    datasets[index].ts2 = datasets[index].ts2 * 256 + (int(package[i]) & 255);

                }
                datasets[index].shortts2 = datasets[index].ts2 % 128;
                datasets[index].column = 131 - (int(package[6]) & 255);
                datasets[index].triggerindex = (int(package[7]) & 255) * 65536
                                                + datasets[index].triggerindex;
                datasets[index].complete |= 2;
            break;
            case(3):
                datasets[index].fifowasfull = (package[1] & 128) != 0;
                datasets[index].row = (package[1] & 1) * 256 + (int(package[2]) & 255);
                if(datasets[index].row < 186)
                    datasets[index].row = 185 - datasets[index].row;
                datasets[index].ts = 0;
                for(int i = 3; i < 8; ++i)
                    datasets[index].ts = datasets[index].ts * 256 + (int(package[i]) & 255);
                datasets[index].shortts = datasets[index].ts % 1024;
                datasets[index].complete |= 4;
                datasets[index].packageid = packageid;
            break;
            default:
                std::cerr << "This should not happen. Is DataMux turned off?" << std::endl;
                          //<< "   package (ID: " << CharToHex(packageid/256) << " "
                          //<< CharToHex(packageid % 256) << "): ";
                //debug output for testing:
                for(int i = 0; i < 8; ++i)
                    std::cout << CharToHex(package[i]) << " ";
                std::cout << std::endl;

                break;
        }

        if(datasets[index].complete==5){
            datasets[index] = Dataset();
            datasets[index].layer = index+1;
        }
        if(datasets[index].is_complete())
        {
            //directly reject hits with invalid address:
            if(datasets[index].row < 372 && datasets[index].column < 132 && datasets[index].row > 0 && datasets[index].column >0 && datasets[index].ts2<18e+6)
                finishedhit = datasets[index];
            datasets[index] = Dataset();  //clear the spot
            datasets[index].layer = layer;
            //if(datasetcount != nullptr)
            //    ++(datasetcount[(splitlayers)?index:0]);

           if(index < int(tsoffset.size()))
            {
                finishedhit.ts += tsoffset[index];
                finishedhit.shortts = (finishedhit.shortts + tsoffset[index]) % 1024;
                while(finishedhit.shortts < 0)
                    finishedhit.shortts += 1024;
                finishedhit.ts2 += ts2offset[index];
                finishedhit.shortts2 = (finishedhit.shortts2 + ts2offset[index]) % 128;
                while(finishedhit.shortts2 < 0)
                    finishedhit.shortts2 += 128;
            }
        }
    }else{

        for(int j=0; j<int(datasets.size()); j++){

            switch (datasets[j].complete) {

                    case(0):
                        datasets[j].triggerts = 0;
                        for(int i = 1; i < 6; ++i)
                            datasets[j].triggerts = datasets[j].triggerts * 256
                                                            + (int(package[i]) & 255);
                        datasets[j].triggerindex = 0;
                        for(int i = 6; i < 8; ++i)
                            datasets[j].triggerindex = datasets[j].triggerindex * 256
                                                            + (int(package[i]) & 255);
                        datasets[j].complete |= 1;
                    break;
                    case(1):
                        datasets[j].ts2 = 0;
                        for(int i = 1; i < 6; ++i){
                            datasets[j].ts2 = datasets[j].ts2 * 256 + (int(package[i]) & 255);

                        }
                        datasets[j].shortts2 = datasets[j].ts2 % 128;
                        datasets[j].column = 131 - (int(package[6]) & 255);
                        datasets[j].triggerindex = (int(package[7]) & 255) * 65536
                                                        + datasets[j].triggerindex;
                        datasets[j].complete |= 2;
                    break;
                    case(3):
                        datasets[j].fifowasfull = (package[1] & 128) != 0;
                        datasets[j].row = (package[1] & 1) * 256 + (int(package[2]) & 255);
                        if(datasets[j].row < 186)
                            datasets[j].row = 185 - datasets[j].row;
                        datasets[j].ts = 0;
                        for(int i = 3; i < 8; ++i)
                            datasets[j].ts = datasets[j].ts * 256 + (int(package[i]) & 255);
                        datasets[j].shortts = datasets[j].ts % 1024;
                        datasets[j].complete |= 4;
                        datasets[j].packageid = packageid;
                    break;
                    default:
                        break;
                    }

        }


    }

#ifdef DEBUG
    if(finishedhit.ts2>18e+6){
        for(int i = 0; i < 8; ++i)
                            std::cout << CharToHex(package[i]) << " ";
                        std::cout << std::endl;

    }
#endif
    return finishedhit;
}

std::vector<Dataset> atlaspix3_decoder::DecodePackage(char *package, int length)
{

    int hl1=0;
    int hl2=0;
    int hl3=0;
    int hl4=0;
    std::vector<Dataset> hitcollection;

    const char header[] = {char(0x80), char(0x81), char(0x82), char(0x83), char(0x84), char(0x85)};
    const char empty[]  = {0, 0, 0, 0, 0, 0, 0, 0};

    int position = 0;

    char content[16] = {0};

    //data alignment variables:
    int lastoffset = 0;

    int packageid = -1;
    //int datasetcount[4] = {0};

    int failcount = 0;
    const int maxfails = 400;

    //read the first package:

    bool prevwasheader=false;

    while(position < length)
    {
#ifdef DEBUG
        //only output if not empty data (all '0'):
        if(!Compare(package, empty, 8))
       {

            std::cout << "Package (" << position << "): ";
            for(int i = 0; i < 8; ++i)
                std::cout << CharToHex(package[i]) << " ";
            std::cout << std::endl;
        }

#endif

        //get package ID:
        if(Compare(package, header, 6))
        {
            packageid = int((unsigned char)(package[6])) * 256 + int((unsigned char)(package[7]));
            package  += 8;
            position += 8;
            failcount = 1; //to distinguish from data
#ifdef DEBUG

            std::cout << "  -> ID"  << packageid << std::endl;

#endif
            prevwasheader=true;
            continue;
        }
        //decode the data from the data concentrator if not empty data:
        else if(!Compare(package, empty, 8) || (prevwasheader && Compare(package, empty, 8)))
        {
            if(prevwasheader){
                int a = (package[1] >> 4) & 15;
                int b = package[1] & 15;
                if((a==1 || a==2 || a==3 || a==4) && (b==1 || b==2 || b==3)){
                        package[0]=char(0x00);


            }
            }

            prevwasheader=false;

            Dataset newhit = DecodeData(package, packageid);
            if(newhit.is_complete()){
                hitcollection.push_back(newhit);
                if(newhit.layer==1) hl1++;
                if(newhit.layer==2) hl2++;
                if(newhit.layer==3) hl3++;
                if(newhit.layer==4) hl4++;
            }
   /*         for(int i = 0; i < 8; ++i)
                content[i] = content[i+8];
            for(int i = 8; i < 16; ++i)
                content[i] = package[i-8];
#ifdef DEBUG
            for(int i = 0; i < 16; ++i)
                std::cout << CharToHex(content[i]) << " ";
            std::cout << std::endl;
#endif

            //data alignment:
            int offset = AlignData(content, 16, lastoffset);
#ifdef DEBUG

            std::cout << "offset: " << offset << std::endl;
#endif
            if(offset >= 0)
            {
                if(offset < lastoffset)
                {
                    int newoffset = AlignData(content, 16, lastoffset);
                    if(newoffset == lastoffset)
                        offset = lastoffset;
                }

                Dataset newhit = DecodeData(&(content[offset]), packageid);
                if(newhit.is_complete())
                    hitcollection.push_back(newhit);
                lastoffset = offset;
            }
            else
            {
                int endalign = AlignData(content, 16, lastoffset, -1000, true);

                if(endalign >= 0)
                {
                    Dataset newhit = DecodeData(&(content[endalign]), packageid);
                    if(newhit.is_complete())
                        hitcollection.push_back(newhit);
                }
#ifdef DEBUG
                else //if(offset < 0)
                    std::cout << "Error: unalignable dataset found" << std::endl;

                    std::cout << "  -> Data" << std::endl;
#endif
            }
            failcount = 0;*/
        }
        //eternal loop prevention:
   /*     else
        {
            failcount++;

#ifdef DEBUG
        //std::cout << "  -> fail" << std::endl;
#endif

            if(failcount > maxfails)
            {
                std::cout << "too many fails (" << maxfails << ") in a row, aborting ..." << std::endl;
                break;
            }
        }*/

        if(Compare(package, empty, 8))
                prevwasheader=false;


        package += 8;
        position += 8;
    }

    if(hl1==0){
        datasets[0]=Dataset();
        datasets[0].layer=1;
    }
    if(hl2==0){
        datasets[1]=Dataset();
        datasets[1].layer=2;
    }
    if(hl3==0){
        datasets[2]=Dataset();
        datasets[2].layer=3;
    }
    if(hl4==0){
        datasets[3]=Dataset();
        datasets[3].layer=4;
    }

    return hitcollection;
}

int atlaspix3_decoder::AlignData(char *package, int datalength, int start, int stop, bool ending)
{
    if(!withudpbug)
        return 0;

    if(start < 0)
        start = 0;
    bool repeat = true;
    if(stop == -1000)
        stop = datalength - 8;
    else
        repeat = false;

    //adapt allowed dataset indices to datamux / no datamux readout
    int dataindexlimit = 4;

    for(int offset = start; offset < datalength-8 && offset - start < stop; ++offset)
    {
        //do not use the debug data... (source = 0) and useful (< 5)
        if(((package[offset] >> 4) & 15) >= 0 && ((package[offset] >> 4) & 15) < 5
            //  dataset index for datamux data ( \in {1,2,3} )
            && (package[offset] & 15) < dataindexlimit && (package[offset] & 15) != 0
            //  and the next dataset (+8 bytes) also fits:
            && ((((package[offset+8] >> 4) & 15) >= 0 && ((package[offset+8] >> 4) & 15) < 5
            && (package[offset+8] & 15) < dataindexlimit && (package[offset+8] & 15) != 0) || ending))
                return offset;
    }
    if(start != 0 && repeat)
        return AlignData(package, datalength, 0, start, ending);
    else
        return -1;
}


atlaspix3_decoder_nomux::atlaspix3_decoder_nomux()
{
    if(datasets.size() == 0)
    {
        for(int i = 0; i < 4; ++i)
        {
            datasets.push_back(Dataset());
            datasets[i].layer = i+1;
        }
    }
}

void atlaspix3_decoder_nomux::ResetDecoder()
{
    int layer = 0;
    for(auto& it : datasets)
    {
        it = Dataset();
        it.layer = ++layer;
    }
}

Dataset atlaspix3_decoder_nomux::DecodeData(char *package, int packageid)
{
    Dataset finishedhit;

    int layer = (package[0] >> 4) & 15;

    int index = ((layer > 0)?layer-1:0);

#ifdef DEBUG
        std::cout << int(package[0]) << " -> " << "layer: " << layer << std::endl;
#endif

    if(layer >= 0 && layer < 5)
    {
        switch(package[0] & 15)
        {
            case(6):
                datasets[index].column = 131 - (int(package[1]) & 255);
                datasets[index].complete |= 32;
                break;
            case(7):
                datasets[index].row = (int(package[1]) & 1) * 256;
                datasets[index].complete |= 64;
                break;
            case(8):
            {
                datasets[index].row |= int(package[1]) & 255;
                int id = 0;
                for(int i = 5; i < 8; ++i)
                    id = id * 256 + (int(package[i]) & 255);
                datasets[index].triggerindex = id;
                datasets[index].complete |= 128;
            }
                break;
            case(9):
            {
                datasets[index].shortts = (int(package[1]) & 3) * 256;

                int trigts = 0;
                for(int i = 5; i < 8; ++i)
                    trigts = trigts * 256 + (int(package[i]) & 255);
                datasets[index].triggerts = trigts;
                datasets[index].complete |= 256;
            }
                break;
            case(10):
            {
                datasets[index].shortts |= int(package[1]) & 255;
                long long ext_ts2 = 0;
                for(int i = 3; i < 8; ++i)
                    ext_ts2 = ext_ts2 * 256 + (int(package[i]) & 255);
                datasets[index].ts2 = int(ext_ts2);
                datasets[index].complete |= 512;
            }
                break;
            case(11):
            {
                long long ext_ts = 0;
                for(int i = 3; i < 8; ++i)
                    ext_ts = ext_ts * 256 + (int(package[i]) & 255);
                datasets[index].ts = int(ext_ts);
                datasets[index].complete |= 1024;
            }
                break;
            case(12):
                datasets[index].shortts2 = (int(package[1]) & 127);
                datasets[index].complete |= 2048;
                break;
            default:
                std::cerr << "This should not happen. Is DataMux turned on?" << std::endl;
#ifdef DEBUG
                //debug output for testing:
                for(int i = 0; i < 8; ++i)
                    std::cout << int(package[i]) << " ";
                std::cout << std::endl;
#endif
                break;
        }

        if(datasets[index].is_complete())
        {
            //correct for the wrong data assignment of row and tot:
            int row = ((datasets[index].row & 2)?1:0) + ((datasets[index].row & 1)?2:0);
            for(int i = 0;i < 7; ++i)
                row += (datasets[index].shortts2 & (64 >> i))?(4 << i):0;
            if(row < 186)
                datasets[index].row = 185 - row;
            else
                datasets[index].row = row;
#warning TS2 is not corrected here...

            datasets[index].packageid = packageid;

            //directly reject hits with invalid address:
            if(datasets[index].row < 372 && datasets[index].column < 132)
                finishedhit = datasets[index];
            datasets[index] = Dataset();  //clear the spot
            datasets[index].layer = layer;

            if(index < int(tsoffset.size()))
            {
                finishedhit.ts += tsoffset[index];
                finishedhit.shortts = (finishedhit.shortts + tsoffset[index]) % 1024;
                while(finishedhit.shortts < 0)
                    finishedhit.shortts += 1024;
                finishedhit.ts2 += ts2offset[index];
                finishedhit.shortts2 = (finishedhit.shortts2 + ts2offset[index]) % 128;
                while(finishedhit.shortts2 < 0)
                    finishedhit.shortts2 += 128;
            }
        }
    }
    else
        std::cerr << "Found invalid layer!" << std::endl;

    return finishedhit;
}

std::vector<Dataset> atlaspix3_decoder_nomux::DecodePackage(char *package, int length)
{
    std::vector<Dataset> hitcollection;

    const char header[] = {char(0x80), char(0x81), char(0x82), char(0x83), char(0x84), char(0x85)};
    const char empty[]  = {0, 0, 0, 0, 0, 0, 0, 0};

    int position = 0;

    char content[16] = {0};

    //data alignment variables:
    int lastoffset = 0;

    int packageid = -1;
    //int datasetcount[4] = {0};

    int failcount = 0;
    const int maxfails = 400;

    //read the first package:

    while(position < length)
    {
#ifdef DEBUG
        //only output if not empty data (all '0'):
        if(!Compare(package, empty, 8))
        {
            std::cout << "Package (" << position << "): ";
            for(int i = 0; i < 8; ++i)
                std::cout << CharToHex(package[i]) << " ";
            std::cout << std::endl;
        }
#endif

        //get package ID:
        if(Compare(package, header, 6))
        {
            packageid = int((unsigned char)(package[6])) * 256 + int((unsigned char)(package[7]));
            package  += 8;
            position += 8;
            failcount = 1; //to distinguish from data
#ifdef DEBUG
            std::cout << "  -> ID" << std::endl;
#endif
            continue;
        }
        //decode the data from the data concentrator if not empty data:
        else if(!Compare(package, empty, 8))
        {
            for(int i = 0; i < 8; ++i)
                content[i] = content[i+8];
            for(int i = 8; i < 16; ++i)
                content[i] = package[i-8];

            //data alignment:
            int offset = AlignData(content, 16, lastoffset);
#ifdef DEBUG
            std::cout << "offset: " << offset << std::endl;
#endif
            if(offset >= 0)
            {
                if(offset < lastoffset)
                {
                    int newoffset = AlignData(content, 16, lastoffset);
                    if(newoffset == lastoffset)
                        offset = lastoffset;
                }

                Dataset newhit = DecodeData(&(content[offset]), packageid);
                if(newhit.is_complete())
                    hitcollection.push_back(newhit);
                lastoffset = offset;
            }
            else
            {
                int endalign = AlignData(content, 16, lastoffset, -1000, true);

                if(endalign >= 0)
                {
                    Dataset newhit = DecodeData(&(content[endalign]), packageid);
                    if(newhit.is_complete())
                        hitcollection.push_back(newhit);
                }
#ifdef DEBUG
                else //if(offset < 0)
                    std::cout << "Error: unalignable dataset found" << std::endl;

                    std::cout << "  -> Data" << std::endl;
#endif
            }
            failcount = 0;
        }
        //eternal loop prevention:
        else
        {
            failcount++;

#ifdef DEBUG
        //std::cout << "  -> fail" << std::endl;
#endif

            if(failcount > maxfails)
            {
                std::cout << "too many fails (" << maxfails << ") in a row, aborting ..." << std::endl;
                break;
            }
        }


        package += 8;
        position += 8;
    }

    return hitcollection;
}

int atlaspix3_decoder_nomux::AlignData(char *package, int datalength, int start, int stop, bool ending)
{
    if(!withudpbug)
        return 0;

    if(start < 0)
        start = 0;
    bool repeat = true;
    if(stop == -1000)
        stop = datalength - 8;
    else
        repeat = false;

    //adapt allowed dataset indices to datamux / no datamux readout
    int dataindexlimit = 13;

    for(int offset = start; offset < datalength-8 && offset - start < stop; ++offset)
    {
        //do not use the debug data... (source = 0) and useful (< 5)
        if(((package[offset] >> 4) & 15) >= 0 && ((package[offset] >> 4) & 15) < 5
            //  dataset index for datamux data ( \in {1,2,3} )
            && (package[offset] & 15) < dataindexlimit && (package[offset] & 15) != 0
            //  and the next dataset (+8 bytes) also fits:
            && ((((package[offset+8] >> 4) & 15) >= 0 && ((package[offset+8] >> 4) & 15) < 5
            && (package[offset+8] & 15) < dataindexlimit && (package[offset+8] & 15) != 0) || ending))
                return offset;
    }
    if(start != 0 && repeat)
        return AlignData(package, datalength, 0, start, ending);
    else
        return -1;
}

atlaspix3_decoder_triggered::atlaspix3_decoder_triggered()
{
    for(int i = 0; i < 1; ++i)
    {
        incompletehits.push_back(std::deque<Dataset>());
        completehits.push_back(std::deque<Dataset>());
    }

    if(datasets.size() == 0)
    {
        for(int i = 0; i < 1; ++i)
        {
            datasets.push_back(Dataset());
            datasets[i].layer = i; //i+1;
        }
    }
    lastoffset = -1;

    lastwasdoublebyte = false;

    tsformat1        = 0;
    tsformat2        = 0;
    tsformaterror    = 0;
    notexpectedtsformat = 0;
}

void atlaspix3_decoder_triggered::ResetDecoder()
{
    lastoffset = -1;

    tsformat1        = 0;
    tsformat2        = 0;
    tsformaterror    = 0;
    notexpectedtsformat = 0;
}

int InvertBitOrder(int number, int numbits)
{
    int result = 0;

    for(int i = 0; i < numbits; ++i)
        result += (number & (1 << i))?(1 << (numbits - 1 - i)):0;

    return result;
}

Dataset atlaspix3_decoder_triggered::DecodeData(char *package, int packageid)
{
    Dataset finishedhit;

    int layer = 0;
    int index = 0; //((layer > 0)?layer-1:0); //layer is 0 for single chip setup

#ifdef DEBUG
    std::cout << int(package[0]) << " -> " << "layer: " << layer << std::endl;
#endif

    if(layer >= 0 && layer < 5)
    {
        switch((package[0] >> 4) & 15)
        {
            case(1):
                datasets[index].triggerts = int(package[0]) & 3;
                for(int i = 1; i < 5; ++i)
                    datasets[index].triggerts = datasets[index].triggerts * 256
                                                    + (int(package[i]) & 255);
                datasets[index].triggerts = datasets[index].triggerts * 64
                                                + (int(package[5] >> 2) & 63);
                datasets[index].shortts = (int(package[6]) & 3) * 256 + (int(package[7]) & 255); //still Gray encoded
                datasets[index].shortts = GrayDecode(datasets[index].shortts, 10);
                datasets[index].triggertag = (int(package[5]) & 3) * 32 + (int(package[6] >> 3) & 31);
                datasets[index].fifofull = ((package[6] & 4)?true:false);
                datasets[index].complete |= 1;
            break;
            case(2):
                datasets[index].ts = (int(package[3]) & 255);
                for(int i = 4; i < 8; ++i)
                    datasets[index].ts = datasets[index].ts * 256 + (int(package[i]) & 255);
                datasets[index].triggerindex = (int(package[0]) & 15);
                for(int i = 1; i < 3; ++i)
                    datasets[index].triggerindex = datasets[index].triggerindex * 256
                                                    + (int(package[i]) & 255);
                datasets[index].complete |= 2;
            break;
            case(3):
                //datasets[index].newline = (package[0] & 8);
                datasets[index].ts2 = (int(package[1]) & 255);
                for(int i = 2; i < 5; ++i)
                    datasets[index].ts2 = datasets[index].ts2 * 256 + (int(package[i]) & 255);
                datasets[index].column = 131 - (int(package[7]) & 255);
                datasets[index].row = (int(package[5]) & 1) * 256 + (int(package[6]) & 255);
                datasets[index].row = (~datasets[index].row) & 511;
                if(datasets[index].row < 186)
                    datasets[index].row = 185 - datasets[index].row;
                datasets[index].shortts2    = ((int(package[5]) / 2) & 127);
                datasets[index].shortts2 = GrayDecode(~(datasets[index].shortts2), 7);
                if(tsformat2 < formatdecision)
                {
                    ++tsformat1;
                    datasets[index].complete |= 4; //case 3 and 4 are used either one or the other, so it is ok
                                                     //  to share the complete flag
                }
                else
                {
                    ++tsformaterror;
#ifdef DEBUG
                    std::cerr << "wrong TS format (1) found. " << tsformaterror << " errors" << std::endl;
#endif
                }
            break;
            case(4):
                //datasets[index].newline = (package[0] & 8);
                datasets[index].column = 131 - (int(package[7]) & 255);
                datasets[index].row = (int(package[5]) & 1) * 256 + (int(package[6]) & 255);
                datasets[index].row = (~datasets[index].row) & 511;
                if(datasets[index].row < 186)
                    datasets[index].row = 185 - datasets[index].row;
                datasets[index].shortts1 = (int(package[4]) & 7) * 128 + (int(package[5] >> 1) & 127);
                datasets[index].ts2 = (int(package[0]) & 7);
                for(int i = 1; i < 4; ++i)
                    datasets[index].ts2 = datasets[index].ts2 * 256 + (int(package[i]) & 255);
                datasets[index].ts2 = datasets[index].ts2 * 32 + (int(package[4] >> 3) & 31);
                if(tsformat1 < formatdecision)
                {
                    ++tsformat2;
                    datasets[index].complete |= 4; //case 3 and 4 are used either one or the other, so it is ok
                                                     //  to share the complete flag
                }
                else
                {
                    ++tsformaterror;
#ifdef DEBUG
                    std::cerr << "wrong TS format (2) found. " << tsformaterror << " errors" << std::endl;
#endif
                }
            break;
            default:
                //std::cerr << "This should not happen. Is this triggered readout?" << std::endl;
                          //<< "   package (ID: " << CharToHex(packageid/256) << " "
                          //<< CharToHex(packageid % 256) << "): ";
                //std::cerr.flush();
                break;
        }

        if(datasets[index].complete & 4)
        {
            datasets[index].packageid = packageid;
            //directly reject hits with invalid address:
            if(datasets[index].row < 372 && datasets[index].column < 132)
                incompletehits[index].push_back(datasets[index]);
            datasets[index].complete &= ~4; //clear the hitword flag
        }

        if(datasets[index].complete & 1)
        {
            for(auto& it : incompletehits[index])
            {
                it.triggerts  = datasets[index].triggerts;
                it.shortts    = datasets[index].shortts;
                it.triggertag = datasets[index].triggertag;
                it.fifofull   = datasets[index].fifofull;
                it.complete  |= 1;
            }
            datasets[index] = Dataset();  //clear the spot
            datasets[index].layer = layer;

            completehits[index].insert(completehits[index].end(), incompletehits[index].begin(), incompletehits[index].end());
            incompletehits[index].clear();

            if(completehits[index].size() > 0)
            {
                finishedhit = completehits[index].front();
                completehits[index].pop_front();

                if(index < int(tsoffset.size()))
                {
                    finishedhit.ts += tsoffset[index];
                    finishedhit.shortts = (finishedhit.shortts + tsoffset[index]) % 1024;
                    while(finishedhit.shortts < 0)
                        finishedhit.shortts += 1024;
                    finishedhit.ts2 += ts2offset[index];
                    finishedhit.shortts2 = (finishedhit.shortts2 + ts2offset[index]) % 128;
                    while(finishedhit.shortts2 < 0)
                        finishedhit.shortts2 += 128;
                }
            }
            //if(datasetcount != nullptr)
            //    ++(datasetcount[(splitlayers)?index:0]);
        }
    }
    else
        std::cerr << "Found invalid layer!" << std::endl;

    return finishedhit;
}

std::vector<Dataset> atlaspix3_decoder_triggered::DecodePackage(char *package, int length)
{
    std::vector<Dataset> hitcollection;

    const char header[] = {char(0x80), char(0x81), char(0x82), char(0x83), char(0x84), char(0x85)};
    const char empty[]  = {0, 0, 0, 0, 0, 0, 0, 0};

    int position = 0;

    static char content[16] = {0};

    //data alignment variables:
    int lastoffset = 0;

    int packageid = -1;
    //int datasetcount[4] = {0};

    int failcount = 0;
    const int maxfails = 400;
    bool veto = false;  //for data being aligned to the very end of the data frame
                        //skip one 8byte block to realign to the front

    //read the first package:

    while(position < length)
    {
        //get package ID:
        if(Compare(package, header, 6))
        {
#ifdef DEBUG
            //only output if not empty data (all '0'):
            if(!Compare(package, empty, 8))
            {
                std::cout << "Package (" << position << "): ";
                for(int i = 0; i < 8; ++i)
                    std::cout << CharToHex(package[i]) << " ";
                std::cout << std::endl;
            }
#endif

            packageid = int((unsigned char)(package[6])) * 256 + int((unsigned char)(package[7]));
            package  += 8;
            position += 8;
            failcount = 1; //to distinguish from data
#ifdef DEBUG
            std::cout << "  -> ID" << std::endl;
#endif
            continue;
        }
        else if(!withudpbug && !Compare(package, empty, 8))
        {
            Dataset newhit = DecodeData(package, packageid);
            if(newhit.is_complete())
            {
                hitcollection.push_back(newhit);
                failcount = 0;
            }
        }
        //decode the data from the data concentrator if not empty data:
        //  (or the end of the package with no data (apart from bug last byte))
        else if(!Compare(package, empty, 8)
                && !(Compare(package, empty, 7) && position == 1016))
        {
            for(int i = 0; i < 8; ++i)
                content[i] = content[i+8];
            for(int i = 8; i < 16; ++i)
                content[i] = package[i-8];

#ifdef DEBUG
            //only output if not empty data (all '0'):
            if(!Compare(package, empty, 8))
            {
                std::cout << "Package (" << position-8 << "): ";
                for(int i = 0; i < 16; ++i)
                    std::cout << CharToHex(content[i]) << " ";
                std::cout << std::endl;
            }
#endif

            if(veto)
            {
                veto = false;
                lastoffset = 0;
                continue;
            }
            //data alignment:
            int offset = AlignData(content, 16, lastoffset);
#ifdef DEBUG
            std::cout << "offset: " << offset << std::endl;
#endif


            if(offset >= 0)
            {
                if(offset < lastoffset)
                {
                    int newoffset = AlignData(content, 16, lastoffset);
                    if(newoffset == lastoffset)
                        offset = lastoffset;
                }

                Dataset newhit = DecodeData(&(content[offset]), packageid);
                if(newhit.is_complete())
                    hitcollection.push_back(newhit);
                lastoffset = offset;
            }
            else
            {
                int endalign = AlignData(content, 16, lastoffset, -1000, true);

                if(endalign >= 0)
                {
                    Dataset newhit = DecodeData(&(content[endalign]), packageid);
                    if(newhit.is_complete())
                        hitcollection.push_back(newhit);
                }
#ifdef DEBUG
                else //if(offset < 0)
                    std::cout << "Error: unalignable dataset found" << std::endl;

                    std::cout << "  -> Data" << std::endl;
#endif
            }
            failcount = 0;

            if(offset == 8)
                veto = true;
        }
        //eternal loop prevention:
        else
        {
            failcount++;

#ifdef DEBUG
        //std::cout << "  -> fail" << std::endl;
#endif

            if(failcount > maxfails)
            {
                std::cout << "too many fails (" << maxfails << ") in a row, aborting ..." << std::endl;
                break;
            }
        }


        package += 8;
        position += 8;
    }

    //add also all other completed hits:
    for(auto& it : completehits)
    {
        hitcollection.insert(hitcollection.end(), it.begin(), it.end());
        it.clear();
    }

    return hitcollection;
}

void atlaspix3_decoder_triggered::SetTSFormat(bool format1)
{
    if(format1)
    {
        tsformat1 = formatdecision + 1;
        tsformat2 = 0;
    }
    else
    {
        tsformat1 = 0;
        tsformat2 = formatdecision + 1;
    }
}

int atlaspix3_decoder_triggered::GetFormatErrors()
{
    return tsformaterror;
}

bool atlaspix3_decoder_triggered::ValidStartByte(uint character, int wrongtsformat)
    //17 can not be equal as the LSB was cleared for character
{
    character &= 240;
    return character >= 16 && character <= 64 && int(character) != wrongtsformat;
}

int atlaspix3_decoder_triggered::AlignData(char *package, int datalength, int start, int stop, bool ending)
{
    if(!withudpbug)
        return 0;

    if(start < 0)
        start = 0;
    bool repeat = true;
    if(stop == -1000)
        stop = datalength - 8;
    else
        repeat = false;

    if(notexpectedtsformat == 0)
    {
        if(tsformat1 >= formatdecision)
            notexpectedtsformat = 4*16;
        else if(tsformat2 >= formatdecision)
            notexpectedtsformat = 3*16;
    }

    for(int offset = start; offset < datalength-8 && offset - start < stop; ++offset)
    {
        if(ValidStartByte(package[offset], 17)) //notexpectedtsformat))
        {
            if(package[offset] == package[offset + 1])
            {
                if(offset == 7 || ending || package[offset + 9] == 0
                        || ValidStartByte(package[offset + 9]))
                {
                    //-> offset + 1
                    //found alignment
                    lastwasdoublebyte = true;
                    if(offset != 7)
                    {
                        lastoffset = offset + 1;
                        return offset + 1;
                    }
                    else
                    {
                        lastoffset = 0;
                        return -1;
                    }
                }
            }
            else if(offset == start && lastwasdoublebyte) // alignment does not change without double byte
            {
                if(lastoffset == 0 && start == 7);
                else if(package[offset + 8] == 0 || ValidStartByte(package[offset + 8]))
                {
                    //-> offset
                    if(offset != 0)
                        lastwasdoublebyte = false;
                    lastoffset = offset;
                    return offset;
                }
            }
        }
    }
    if(start != 0 && repeat)
        return AlignData(package, datalength, 0, start, ending);
    else if(ending && ValidStartByte(package[lastoffset])) //, notexpectedtsformat))
        return lastoffset;
    else
        return -1;
}
