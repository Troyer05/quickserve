#ifndef HELPERS_H
#define HELPERS_H

#include <filesystem>
#include <iostream>
#include <string>

using namespace std;
using namespace std::filesystem;

void sPrint(string text, bool err = false);
bool sendAll(int fd, const string &data);
bool isInsideDir(const path &root, const path &target);
int prepArgs(int argc, char *argv[], path &dir, int &port, string &iface,
             string &password, string &user);
string base64Encode(const string &input);

#endif
