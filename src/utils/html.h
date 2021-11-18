/*
    Utilities for working with HTML
*/
#ifndef HTML_H
#define HTML_H

#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

namespace HtmlUtils
{
    std::string ClearHtmlTags(const std::string& value);
    std::string UrlEncode(const std::string& value);
    std::string UrlDecode(const std::string& value);
    void StringToLower(std::string& value);
}

#endif // HTML_H