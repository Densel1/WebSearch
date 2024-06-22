#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include "table.h"
#include <Windows.h>



void clientDB::createTable(pqxx::work& tx)
{

	tx.exec("create table if not exists " + UrlsT + "(id serial primary key, " + url + " varchar(2500) not null unique)");
	tx.exec("create table if not exists " + WordsT + "(id serial primary key, " + word + " varchar(32) not null unique)");
	tx.exec("create table if not exists " + UrlsWordsT + "(" + url_id +	" integer references Urls(id), " + 
	word_id + " integer references words(id),  constraint pk PRIMARY KEY(" + url_id + ", "   + word_id + "), " +
			amount + " integer not null);");
}

void clientDB::insertData(db_connect con, std::string Url, std::unordered_map<std::string, uint16_t> words)
{
	std::lock_guard<std::mutex> lock(mu);
	std::string conf = "host=" + con.host + " " + "port=" + con.port + " " + "dbname=" + con.name + " "
		"user=" + con.user + " " + "password=" + con.pass;

	pqxx::connection c(conf);
	pqxx::work tx{ c };
	if (!(c.is_open())) throw myException("Не удалось подключиться к БД");
	createTable(tx);

	pqxx::result queryUrl = tx.exec
	(
		"SELECT id FROM " + UrlsT +
		" WHERE url = '" + Url + "'"
	);

	if (queryUrl.size()) {
		return;
	}
	else {

		tx.exec("insert into " + UrlsT + '(' + url + ") " + "values('" + tx.esc(Url) + "') on conflict (" + url + ") do nothing; ");

		for (std::unordered_map<std::string, uint16_t>::const_iterator it = words.cbegin(); it != words.end(); ++it)
		{
			std::string db_Word = it->first;

			std::wstring temp = ansi2wideUtf(db_Word);

			std::string db_word = wideUtf2utf8(temp);

			std::string db_amount = std::to_string(it->second);

	//		std::cout << db_word << std::endl;

			tx.exec(
				"insert into " + WordsT + '(' + word + ") "
				"values('" + tx.esc(db_word) + "') on conflict (" + word + ") do nothing;"
			);


			tx.exec(
				"insert into " + UrlsWordsT + '(' + url_id + ", " + word_id + ", " + amount + ") "
				"values((select id from " + UrlsT + " where " + url + " = '" + Url + "'), " +
				"(select id from " + WordsT + " where " + word + " = '" + tx.esc(db_word) + "'), " + tx.esc(db_amount) + ");"
			);
		}
		tx.commit();
	}
}

// Convert an ANSI string to a wide Unicode String
std::wstring clientDB::ansi2wideUtf(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

// Convert a wide Unicode string to an UTF8 string
std::string clientDB::wideUtf2utf8(const std::wstring& wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}
