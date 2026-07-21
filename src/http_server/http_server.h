#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <filesystem>
#include <iostream>
#include <string>

using namespace std;
using namespace std::filesystem;

int startServer(path dir, int port);

#endif
