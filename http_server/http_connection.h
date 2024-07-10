#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <pqxx/pqxx>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


struct db_connect {
	std::string host;
	std::string port;
	std::string name;
	std::string user;
	std::string pass;
};


static db_connect m_con;


class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
protected:

	tcp::socket socket_;

	beast::flat_buffer buffer_{8192};

	http::request<http::dynamic_body> request_;

	http::response<http::dynamic_body> response_;


	net::steady_timer deadline_{
		socket_.get_executor(), std::chrono::seconds(60)};

	void readRequest();
	void processRequest();

	void createResponseGet();

	void createResponsePost();
	void writeResponse();
	void checkDeadline();
	std::vector<std::string> wordsFromText(const std::string& str);

public:
	HttpConnection(tcp::socket socket);
	void start();
};

