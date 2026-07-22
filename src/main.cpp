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

string VERSION = "v1.0 - stable";
string PRODUCT = "QuickServe by MM - " + VERSION;

int main(int argc, char *argv[]) {
  sPrint(PRODUCT);
  sPrint("------------------------------");

  path dir = current_path();

  int port = 8080;

  string iface = "all";
  string password = "";
  string user = "user0";

  int argsOk = prepArgs(argc, argv, dir, port, iface, password, user);

  if (argsOk != 0) {
    if (argsOk == -2) {
      return 0;
    }

    return argsOk;
  }

  if (port < 1 || port > 65535) {
    sPrint("Error: port must be between 1 and 65535", true);
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
  sPrint("Interface: " + iface);
  sPrint("Port: " + to_string(port));

  return startServer(dir, port, iface, password, user);
}
