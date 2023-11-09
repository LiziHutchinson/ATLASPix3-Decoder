#include "dataset.h"

///converts the passed string to lower case: [A-Z] -> [a-z]
std::string DatasetFunctions::ToLowerCase(std::string text)
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
std::string DatasetFunctions::StripSpaces(std::string text)
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

Dataset DatasetFunctions::FindOrder(std::string line)
{
    if(line[0] != '#') //no header line
        return Dataset();
    else
    {
        Dataset fieldorder;
        unsigned int numfields = 0;

        line = std::string(line).substr(2);

         numfields = std::count(line.begin(), line.end(), ';');
        if(*line.rbegin() != ';')
        {
            //std::cout << "last character: \"" << *line.rbegin() << "\"" << std::endl;
            ++numfields;
        }

        fieldorder.complete = numfields;

        int start = 0;
        int end = 0;
        int fieldcounter = 0;
        while(end != int(std::string::npos))
        {
            end = int(line.find(';', start));
            std::string field = DatasetFunctions::ToLowerCase(
                                    DatasetFunctions::StripSpaces(line.substr(start,
                                                                              end - start)));

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
                fieldorder.ts           = fieldcounter;
            else if(field.compare("ext. ts2") == 0)
                fieldorder.ts2          = fieldcounter;
            else if(field.compare("ts") == 0)
                fieldorder.shortts      = fieldcounter;
            else if(field.compare("ts1") == 0)
                fieldorder.shortts1     = fieldcounter;
            else if(field.compare("ts2") == 0)
                fieldorder.shortts2     = fieldcounter;
            else if(field.compare("fifo overflow") == 0)
                fieldorder.fifowasfull  = fieldcounter;
            else if(field.compare("triggertag") == 0)
                fieldorder.triggertag   = fieldcounter;
            else if(field.compare("triggertable overflow") == 0)
                fieldorder.fifofull     = fieldcounter;
            else
                std::cout << "Warning: unknown field \"" << field << "\" found." << std::endl;

            start  = end + 1;
            ++fieldcounter;
        }

        return fieldorder;
    }
}

/**
 * @brief Construct fills the entries of the Dataset object from the text passed
 *      assuming the data field order and count from `order`
 * @param line              - the data to extract from
 * @param order             - Dataset containing the field indices (starting at 0)
 *                                for the fields to extract (-1 for do not use)
 * @return                  - a reconstructed Dataset or an empty Dataset on an error
 */
Dataset DatasetFunctions::Construct(std::string line, Dataset order)
{
    std::stringstream s(line);

    const int entriesperline = order.complete;

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
    if(order.triggertag   != -1) dat.triggertag   = short(data[order.triggertag]);
    if(order.ts           != -1) dat.ts           = data[order.ts];
    if(order.ts2          != -1) dat.ts2          = data[order.ts2];
    if(order.shortts      != -1) dat.shortts      = data[order.shortts];
    if(order.shortts1     != -1) dat.shortts1     = short(data[order.shortts1]);
    if(order.shortts2     != -1) dat.shortts2     = data[order.shortts2];
    if(order.packageid    != -1) dat.packageid    = int(data[order.packageid]);
    if(order.layer        != -1) dat.layer        = short(data[order.layer]);
    if(order.fifofull     != -1) dat.fifofull     = short(data[order.fifofull]);
    if(order.fifowasfull  != -1) dat.fifowasfull  = short(data[order.fifowasfull]);
    dat.complete = 7;

    return dat;
}
