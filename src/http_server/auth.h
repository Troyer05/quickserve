#ifndef AUTH_H
#define AUTH_H

#include <string>

using namespace std;

bool isAuthorized(const string &request, const string &password,
                  const string &user);

#endif
