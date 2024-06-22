#pragma once 
#include <vector>
#include <string>
#include <unordered_map>
#include "link.h"


std::vector<std::string> findLinks(std::string const& sBody);

void removeDuplicates(std::vector<std::string>& vres, std::unordered_set<std::string>& ustUsed);

void absLinks(const std::string& sUrl, std::vector<std::string>& res);

std::string cleanText(std::string body);

std::vector<std::string> wordsFromText(std::string str);

std::string getHtmlContent(const Link& link);

std::unordered_map<std::string, uint16_t> wordsCounter(const std::vector<std::string>& words);

std::string linkToUrl(const Link& link);

Link urlToLink(const std::string& url);

