#include "http_utils.h"

#include <regex>
#include <iostream>
#include <vector>
#include <queue>


#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <openssl/ssl.h>
#include "link.h"
#include "gumbo.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;

using tcp = boost::asio::ip::tcp;



Link urlToLink(const std::string& url)
{
	std::regex pattern("^(?:(https?)://)([^/]+)(/.*)?");
	std::smatch mr;
	std::regex_search(url, mr, pattern);

	Link result;


	if (mr[1].length() != 0)
	{
		if (mr[1].str() == "http") {
			result.protocol = ProtocolType::HTTP;
		}
		else if (mr[1].str() == "https") {
			result.protocol = ProtocolType::HTTPS;
		}
	}
	else 
	{
		result.protocol = ProtocolType::NONE;
	}

	
	if (mr[2].length() != 0)
	{
		result.hostName = mr[2].str();
	}
	else
	{
		result.hostName = "";
	}

	if (mr[3].length() != 0) {
			result.query = mr[3].str();
	}
	else {
		result.query = '/';
	}
	return result;
}


// ищет дерево ссылок
std::vector<std::string> findLinks(std::string const& sBody)
{
	static uint32_t linkNum = 0;
	std::cout << "Serching " << linkNum++ << " Link ... " << "\t\t";

	std::vector<std::string> vLinks;

	GumboAttribute* hrefBase = nullptr;
	GumboOutput* output = gumbo_parse(sBody.c_str());

	std::queue<GumboNode*> qn;
	qn.push(output->root);

	while (!qn.empty())
	{
		GumboNode* node = qn.front();
		qn.pop();
		if (GUMBO_NODE_ELEMENT == node->type)
		{
			GumboAttribute* href = nullptr;
			if (node->v.element.tag == GUMBO_TAG_A && (href = gumbo_get_attribute(&node->v.element.attributes, "href")))
			{
				std::string sLnk{ href->value };
				if (!sLnk.empty())
				{
					vLinks.emplace_back(href->value);
				}
			}
			GumboVector* children = &node->v.element.children;
			for (unsigned int i = 0; i < children->length; ++i)
			{
				qn.push(static_cast<GumboNode*>(children->data[i]));
			}
		}
	}

	
	if (hrefBase) // с учётом тега <base>
	{
		std::string sBase = hrefBase->value;

		if (sBase.back() == '/')
		{
			sBase.pop_back();
		}
		for (auto& sLnk : vLinks)
		{
			if (std::regex_match(sLnk, std::regex{ "(?:[^/]+/)+[^/]+" }) || std::regex_match(sLnk, std::regex{ "[^/#?]+" })) // относительно дочерней или текущей директории
			{
				sLnk = sBase + '/' + sLnk;
			}
			else if (sLnk.find("../") == 0) // относительно родительской директории
			{
				int ind = std::string::npos;
				int cnt = (sLnk.rfind("../") + 3) / 3;
				for (int i = 0; i < cnt + 1; ++i)
				{
					ind = sBase.rfind('/', ind - 1);
				}
				sLnk = std::string{ sBase.begin(), sBase.begin() + ind + 1 } + std::string{ sLnk.begin() + cnt * 3, sLnk.end() };
			}
			sLnk;  
		}
	}
	
	gumbo_destroy_output(&kGumboDefaultOptions, output);
	std::cout << "Found " << vLinks.size() << " references" << "\t\t";
	return vLinks;
}




// копирует найденные ссылки в ustUsed и удаляет дубликаты и ссылки на фрагменты
void removeDuplicates(std::vector<std::string>& vres, std::unordered_set<std::string>& ustUsed)
{
	uint16_t deleted = 0;
	for (int i = 0; i < vres.size(); ++i)
	{
		if (vres.at(i).at(0) == '#' || !ustUsed.emplace(vres.at(i)).second)
		{
			vres.erase(vres.begin() + i);
			--i;
			++deleted;
		}
	}
	std::cout << "Reference deleted: " << deleted << "\t\t";
}



// исправляет некоторые относительные ссылки в абсолютные
void absLinks(const std::string& sUrl, std::vector<std::string>& res)
{
	    std::smatch mr;
		
		for (auto &s_lnk:res)
		{
			if (s_lnk.find("//") == 0) // относительно протокола
			{
				std::regex_search(sUrl, mr, std::regex{ "^[^/]+" });
				s_lnk = mr.str() + s_lnk;
			}
			else if (s_lnk.find('/') == 0) // относительно имени хоста
			{
				std::regex_search(sUrl, mr, std::regex{ "^[^/]+//[^/]+" });
				s_lnk = mr.str() + s_lnk;
			}
			else if (s_lnk.find("../") == 0) // относительно родительской директории
			{
				int ind = std::string::npos;
				int cnt = (s_lnk.rfind("../") + 3) / 3;
				for (int i = 0; i < cnt + 1; ++i)
				{
					ind = sUrl.rfind('/', ind - 1);
				}
				s_lnk = std::string{ sUrl.begin(), sUrl.begin() + ind + 1 } + std::string{ s_lnk.begin() + cnt * 3, s_lnk.end() };
			}
			else if (std::regex_match(s_lnk, std::regex{ "(?:[^/]+/)+[^/]+" }) || std::regex_match(s_lnk, std::regex{ "[^/#?]+" })) // относительно дочерней директории или просто имя файла
			{
				int ind = sUrl.rfind('/');
				s_lnk = std::string{ sUrl.begin(), sUrl.begin() + ind + 1 } + s_lnk;
			}
		}
	}



std::string cleanText(std::string str) {
	std::string result = str;
	int index = result.find("<body");
	result.replace(0, index, "");

	std::regex script("<script[\\s\\S]*?>[\\s\\S]*?<\/script>");
	result = regex_replace(result, script, "");

	std::regex pattern_style("<style[^>]*?>[\\s\\S]*?<\/style>");
	result = regex_replace(result, pattern_style, " ");

	std::regex pattern_a("<a[^>]*?>[\\s\\S]*?<\/a>");
	result = regex_replace(result, pattern_a, " ");

	std::regex pattern("\\<(\/?[^\\>]+)\\>");
	result = std::regex_replace(result, pattern, " ");

	std::regex re("(\\n|\\t|[0-9]|\\s+)");
	result = std::regex_replace(result, re, " ");

	std::regex branch("[[:punct:]]");
	result = std::regex_replace(result, branch, "");

	std::regex lb("«");
	result = std::regex_replace(result, lb, "");

	std::regex rb("»");
	result = std::regex_replace(result, rb, "");

	std::regex noe("–");
	result = std::regex_replace(result, noe, "");

	std::regex noeb("—");
	result = std::regex_replace(result, noeb, "");


	boost::algorithm::to_lower(result);
	return result;
}


std::vector<std::string> wordsFromText(std::string str)
{
	std::string temp;
	std::vector<std::string> result;
    std::unordered_set<char> pattern{' '};

	const uint8_t n = 3;
	const uint8_t max = 32;
	
	for(int i = 0; i<str.size();i++)
	{
	  if(pattern.count(str.at(i)) == 1) 
	  {
		if((temp.size()>n)&&(temp.size() < max)) result.push_back(temp);
		temp = "";
	  }
	  else
	  {
		temp += str.at(i);
	  }
    }	
	
	

	if (temp.size() > 0)
	{
		result.push_back(temp);
	}
	std::cout << "Words found: " << result.size() << std::endl;

	return result;
}


std::unordered_map<std::string, uint16_t> wordsCounter(const std::vector<std::string>& words)
{
	std::unordered_map<std::string, uint16_t> series;


	for(auto& word : words)
	{
		(series.count(word) == 0)?series[word] = 1:++series[word];
	}

	return series;
}



bool isText(const boost::beast::multi_buffer::const_buffers_type& b)
{
	for (auto itr = b.begin(); itr != b.end(); itr++)
	{
		for (int i = 0; i < (*itr).size(); i++)
		{
			if (*((const char*)(*itr).data() + i) == 0)
			{
				return false;
			}
		}
	}
	return true;
}



std::string getHtmlContent(const Link& link)
{

	std::string result;

	try
	{
		std::string host = link.hostName;
		std::string query = link.query;

		net::io_context ioc;

		if (link.protocol == ProtocolType::HTTPS)
		{

			ssl::context ctx(ssl::context::tlsv13_client);
			ctx.set_default_verify_paths();

			beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
			stream.set_verify_mode(ssl::verify_none);

			stream.set_verify_callback([](bool preverified, ssl::verify_context& ctx) {
				return true; // Accept any certificate
				});


			if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
				beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
				throw beast::system_error{ ec };
			}

			ip::tcp::resolver resolver(ioc);
			get_lowest_layer(stream).connect(resolver.resolve({ host, "https" }));
			get_lowest_layer(stream).expires_after(std::chrono::seconds(30));


			http::request<http::empty_body> req{ http::verb::get, query, 11 };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			stream.handshake(ssl::stream_base::client);
			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<http::dynamic_body> res;
			http::read(stream, buffer, res);

			if (isText(res.body().data()))
			{
				result = buffers_to_string(res.body().data());
			}
			else
			{
				std::cout << "This is not a text link, bailing out..." << std::endl;
			}

			beast::error_code ec;
			stream.shutdown(ec);
			if (ec == net::error::eof) {
				ec = {};
			}

			if (ec) {
				throw beast::system_error{ ec };
			}
		}
		else
		{
			tcp::resolver resolver(ioc);
			beast::tcp_stream stream(ioc);

			auto const results = resolver.resolve(host, "http");

			stream.connect(results);

			http::request<http::string_body> req{ http::verb::get, query, 11 };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);


			http::write(stream, req);

			beast::flat_buffer buffer;

			http::response<http::dynamic_body> res;


			http::read(stream, buffer, res);

			if (isText(res.body().data()))
			{
				result = buffers_to_string(res.body().data());
			}
			else
			{
				std::cout << "This is not a text link, bailing out..." << std::endl;
			}

			beast::error_code ec;
			stream.socket().shutdown(tcp::socket::shutdown_both, ec);

			if (ec && ec != beast::errc::not_connected)
				throw beast::system_error{ ec };
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return result;
}
