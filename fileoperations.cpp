#include "fileoperations.h"

#define fileoperationssources

std::string ConvertToWinPath(std::string path)
{
    std::string winpath = "";
    int pos = path.find('/');
    int lastpos = 0;

    while(pos != int(std::string::npos))
    {
        if(winpath == "")
            winpath = path.substr(0, pos);
        else
            winpath = winpath + "\\" + path.substr(lastpos, pos - lastpos);

        lastpos = pos + 1;
        pos = path.find('/', lastpos);
    }

    if(winpath == "")
        winpath = path;
    else
        winpath = winpath + "\\" + path.substr(lastpos);

    return winpath;
}

bool DirectoryExists(std::string path)
{
    struct stat info;

    if(stat(path.c_str(), &info) != 0) //file/dir does not exist
        return false;
    else
        return (info.st_mode & S_IFDIR) != 0;
}

bool FileExists(std::string path)
{
    struct stat info;
    if(stat(path.c_str(), &info) != 0) //file does not exist
        return false;
    else
        return (info.st_mode & S_IFREG) != 0;
}

bool CreateDirectory(std::string path)
{
#if defined(__linux__)
    return mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#else //windows
    return system(ConvertToWinPath("mkdir " + path).c_str()) == 0;
#endif
}

bool WriteToFile(std::string filename, std::string data, std::ios::openmode fileflags)
{
    if(filename == "")
        return false;

    std::fstream f;
    f.open(filename.c_str(), std::ios::out | fileflags);
    
    if(!f.is_open())
        return false;
        
    f << data << std::flush;
    f.close();
    
    return true;
}

std::string StripSpaces(std::string text)
{
    int start = 0;
    while(start < int(text.length()) && (text[start] == ' ' || text[start] == '\t'))
        ++start;

    int end = int(text.length()) - 1;
    while(end > start && (text[end] == ' ' || text[end] == '\t'))
        --end;

    return text.substr(start, end - start + 1);
            //+1 is ok, since for an empty string start will at length() and no data will be taken
}

StringPairs LoadParameterFile(std::string filename, std::string header, bool printerrors)
{
    if(filename == "")
        return StringPairs();
    
    std::fstream f;
    f.open(filename.c_str(), std::ios::in);
    if(!f.is_open())
        return StringPairs();
        
    StringPairs data;
    
    bool correctheader = true;
    
    std::string line;
    std::getline(f, line);
    int linecounter = 1;
    
    while(f.good())
    {
        if(line.length() > 0 && line[0] != '#' && correctheader) // no comment line
        {
            std::string key, value;
        
            if(line[0] == '\"')
            {
                int endkey = line.find('\"', 1);
                if(endkey == int(std::string::npos) && printerrors)
                    std::cerr << "Unmatched opening \'\"\' found, skipped line " << linecounter 
                              << "\"" << line << "\"" << std::endl;
                else
                {
                    key = line.substr(1, endkey - 1);
                    ++endkey;
                    //go to start of value after the key ended:
                    while((line[endkey] == ' ' || line[endkey] == '\t') && endkey < int(line.length()))
                        ++endkey;
                    value = StripSpaces(line.substr(endkey));
                }
            }
            else
            {
                int separator = line.find(' ');
                if(separator == int(std::string::npos))
                    separator = line.find('\t');
                key = StripSpaces(line.substr(0, separator));
                value = StripSpaces(line.substr(separator + 1));
            }

            if(value[0] == '\"') //take quotation mark enclosed part if started with quotation marks
            {
                int end = value.find('\"', 1);
                if(end != int(std::string::npos))
                    value = value.substr(1, end - 1);
            }
            else //remove comment at the end of the line
            {
                int end = value.find('#');
                value = StripSpaces(value.substr(0, end));
            }
            
            data.push_back(std::make_pair(key, value));
        }
        else if(line.substr(0,2).compare("##") == 0)
            correctheader = (line.find(header) == 3);
        
        std::getline(f, line);
        ++linecounter;
    }
    
    f.close();
    
#ifdef FILEOP_DEBUG
    for(const auto& it : data)
        std::cout << "\"" << it.first << "\": \"" << it.second << "\"\n";
    std::cout << std::flush;
#endif
    
    return data;
}

StringIntPairs LoadParameterFileInt(std::string filename, std::string header, bool printerrors)
{
    StringPairs data;
    data = LoadParameterFile(filename, header, printerrors);
    
    StringIntPairs dataint;
    
    for(const auto& it : data)
    {
        std::stringstream s(it.second);
        int value;
        if(s >> value)
            dataint.push_back(std::make_pair(it.first, value));
        else if(printerrors)
            std::cerr << "Error converting \"" << it.second << "\" for key \"" << it.first 
                      << "\" to int" << std::endl;
    }

    return dataint;
}

StringDoublePairs LoadParameterFileDouble(std::string filename, std::string header,
                                          bool printerrors)
{
    StringPairs data;
    data = LoadParameterFile(filename, header, printerrors);
    
    StringDoublePairs datadouble;
    
    for(const auto& it : data)
    {
        std::stringstream s(it.second);
        double value;
        if(s >> value)
            datadouble.push_back(std::make_pair(it.first, value));
        else if(printerrors)
            std::cerr << "Error converting \"" << it.second << "\" for key \"" << it.first 
                      << "\" to double" << std::endl;
   }

    return datadouble;
}

std::string LoadParameterFileHeader(std::string filename, std::string header)
{
    if(filename == "")
        return "";

    std::fstream f;
    f.open(filename.c_str(), std::ios::in);
    if(!f.is_open())
        return "";

    bool correctheader = true;
    std::string title = "";

    std::string line;
    std::getline(f, line);

    while(f.good())
    {
        if(line.substr(0, 2).compare("##") == 0) // no comment line
            correctheader = (line.find(header) == 3);

        if(line.substr(0, 8).compare("# Title:") == 0 && correctheader)
        {
            std::string header = line.substr(8); //cut off the "# Title:"
            header = StripSpaces(header);
            if(header[0] == '\"')
            {
                int end = header.find('\"', 1); //find the next (closing) quotation mark
                if(end == int(std::string::npos))
                    title = header.substr(1);
                else
                    title = header.substr(1, end - 1);
            }
            else
            {
                int comment = header.find('#');
                if(comment != int(std::string::npos))
                    header = header.substr(0, comment);
                title = StripSpaces(header);
            }
        }

        std::getline(f, line);
    }

    f.close();

    return title;
}

//TGraphErrors* LoadDataFile(std::string filename, std::string header, bool printerrors,
//                           std::string values)
//{
//    StringPairs data = LoadParameterFile(filename, header, printerrors);

//    if(data.size() == 0)
//        return nullptr;

//    values = ToLowerCase(values);

//    //check data format pattern:
//    for(int i = 0; i < int(values.length()); ++i)
//    {
//        switch(values[i])
//        {
//        case('x'):
//        case('y'):
//        //case('z'):
//        case('e'):
//        case('i'):
//        case(' '):
//            break;
//        default:
//            std::cout << "Data format not supported" << std::endl;
//            return nullptr;
//        }
//    }

//    std::map<std::string, int> fieldindex;

//    fieldindex.insert(std::make_pair("x", -1));
//    fieldindex.insert(std::make_pair("xe", -1));
//    fieldindex.insert(std::make_pair("y", -1));
//    fieldindex.insert(std::make_pair("ye", -1));
//    //fieldindex.insert(std::make_pair("z", -1));
//    //fieldindex.insert(std::make_pair("ze", -1));

//    std::stringstream sindex(values);

//    std::string part = "";
//    int index = 0;
//    while((sindex >> part))
//    {
//        auto it = fieldindex.find(part);
//        if(it != fieldindex.end())
//            it->second = index;
//        else if(part.compare("i") == 0)
//            std::cout << "ignore column " << index << std::endl;
//        else
//        {
//            std::cerr << "This should not happen: Detected invalid column identifier: \""
//                      << part << "\" at position " << index << std::endl;
//            return nullptr;
//        }

//        ++index;
//    }

//    //for(const auto& it : fieldindex)
//    //    std::cout << "Position of \"" << it.first << "\" is " << it.second << std::endl;

//    TGraphErrors* gr = new TGraphErrors();

//    for(const auto& it : data)
//    {
//        std::stringstream sline(it.first + " " + it.second + " 0");
//        int index = 0;
//        double value = 0;
//        gr->SetPoint(gr->GetN(), 0, 0);

//        //std::cout << "processing data: \"" << (it.first + " " + it.second + " 0") << "\""
//        //          << std::endl;

//        while((sline >> value))
//        {
//            if(index == fieldindex["x"])
//                gr->GetX()[gr->GetN() - 1] = value;
//            else if(index == fieldindex["y"])
//                gr->GetY()[gr->GetN() - 1] = value;
//            else if(index == fieldindex["xe"])
//                gr->SetPointError(gr->GetN() - 1, value, gr->GetErrorY(gr->GetN() - 1));
//            else if(index == fieldindex["ye"])
//                gr->SetPointError(gr->GetN() - 1, gr->GetErrorX(gr->GetN() - 1), value);

//            ++index;
//        }

//        //int pos = gr->GetN() - 1;
//        //std::cout << "   -> Data Point: (" << gr->GetX()[pos] << "|" << gr->GetY()[pos] << ")"
//        //          << " E(" << gr->GetErrorX(pos) << "|" << gr->GetErrorY(pos) << ")"
//        //          << std::endl;

//    }


//    return gr;
//}

std::string ToLowerCase(const std::string text)
{
    std::string ltext = "";

    for(const auto& c : text)
        if(c >= 'A' && c <= 'Z')
            ltext = ltext + char(c - ('A' - 'a'));
        else
            ltext = ltext + char(c);

    return ltext;
}

std::string FindKey(StringPairs &data, std::string key, const std::string fail)
{
    for(const auto& it : data)
    {
        if(it.first.compare(key) == 0)
            return it.second;
    }

    return fail;
}

int FindKeyInt(StringPairs &data, std::string key, int fail)
{
    std::string value = FindKey(data, key);

    if(value != "")
    {
        std::stringstream s(value);
        int val;
        if((s >> val))
            return val;
        else
            return fail;
    }
    else
        return fail;
}

double FindKeyDouble(StringPairs &data, std::string key, const double fail)
{
    std::string value = FindKey(data, key);

    if(value != "")
    {
        std::stringstream s(value);
        double val;
        if((s >> val))
            return val;
        else
            return fail;
    }
    else
        return fail;
}

bool FindKeyBool(StringPairs &data, std::string key, const bool fail)
{
    std::string value = FindKey(data, key);

    if(value != "")
        return ToLowerCase(value).compare("true") == 0;
    else
        return fail;
}

