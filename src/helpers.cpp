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

int prepArgs(int argc, char *argv[], path &dir, int &port, string &iface,
             string &password, string &user) {
  for (int i = 1; i < argc; i++) {
    string arg = argv[i];

    if (arg == "--dir" || arg == "-d") {
      if (i + 1 >= argc) {
        sPrint("Error: missing directory after " + arg, true);
        return 1;
      }

      dir = argv[++i];
    } else if (arg == "--port" || arg == "-p") {
      if (i + 1 >= argc) {
        sPrint("Error: missing port after " + arg, true);
        return 1;
      }

      string portValue = argv[++i];

      try {
        size_t parsedLength = 0;

        port = stoi(portValue, &parsedLength);

        if (parsedLength != portValue.size()) {
          sPrint("Error: invalid port: " + portValue, true);
          return 1;
        }
      } catch (const exception &) {
        sPrint("Error: invalid port: " + portValue, true);
        return 1;
      }
    } else if (arg == "--iface" || arg == "-i") {
      if (i + 1 >= argc) {
        sPrint("Error: missing interface after " + arg, true);
        return 1;
      }

      iface = argv[++i];
    } else if (arg == "--help" || arg == "-h" || arg == "/?" || arg == "-?" ||
               arg == "--?") {
      sPrint("Usage:");
      sPrint(
          "  " + string(argv[0]) +
          " [--dir|-d directory] [--port|-p port] "
          "[--iface|-i interface] [--password|-pw password] [--user|-u user]");
      sPrint("");
      sPrint("Defaults:");
      sPrint("  directory: current directory");
      sPrint("  port:      8080");
      sPrint("  interface: all");
      sPrint("  password:  none");
      sPrint("  user:      none / user0 when password set");
      sPrint("");
      sPrint("Examples:");
      sPrint("  " + string(argv[0]) +
             " --dir /var/www --port 8080 --iface 192.168.1.1 --password "
             "mypassword");
      sPrint("  " + string(argv[0]) +
             " -d /var/www -p 8080 -i 192.168.1.1 -pw mypassword");
      sPrint("  " + string(argv[0]) +
             " -d /var/www -p 8080 -i 192.168.1.1 -pw mypassword -u myuser");

      return -2;
    } else if (arg == "--password" || arg == "-pw") {
      if (i + 1 >= argc) {
        sPrint("Error: missing password after " + arg, true);
        return 1;
      }

      password = argv[++i];
    } else if (arg == "--user" || arg == "-u") {
      if (i + 1 >= argc) {
        sPrint("Error: missing user after " + arg, true);
        return 1;
      }

      user = argv[++i];
    } else {
      sPrint("Error: unknown argument: " + arg, true);
      sPrint("Use --help or -h or -? or --? or /? for usage information.",
             true);

      return 1;
    }
  }

  return 0;
}

string base64Encode(const string &input) {
  static const string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz"
                              "0123456789+/";

  string output;
  int value = 0;
  int bits = -6;

  for (unsigned char character : input) {
    value = (value << 8) + character;
    bits += 8;

    while (bits >= 0) {
      output.push_back(chars[(value >> bits) & 0x3F]);
      bits -= 6;
    }
  }

  if (bits > -6) {
    output.push_back(chars[((value << 8) >> (bits + 8)) & 0x3F]);
  }

  while (output.size() % 4 != 0) {
    output.push_back('=');
  }

  return output;
}
