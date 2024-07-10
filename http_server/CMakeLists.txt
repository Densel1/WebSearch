cmake_minimum_required(VERSION 3.20)

project(HttpServerApp)


include_directories(../spider/boost_1_85_0 )

add_executable(HttpServerApp 
	main.cpp
	http_connection.h
	http_connection.cpp
	../spider/iniParser.h
	../spider/iniParser.cpp
	)

find_package(OpenSSL REQUIRED)

target_compile_features(HttpServerApp PRIVATE cxx_std_17) 

target_link_libraries(HttpServerApp pqxx)  

add_subdirectory(C:\\libpqxx-7.8.1 libpqxx-build)

target_link_libraries(HttpServerApp OpenSSL::SSL)
