

#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "myException.h"
#include <iostream>


class ini_parser
{
private:
	std::multimap<std::string, std::pair<std::string, std::string>> sections;

public:
	ini_parser() {};

	template <typename T>
	T get_value(const std::string& line)
	{
		std::string sectoin_name;
		std::string str;
		bool point_char = false;
		//section
		std::string sel;
		//value
		std::string val;
		for (auto& s : line)
		{
			if (s == '.') {
				point_char = true;
				continue;
			}
			if (!point_char)
			{
				sel.push_back(s);
			}
			else { val.push_back(s); }

		}

		for (auto it = begin(sections); it != end(sections); it++)
		{
			if ((it->first) == sel)
			{
				if (val == (it->second.first))
				{
					str = it->second.second;
				}
			}
		}
		if (str.empty()) throw myException(sections, sel);

		// this works in C++17
		if constexpr (std::is_same_v<T, std::string>) return str;
		else if constexpr (std::is_same_v<T, int>)
		{
			if (is_number(str)) return atoi(str.c_str());
			else throw myException("not a number");
		}
	}

	void Open_ini(std::string f_name);

	~ini_parser() = default;

private:


	bool is_number(const std::string& s);
};

