#pragma once
#include <string>
#include <iostream>
#include <mutex>
#include <pqxx/pqxx>
#include "myException.h"



struct db_connect {
	std::string host;
	std::string port;
	std::string name;
	std::string user;
	std::string pass;
};


static db_connect m_con;

class clientDB
{
public:
	clientDB() {
	
	};

	void createTable(pqxx::work& tx);

	void insertData(db_connect con,  std::string Url, std::unordered_map<std::string, uint16_t> words);

private:
	std::string UrlsT = "Urls";
	std::string WordsT = "Words";
	std::string UrlsWordsT = "UrlsWords";
	std::string url = "url";
	std::string word = "word";
	std::string url_id = "url_id";
	std::string word_id = "word_id";
	std::string amount = "amount";
	std::mutex mu;


	std::wstring ansi2wideUtf(const std::string& str);
	std::string wideUtf2utf8(const std::wstring& wstr);
};