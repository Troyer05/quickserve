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

#include "../helpers.h"

using namespace std;
using namespace std::filesystem;

int startServer(path dir, int port) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    sPrint("Error: failed to create socket", true);
    return 1;
  }

  struct sockaddr_in server_addr{};

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  int reuse = 1;

  int socketOpt =
      setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  if (socketOpt < 0) {
    sPrint("Error: failed to set socket options", true);
    close(server_fd);

    return 1;
  }

  int bindRes =
      bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  if (bindRes < 0) {
    sPrint("Error: failed to bind socket", true);
    close(server_fd);

    return 1;
  }

  int listenRes = listen(server_fd, 10);

  if (listenRes < 0) {
    sPrint("Error: failed to listen on socket", true);
    close(server_fd);

    return 1;
  }

  ifaddrs *interfaces = nullptr;

  if (getifaddrs(&interfaces) == -1) {
    sPrint("Error: failed to get interfaces", true);
    close(server_fd);

    return 1;
  }

  for (ifaddrs *entry = interfaces; entry != nullptr; entry = entry->ifa_next) {
    if (entry->ifa_addr == nullptr) {
      continue;
    }

    if (entry->ifa_addr->sa_family != AF_INET) {
      continue;
    }

    sockaddr_in *address = reinterpret_cast<sockaddr_in *>(entry->ifa_addr);

    char ip[INET_ADDRSTRLEN];

    if (inet_ntop(AF_INET, &address->sin_addr, ip, sizeof(ip)) == nullptr) {
      continue;
    }

    if (entry->ifa_flags & IFF_LOOPBACK) {
      continue;
    }

    sPrint("Available at: http://" + string(ip) + ":" + to_string(port));
  }

  freeifaddrs(interfaces);

  while (true) {
    struct sockaddr_in client_addr{};

    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd < 0) {
      sPrint("Error: failed to accept client connection", true);
      continue;
    }

    char buffer[1024];
    int bytes_read = read(client_fd, buffer, sizeof(buffer));

    if (bytes_read <= 0) {
      sPrint("Error: failed to read from client", true);
      close(client_fd);

      continue;
    }

    string request(buffer, bytes_read);

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
    path requestedPath = dir / relPath;

    requestedPath = weakly_canonical(requestedPath);

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
    } else if (httpPath == "/" && method == "GET") {
      string body = "<center><h1><u>Hi! There they are:</u></h1></center><br>";
      string f_tmp;

      for (const auto &entry : directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
          continue;
        }

        f_tmp = entry.path().filename().string();

        body += "<li>";
        body += "<a href=\"" + f_tmp + "\" download=\"" + f_tmp + "\">";
        body += f_tmp;
        body += "</a></li>";
      }

      resp = "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=utf-8\r\n"
             "Content-Length: " +
             to_string(body.size()) +
             "\r\n"
             "Connection: close\r\n"
             "\r\n" +
             body;

      sPrint(method + " " + httpPath + " " + version);
    } else if (method == "GET" && httpPath != "/") {
      if (exists(requestedPath) && is_regular_file(requestedPath)) {
        string body = "File found: " + requestedPath.filename().string() + "\n";

        resp = "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/plain; charset=utf-8\r\n"
               "Content-Length: " +
               to_string(body.size()) +
               "\r\n"
               "Connection: close\r\n"
               "\r\n" +
               body;

        sPrint(method + " " + httpPath + " " +
               requestedPath.filename().string() + " " + version + " -> 200");
      } else {
        string body = "Oh noooo, this site does not exists :(\n";

        resp = "HTTP/1.1 404 NOT FOUND\r\n"
               "Content-Type: text/plain; charset=utf-8\r\n"
               "Content-Length: " +
               to_string(body.size()) +
               "\r\n"
               "Connection: close\r\n"
               "\r\n" +
               body;

        sPrint(method + " " + httpPath + " " + version + " -> 404", true);
      }
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
    }

    bool sent = sendAll(client_fd, resp);

    if (!sent) {
      sPrint("Error: failed to write to client", true);
    }

    close(client_fd);
  }

  close(server_fd);
}
