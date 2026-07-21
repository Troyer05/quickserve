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

#include "helpers.h"
#include "http_server/http_server.h"

using namespace std;
using namespace std::filesystem;

string VERSION = "v0.5 - beta";
string PRODUCT = "QuickServe by MM - " + VERSION;

int main(int argc, char *argv[]) {
  sPrint(PRODUCT);
  sPrint("------------------------------");

  path dir;
  int port = 8080;

  if (argc == 1) {
    dir = current_path();
  } else if (argc == 2) {
    dir = argv[1];
  } else if (argc == 3) {
    dir = argv[1];
    port = atoi(argv[2]);
  } else {
    sPrint("Error: too many arguments", true);
    return 1;
  }

  if (!exists(dir)) {
    sPrint("Error: " + dir.string() + " does not exist", true);
    return 1;
  }

  if (!is_directory(dir)) {
    sPrint("Error: " + dir.string() + " is not a directory", true);
    return 1;
  }

  dir = canonical(dir);

  sPrint("Serving directory: " + dir.string());

  startServer(dir, port);

  return 0;
}
