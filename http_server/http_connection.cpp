#include "http_connection.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include "../spider/iniParser.h" // заменить  на spider


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;



std::string url_decode(const std::string& encoded) {
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch)) {
		if (ch == '%') {
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else {
			res += ch;
		}
	}

	return res;
}

std::string convert_to_utf8(const std::string& str) {
	std::string url_decoded = url_decode(str);
	return url_decoded;
}

HttpConnection::HttpConnection(tcp::socket socket)
	: socket_(std::move(socket))
{
}


void HttpConnection::start()
{
	readRequest();
	checkDeadline();
}


void HttpConnection::readRequest()
{
	auto self = shared_from_this();

	http::async_read(
		socket_,
		buffer_,
		request_,
		[self](beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);
			if (!ec)
				self->processRequest();
		});
}

void HttpConnection::processRequest()
{
	response_.version(request_.version());
	response_.keep_alive(false);

	switch (request_.method())
	{
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponseGet();
		break;
	case http::verb::post:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponsePost();
		break;

	default:
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body())
			<< "Invalid request-method '"
			<< std::string(request_.method_string())
			<< "'";
		break;
	}

	writeResponse();
}


void HttpConnection::createResponseGet()
{
	if (request_.target() == "/")
	{
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<p>Welcome!<p>\n"
			<< "<form action=\"/\" method=\"post\">\n"
			<< "    <label for=\"search\">Search:</label><br>\n"
			<< "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
			<< "    <input type=\"submit\" value=\"Search\">\n"
			<< "</form>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void HttpConnection::createResponsePost()
{
	try {

		if (request_.target() == "/")
		{
			std::string s = buffers_to_string(request_.body().data());

			std::cout << "POST data: " << s << std::endl;

			size_t pos = s.find('=');
			if (pos == std::string::npos)
			{
				response_.result(http::status::not_found);
				response_.set(http::field::content_type, "text/plain");
				beast::ostream(response_.body()) << "File not found\r\n";
				return;
			}

			std::string key = s.substr(0, pos);
			std::string value = s.substr(pos + 1);

			std::string utf8value = convert_to_utf8(value);

			if (key != "search")
			{
				response_.result(http::status::not_found);
				response_.set(http::field::content_type, "text/plain");
				beast::ostream(response_.body()) << "File not found\r\n";
				return;
			}

			// TODO: Fetch your own search results here
			std::vector<std::pair<std::string, int>> res;
			ini_parser parser;
			parser.Open_ini("C:\\data.ini");
			m_con.host = parser.get_value<std::string>("ConnectSettings.host");
			std::cout << "db_host= " << m_con.host << std::endl;
			m_con.port = parser.get_value < std::string >("ConnectSettings.port");
			std::cout << "port = " << m_con.port << std::endl;
			m_con.name = parser.get_value<std::string>("DataBase.nameDB");
			std::cout << "db_name = " << m_con.name << std::endl;
			m_con.user = parser.get_value<std::string>("DataBase.user");
			std::cout << "db_user = " << m_con.user << std::endl;
			m_con.pass = parser.get_value<std::string>("DataBase.password");
			std::cout << "db_pass = " << m_con.pass << std::endl;

			std::string con_str = "host=" + m_con.host +
				" " + "port=" + m_con.port +
				" " + "dbname=" + m_con.name +
				" " + "user=" + m_con.user +
				" " + "password=" + m_con.pass;


			pqxx::connection connection(con_str);
			pqxx::work work{ connection };

			std::string QueryWords;
			std::vector<std::string> queryWords = wordsFromText(s);
			std::unordered_map<std::string, int> querySqlResponse;

			for (int i = 0; i < queryWords.size(); ++i) {
				for (auto [url, amount] : work.query<std::string, int>(
					"select url, amount from Urls "
					"join UrlsWords on Urls.id = UrlsWords.url_id "
					"where word_id = (select id from words where word = '" + queryWords.at(i) + "');")) {
					auto it = querySqlResponse.find(url);
					if (it == querySqlResponse.end())
						querySqlResponse.emplace(url, amount);
					else
						it->second += amount;
				}
			}
			std::multimap<int, std::string> SortByAmount;
			for (auto it = querySqlResponse.begin(); it != querySqlResponse.end(); ++it)
			{
				SortByAmount.emplace(it->second, it->first);
			}
			connection.close();
			std::vector<std::string> searchResult;

			for (const auto& word : SortByAmount)
			{
				searchResult.push_back(word.second);
			}


			std::reverse(searchResult.begin(), searchResult.end());


			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body())
				<< "<html>\n"
				<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
				<< "<body>\n"
				<< "<h1>Search Engine</h1>\n"
				<< "<p>Response:<p>\n"
				<< "<ul>\n";

			for (const auto& url : searchResult) {

				beast::ostream(response_.body())
					<< "<li><a href=\""
					<< url << "\">"
					<< url << "</a></li>";
			}

			if (searchResult.size() == 0)
			{
				beast::ostream(response_.body())
					<< "\nNo result";
			}

			if (queryWords.size() == 0) beast::ostream(response_.body()) << "<br>" << "\n Empty request";


			beast::ostream(response_.body())
				<< "</ul>\n"
				<< "</body>\n"
				<< "</html>\n";
		}
		else
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
		}
	}
catch (std::runtime_error const& e)
{
	beast::ostream(response_.body())
		<< "<br>" << e.what();
//	std::cerr << "Error: " << e.what() << std::endl;

}
}

void HttpConnection::writeResponse()
{
	auto self = shared_from_this();

	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
}

void HttpConnection::checkDeadline()
{
	auto self = shared_from_this();

	deadline_.async_wait(
		[self](beast::error_code ec)
		{
			if (!ec)
			{
				self->socket_.close(ec);
			}
		});
}


std::vector<std::string> HttpConnection::wordsFromText(const std::string& Str)
{
	std::string temp;
	std::vector<std::string> result;
	std::string str = convert_to_utf8(Str.substr(7, Str.length()));

	boost::algorithm::to_lower(str);
	const uint8_t n = 3;
	const uint8_t max = 32;

	for (int i = 0; i < str.size(); i++)
	{
		if (str.at(i) == '+')
		{
			if ((temp.length() > n) && (temp.length() < max)) result.push_back(temp);
			temp = "";
		}
		else
		{
			temp += str.at(i);
		}
	}

	if ((temp.length() > n) && (temp.length() < max))
	{
		std::cout << temp << std::endl;
		result.push_back(temp);
	}

	return result;
}