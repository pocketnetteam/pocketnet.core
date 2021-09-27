/*
    Utilities for working with HTML
*/
#include "html.h"

std::string ClearHtmlTags(std::string &input)
{
    bool copy = true;
    std::string plainString = "";
    std::stringstream convertStream;

    // remove all xml tags
    for (size_t i = 0; i < input.length(); i++)
    {
        convertStream << input[i];

        if (convertStream.str().compare("<") == 0) copy = false;
        else if (convertStream.str().compare(">") == 0)
        {
            copy = true;
            convertStream.str(std::string());
            continue;
        }

        if (copy) plainString.append(convertStream.str());

        convertStream.str(std::string());
    }

    return plainString;
}

std::string UrlEncode(std::string str)
{
    std::string new_str = "";
    char c;
    int ic;
    const char* chars = str.c_str();
    char bufHex[10];
    int len = str.length();
    
    for (int i = 0; i < len; i++) {
        c = chars[i];
        ic = c;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            new_str += c;
        else {
 #ifdef WIN32
            sprintf_s(bufHex, "%X", c);
 #else
            sprintf(bufHex, "%X", c);
 #endif
            if (ic < 16)
                new_str += "%0";
            else
                new_str += "%";
            new_str += bufHex;
        }
    }
    return new_str;
}

std::string UrlDecode(std::string str)
{
    std::string ret;
    char ch;
    int i, ii, len = str.length();

    for (i = 0; i < len; i++) {
        if (str[i] != '%') {
            if (str[i] == '+')
                ret += ' ';
            else
                ret += str[i];
        }
        else {
 #ifdef WIN32
            sscanf_s(str.substr(i + 1, 2).c_str(), "%x", &ii);
 #else
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
 #endif
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return ret;
}
