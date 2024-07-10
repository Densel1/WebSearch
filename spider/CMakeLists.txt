
cmake_minimum_required(VERSION 3.22.0) 

project(spider)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(OPENSSL_USE_STATIC_LIBS TRUE)

include_directories(boost_1_85_0 )
include_directories(C:/openssl/include)
include_directories(C:/vcpkg/vcpkg)

add_compile_definitions(_WIN32_WINNT=0x0A01) #for windows10

find_package(OpenSSL REQUIRED)
find_package(unofficial-gumbo CONFIG REQUIRED)
add_executable(spider iniParser.cpp iniParser.h main.cpp MyException.h MyException.cpp table.h table.cpp link.h http_utils.h http_utils.cpp)
add_subdirectory(C:\\libpqxx-7.8.1 libpqxx-build)
target_compile_features(spider PRIVATE cxx_std_17)
target_link_libraries(spider PRIVATE OpenSSL::SSL OpenSSL::Crypto pqxx unofficial::gumbo::gumbo)