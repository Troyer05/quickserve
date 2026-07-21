#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
using namespace std::filesystem;

void sPrint(string text, bool err = false) {
  if (err) {
    cerr << text << endl;
  } else {
    cout << text << endl;
  }
}

bool sendAll(int fd, const string &data) {
  size_t totalSent = 0;
  size_t dataSize = data.size();

  while (totalSent < dataSize) {
    ssize_t sent = write(fd, data.c_str() + totalSent, dataSize - totalSent);

    if (sent < 0) {
      return false;
    }

    totalSent += sent;
  }

  return true;
}

bool isInsideDir(const path &root, const path &target) {
  auto rootPath = root.begin();
  auto targetPath = target.begin();

  while (rootPath != root.end()) {
    if (targetPath == target.end() || *rootPath != *targetPath) {
      return false;
    }

    ++rootPath;
    ++targetPath;
  }

  return true;
}
