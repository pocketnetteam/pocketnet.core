/*
    Utilities for working with HTML
*/
#ifndef HTML_H
#define HTML_H

#include <iostream>
#include <sstream>

std::string ClearHtmlTags(std::string &input);

std::string UrlEncode(std::string str);

std::string UrlDecode(std::string str);

#endif // HTML_H