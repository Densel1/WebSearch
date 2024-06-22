#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <map>
#include <string>
#include <cstdlib>
#include <regex>


#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <condition_variable>
#include <Windows.h>

#include "myException.h"
#include "iniParser.h"
#include "table.h"
#include "http_utils.h"
#include <functional>
#include <pqxx/pqxx>


std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
clientDB db;
bool exitThreadPool = false;



void threadPoolWorker() {
	std::unique_lock<std::mutex> lock(mtx);
	while (!exitThreadPool || !tasks.empty()) {
		if (tasks.empty()) {
			cv.wait(lock);
		}
		else {
			auto task = tasks.front();
			tasks.pop();
			lock.unlock();
			task();
			lock.lock();
		}
	}
}
void parseLink(const std::string& link, uint16_t depth, std::unordered_set<std::string>& oldVec)
{
	try {

		std::cout << "dpth = " << depth << "\t";

		std::this_thread::sleep_for(std::chrono::milliseconds(500));




		Link newLink = urlToLink(link);

		std::string html = getHtmlContent(newLink);

		if (html.size() == 0)
		{
			std::cout << "Failed to get HTML Content " << link <<  std::endl;
			return;
		}

		std::string unTagText = cleanText(html);

		std::vector<std::string> words = wordsFromText(unTagText);

		std::unordered_map<std::string, uint16_t> countWords = wordsCounter(words);

		std::vector<std::string> links = findLinks(html);
		
		absLinks(link, links);

		removeDuplicates(links, oldVec);

		db.insertData(m_con, link, countWords);
	
		if (depth > 0) {
			std::lock_guard<std::mutex> lock(mtx);

			for (auto& subLink : links)
			{
				tasks.push([subLink, depth, &oldVec]() { parseLink(subLink, depth - 1, oldVec); });
			}
			cv.notify_one();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

}



int main()
{
	ini_parser parser;
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	try {

		parser.Open_ini("data.ini");

		auto StartPage = parser.get_value<std::string>("ClientSettings.startPage");
		std::cout << "Start Page= " << StartPage << std::endl;
		int depth = parser.get_value<int>("ClientSettings.recursion");
		std::cout << "Recurency = " << depth << std::endl;

		m_con.host = parser.get_value<std::string>("ConnectSettings.host");
		std::cout << "db_host= " << m_con.host << std::endl;
		m_con.port = parser.get_value < std::string > ("ConnectSettings.port");
		std::cout << "port = " << m_con.port << std::endl;
		m_con.name = parser.get_value<std::string>("DataBase.nameDB");
		std::cout << "db_name = " << m_con.name << std::endl;
		m_con.user = parser.get_value<std::string>("DataBase.user");
		std::cout << "db_user = " << m_con.user << std::endl;
		m_con.pass = parser.get_value<std::string>("DataBase.password");
		std::cout << "db_pass = " << m_con.pass << std::endl;

		
		int numThreads = std::thread::hardware_concurrency();
		std::vector<std::thread> threadPool;

		for (int i = 0; i < numThreads; ++i) {
			threadPool.emplace_back(threadPoolWorker);
		}

		std::string link = StartPage;
        std::unordered_set<std::string> temp_set;
		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([link, &temp_set, depth]() { parseLink(link, depth, temp_set); });
			cv.notify_one();
		}


		std::this_thread::sleep_for(std::chrono::seconds(2));


		{
			std::lock_guard<std::mutex> lock(mtx);
			exitThreadPool = true;
			cv.notify_all();
		}

		for (auto& t : threadPool) {
			t.join();
		}

	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}

