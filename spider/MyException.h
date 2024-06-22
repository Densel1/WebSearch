#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <exception>
#include <string.h>
#include <stdio.h>
#include <map>
#include <string>
#include <iostream>

class myException :public std::exception
{
	char *myStr;
	int myNum;
public:

	myException(const char* str)
	{
		myStr = new char[sizeof(str)];
		strcpy(myStr, str);
	}
	myException(const char* str, int num)
	{
		char tmp[] = "0000000000";
		myStr = new char[sizeof(str)];
		strcpy(myStr, "ERROR: ");
		strcat(myStr, str);
		_itoa(num, tmp, 10);
		strcat(myStr, tmp);
	}
	myException(const std::multimap< std::string, std::pair<std::string, std::string>>& m, const std::string section)
	{
		myStr = new char[m.size() * 256];
		strcpy(myStr, "ERROR: avalable names for section \"");
		strcat(myStr, section.c_str());
		strcat(myStr, "\" are: \n");
		for (auto it = begin(m); it != end(m); ++it)
		{
			if (it->first == section)
			{
				strcat(myStr, (it->second.first).c_str());
				strcat(myStr, "\n\0");
			}
		}
	}

	const char* what() const override
	{
		return myStr;
	}
};