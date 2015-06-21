#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED

#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <locale>
#include <sstream>
#include <cstdlib>
#include <vector>

std::string string_strip(const std::string &str) {
    std::locale loc;
    
    std::string s = str;
    while (s.size() != 0 && std::isspace(s[0], loc)) {
        s.erase(s.begin());
    }
    while (s.size() != 0 && std::isspace(*s.rbegin(), loc)) {
        s.resize(s.size() - 1);
    }
    return s;
}

std::map<std::string, std::string> get_config(const char *filename)
{
    std::map<std::string, std::string> res;
    
    std::ifstream fin(filename);
    
    std::string s;
    int line_num = 0;
    while (getline(fin, s)) {
        line_num++;
        s = s.substr(0, s.find('#'));
        s = string_strip(s);
        if (s.size() == 0) {
            continue;
        }
        if (s.find('=') != s.rfind('=')) {
            std::cerr << "Warning: config bad line " << line_num << ": too many '=' characters.";
            continue;
        }
        if (s.find('=') == s.npos) {
            std::cerr << "Warning: config bad line " << line_num << ": no '=' characters.";
            continue;
        }
        int pos = s.find('=');
        res[string_strip(s.substr(0, pos))] = string_strip(s.substr(pos + 1));
    }
    return res;
}

std::vector<std::string> get_tokens(const std::string &str)
{
    std::vector<std::string> res;
    std::string cur;
    std::locale loc;
    for (int i = 0; i <= (int) str.length(); ++i) {
        if (i == (int) str.length() || isspace(str[i], loc)) {
            while (cur.length() && ispunct(cur[(int) cur.length() - 1], loc)) {
                cur.erase(cur.length() - 1, 1);
            }
            while (cur.length() && ispunct(cur[0], loc)) {
                cur.erase(0, 1);
            }
            if (cur != "") {
                res.push_back(cur);
                cur = "";
            }
        } else {
            cur.push_back(str[i]);
        }
    }
    return res;
}

int string_to_int(const std::string &str)
{
    return strtol(str.c_str(), NULL, 10);
}

long double string_to_ldouble(const std::string &str)
{
    std::stringstream ss(str);
    long double a;
    ss >> a;
    return a;
}

double rand_double()
{
    return double(rand()) / RAND_MAX;
}

#endif