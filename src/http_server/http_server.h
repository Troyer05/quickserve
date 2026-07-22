#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <filesystem>
#include <iostream>
#include <string>

using namespace std;
using namespace std::filesystem;

int startServer(const path &dir, int port, const string &iface,
                const string &password, const string &user);
string getHeaderValue(const string &request, const string &header);

#endif
