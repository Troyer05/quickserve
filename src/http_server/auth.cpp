#include "../helpers.h"
#include "http_server.h"
#include <iostream>
#include <string>

using namespace std;

bool isAuthorized(const string &request, const string &password,
                  const string &user) {
  if (password.empty()) {
    return true;
  }

  string authorization = getHeaderValue(request, "Authorization");
  string expected = "Basic " + base64Encode(user + ":" + password);

  return authorization == expected;
}
