

#include "iniParser.h"



bool ini_parser::is_number(const std::string& s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && (std::isdigit(*it) || (*it == '.') || (*it == ' '))) ++it;

	return !s.empty() && it == s.end();
}


void ini_parser::Open_ini(std::string f_name)
{
	std::ifstream in(f_name);
	std::string line;
	bool is_reading_section = false;
	bool is_reading_value = false;
	bool delete_spases = true;
	std::string section_name;
	std::string value;
	std::string value_name;
	unsigned string_number = 0;
	bool first_section = 0;

	if (in.is_open())
	{
		while (std::getline(in, line))
		{
			string_number++;
			for (auto& s : line)
			{
				if (s == ';') break;
				if (s == '[')
				{
					if (!section_name.empty())
						section_name.erase();
					if (is_reading_section)
					{
						throw myException("duplicate bracket in line:", string_number);
					}
					if (!is_reading_value) is_reading_section = true;
					continue;
				}
				if (s == ']')
				{
					if (is_reading_section)
					{
						is_reading_section = false;
						first_section = true;
						continue;
					}
					else throw myException("wrong symbol at line:", string_number);
				}
				if (s == '=')
				{
					is_reading_value = true;
					continue;
				}
				if (s == ' ')
				{
					if (delete_spases) continue;
				}
				if (s != ' ')
				{
					if (is_reading_value && delete_spases) {
						delete_spases = false;
					}
				}
				if (is_reading_section)
					section_name.push_back(s);
				else if (is_reading_value)
				{
					if (!delete_spases) value.push_back(s);
				}
				else
				{
					if (first_section)
					{
						if (s != ' ') value_name.push_back(s);
					}
					else throw myException("no section for value in line: ", string_number);
				}
			}


			if (is_reading_value && !value.empty())
			{
				std::pair<std::string, std::string> p1(value_name, value);
				sections.insert(std::pair<std::string, std::pair<std::string, std::string>>(section_name, p1));
				value.erase();
			}
			is_reading_value = false;
			delete_spases = true;
			value_name.erase();
		}
	}
	else  throw myException("can't open file");
	in.close();
}

