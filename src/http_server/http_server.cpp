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

#include "../helpers.h"
#include "auth.h"

using namespace std;
using namespace std::filesystem;

string getHeaderValue(const string &request, const string &headerName) {
  istringstream stream(request);
  string line;
  string prefix = headerName + ":";

  while (getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    if (line.rfind(prefix, 0) != 0) {
      continue;
    }

    string value = line.substr(prefix.size());

    while (!value.empty() && value.front() == ' ') {
      value.erase(value.begin());
    }

    return value;
  }

  return "";
}

int startServer(const path &dir, int port, const string &iface,
                const string &password, const string &user) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    sPrint("Error: failed to create socket", true);
    return 1;
  }

  sockaddr_in server_addr{};

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  ifaddrs *interfaces = nullptr;

  if (getifaddrs(&interfaces) == -1) {
    sPrint("Error: failed to get interfaces", true);
    close(server_fd);

    return 1;
  }

  bool ifaceFound = iface == "all";
  string selectedIp;

  if (iface == "all") {
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    for (ifaddrs *entry = interfaces; entry != nullptr;
         entry = entry->ifa_next) {
      if (entry->ifa_addr == nullptr) {
        continue;
      }

      if (entry->ifa_addr->sa_family != AF_INET) {
        continue;
      }

      if (iface != entry->ifa_name) {
        continue;
      }

      sockaddr_in *address = reinterpret_cast<sockaddr_in *>(entry->ifa_addr);

      char ip[INET_ADDRSTRLEN];

      if (inet_ntop(AF_INET, &address->sin_addr, ip, sizeof(ip)) == nullptr) {
        continue;
      }

      server_addr.sin_addr = address->sin_addr;
      selectedIp = ip;
      ifaceFound = true;

      break;
    }
  }

  if (!ifaceFound) {
    sPrint("Error: interface not found or has no IPv4 address: " + iface, true);

    freeifaddrs(interfaces);
    close(server_fd);

    return 1;
  }

  int reuse = 1;

  int socketOpt =
      setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  if (socketOpt < 0) {
    sPrint("Error: failed to set socket options", true);

    freeifaddrs(interfaces);
    close(server_fd);

    return 1;
  }

  int bindRes = bind(server_fd, reinterpret_cast<sockaddr *>(&server_addr),
                     sizeof(server_addr));

  if (bindRes < 0) {
    sPrint("Error: failed to bind socket", true);

    freeifaddrs(interfaces);
    close(server_fd);

    return 1;
  }

  int listenRes = listen(server_fd, 10);

  if (listenRes < 0) {
    sPrint("Error: failed to listen on socket", true);

    freeifaddrs(interfaces);
    close(server_fd);

    return 1;
  }

  if (iface == "all") {
    for (ifaddrs *entry = interfaces; entry != nullptr;
         entry = entry->ifa_next) {
      if (entry->ifa_addr == nullptr) {
        continue;
      }

      if (entry->ifa_addr->sa_family != AF_INET) {
        continue;
      }

      if (entry->ifa_flags & IFF_LOOPBACK) {
        continue;
      }

      sockaddr_in *address = reinterpret_cast<sockaddr_in *>(entry->ifa_addr);

      char ip[INET_ADDRSTRLEN];

      if (inet_ntop(AF_INET, &address->sin_addr, ip, sizeof(ip)) == nullptr) {
        continue;
      }

      sPrint("Available at: http://" + string(ip) + ":" + to_string(port));
    }
  } else {
    sPrint("Available at: http://" + selectedIp + ":" + to_string(port));
  }

  freeifaddrs(interfaces);

  while (true) {
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd =
        accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr),
               &client_addr_len);

    if (client_fd < 0) {
      sPrint("Error: failed to accept client connection", true);
      continue;
    }

    char buffer[1024];

    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));

    if (bytes_read <= 0) {
      sPrint("Error: failed to read from client", true);
      close(client_fd);

      continue;
    }

    string request(buffer, static_cast<size_t>(bytes_read));
    istringstream requestStream(request);

    string method;
    string httpPath;
    string version;
    string resp;

    if (!(requestStream >> method >> httpPath >> version)) {
      sPrint("Error: failed to parse request", true);
      close(client_fd);

      continue;
    }

    string relPath = httpPath.substr(1);
    path requestedPath = weakly_canonical(dir / relPath);

    sPrint("Requested HTTP Path: " + requestedPath.string());

    if (!isInsideDir(dir, requestedPath)) {
      string body = "No no no, you are not allowed to access this file >:(\n";

      resp = "HTTP/1.1 403 Forbidden\r\n"
             "Content-Type: text/plain; charset=utf-8\r\n"
             "Content-Length: " +
             to_string(body.size()) +
             "\r\n"
             "Connection: close\r\n"
             "\r\n" +
             body;

      sPrint(method + " " + httpPath + " " + version + " -> 403", true);
    } else if (method != "GET") {
      string body = "Method not allowed\n";

      resp = "HTTP/1.1 405 Method Not Allowed\r\n"
             "Content-Type: text/plain; charset=utf-8\r\n"
             "Content-Length: " +
             to_string(body.size()) +
             "\r\n"
             "Allow: GET\r\n"
             "Connection: close\r\n"
             "\r\n" +
             body;

      sPrint(method + " " + httpPath + " " + version + " -> 405", true);
    } else if (httpPath == "/") {
      if (!isAuthorized(request, password, user)) {
        string body = "Authentication required\n";

        resp = "HTTP/1.1 401 Unauthorized\r\n"
               "WWW-Authenticate: Basic realm=\"QuickServe\"\r\n"
               "Content-Type: text/plain; charset=utf-8\r\n"
               "Content-Length: " +
               to_string(body.size()) +
               "\r\n"
               "Connection: close\r\n"
               "\r\n" +
               body;

        sPrint(method + " " + httpPath + " " + version + " -> 401", true);
      } else {
        string body =
            "<center><h1><u>Hi! There they are:</u></h1></center><br>";

        for (const auto &entry : directory_iterator(dir)) {
          if (!entry.is_regular_file()) {
            continue;
          }

          string filename = entry.path().filename().string();

          body += "<li>";
          body += "<a href=\"" + filename + "\" download=\"" + filename + "\">";
          body += filename;
          body += "</a>";
          body += "</li>";
        }

        resp = "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html; charset=utf-8\r\n"
               "Content-Length: " +
               to_string(body.size()) +
               "\r\n"
               "Connection: close\r\n"
               "\r\n" +
               body;

        sPrint(method + " " + httpPath + " " + version + " -> 200");
      }
    } else if (exists(requestedPath) && is_regular_file(requestedPath)) {
      string body = "File found: " + requestedPath.filename().string() + "\n";

      resp = "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/plain; charset=utf-8\r\n"
             "Content-Length: " +
             to_string(body.size()) +
             "\r\n"
             "Connection: close\r\n"
             "\r\n" +
             body;

      sPrint(method + " " + httpPath + " " + requestedPath.filename().string() +
             " " + version + " -> 200");
    } else {
      string body = "Oh noooo, this site does not exist :(\n";

      resp = "HTTP/1.1 404 Not Found\r\n"
             "Content-Type: text/plain; charset=utf-8\r\n"
             "Content-Length: " +
             to_string(body.size()) +
             "\r\n"
             "Connection: close\r\n"
             "\r\n" +
             body;

      sPrint(method + " " + httpPath + " " + version + " -> 404", true);
    }

    if (!sendAll(client_fd, resp)) {
      sPrint("Error: failed to write to client", true);
    }

    close(client_fd);
  }

  close(server_fd);

  return 0;
}
