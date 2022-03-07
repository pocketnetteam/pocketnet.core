/*
    Utilities for working with HTML
*/
#include "util/html.h"

namespace HtmlUtils
{
    std::string ClearHtmlTags(const std::string& value)
    {
        bool copy = true;
        std::string plainString = "";
        std::stringstream convertStream;

        // remove all xml tags
        for (size_t i = 0; i < value.length(); i++)
        {
            convertStream << value[i];

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

    std::string UrlEncode(const std::string& value)
    {
        std::string new_str = "";
        char c;
        int ic;
        const char* chars = value.c_str();
        char bufHex[10];
        int len = value.length();
        
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

    std::string UrlDecode(const std::string& value)
    {
        std::string ret;
        char ch;
        int i, ii, len = value.length();

        for (i = 0; i < len; i++) {
            if (value[i] != '%') {
                if (value[i] == '+')
                    ret += ' ';
                else
                    ret += value[i];
            }
            else {
                #ifdef WIN32
                sscanf_s(value.substr(i + 1, 2).c_str(), "%x", &ii);
                #else
                sscanf(value.substr(i + 1, 2).c_str(), "%x", &ii);
                #endif
                ch = static_cast<char>(ii);
                ret += ch;
                i = i + 2;
            }
        }
        return ret;
    }

    void StringToLower(std::string& value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](char c) { return 'A' <= c && c <= 'Z' ? c ^ 32 : c; });
    }

} // namespace HtmlUtils