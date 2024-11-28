#include "util.h"

using namespace std;

bool is_safe(uint8_t b) {
    return b >= ' ' && b < 128;
}

string hexdump(const void *buf, size_t len) {
    string ret("\r\n");
    char tmp[8];
    const uint8_t *data = (const uint8_t *) buf;
    for (size_t i = 0; i < len; i += 16) {
        for (int j = 0; j < 16; ++j) {
            if (i + j < len) {
                int sz = snprintf(tmp, sizeof(tmp), "%.2x ", data[i + j]);
                ret.append(tmp, sz);
            } else {
                int sz = snprintf(tmp, sizeof(tmp), "   ");
                ret.append(tmp, sz);
            }
        }
        for (int j = 0; j < 16; ++j) {
            if (i + j < len) {
                ret += (is_safe(data[i + j]) ? data[i + j] : '.');
            } else {
                ret += (' ');
            }
        }
        ret += ('\n');
    }
    return ret;
}

string hexmem(const void *buf, size_t len) {
    string ret;
    char tmp[8];
    const uint8_t *data = (const uint8_t *) buf;
    for (size_t i = 0; i < len; ++i) {
        int sz = sprintf(tmp, "%.2x ", data[i]);
        ret.append(tmp, sz);
    }
    return ret;
}

string exePath(bool isExe /*= true*/) {
    char buffer[PATH_MAX * 2 + 1] = {0};
    int n = -1;
#if defined(_WIN32)
    n = GetModuleFileNameA(isExe?nullptr:(HINSTANCE)&__ImageBase, buffer, sizeof(buffer));
#elif defined(__MACH__) || defined(__APPLE__)
    n = sizeof(buffer);
    if (uv_exepath(buffer, &n) != 0) {
        n = -1;
    }
#elif defined(__linux__)
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif

    string filePath;
    if (n <= 0) {
        filePath = "./";
    } else {
        filePath = buffer;
    }

#if defined(_WIN32)
    //windows下把路径统一转换层unix风格，因为后续都是按照unix风格处理的
    for (auto &ch : filePath) {
        if (ch == '\\') {
            ch = '/';
        }
    }
#endif //defined(_WIN32)

    return filePath;
}

string exeDir(bool isExe /*= true*/) {
    auto path = exePath(isExe);
    return path.substr(0, path.rfind('/') + 1);
}

string exeName(bool isExe /*= true*/) {
    auto path = exePath(isExe);
    return path.substr(path.rfind('/') + 1);
}

// string转小写
std::string &strToLower(std::string &str) {
    transform(str.begin(), str.end(), str.begin(), towlower);
    return str;
}

// string转大写
std::string &strToUpper(std::string &str) {
    transform(str.begin(), str.end(), str.begin(), towupper);
    return str;
}

// string转小写
std::string strToLower(std::string &&str) {
    transform(str.begin(), str.end(), str.begin(), towlower);
    return std::move(str);
}

// string转大写
std::string strToUpper(std::string &&str) {
    transform(str.begin(), str.end(), str.begin(), towupper);
    return std::move(str);
}

vector<string> split(const string &s, const char *delim) {
    vector<string> ret;
    size_t last = 0;
    auto index = s.find(delim, last);
    while (index != string::npos) {
        if (index - last > 0) {
            ret.push_back(s.substr(last, index - last));
        }
        last = index + strlen(delim);
        index = s.find(delim, last);
    }
    if (!s.size() || s.size() - last > 0) {
        ret.push_back(s.substr(last));
    }
    return ret;
}

#define TRIM(s, chars) \
do{ \
    string map(0xFF, '\0'); \
    for (auto &ch : chars) { \
        map[(unsigned char &)ch] = '\1'; \
    } \
    while( s.size() && map.at((unsigned char &)s.back())) s.pop_back(); \
    while( s.size() && map.at((unsigned char &)s.front())) s.erase(0,1); \
}while(0);

//去除前后的空格、回车符、制表符
std::string &trim(std::string &s, const string &chars) {
    TRIM(s, chars);
    return s;
}

std::string trim(std::string &&s, const string &chars) {
    TRIM(s, chars);
    return std::move(s);
}

void replace(string &str, const string &old_str, const string &new_str,std::string::size_type b_pos) {
    if (old_str.empty() || old_str == new_str) {
        return;
    }
    auto pos = str.find(old_str,b_pos);
    if (pos == string::npos) {
        return;
    }
    str.replace(pos, old_str.size(), new_str);
    replace(str, old_str, new_str,pos + new_str.length());
}

bool start_with(const string &str, const string &substr) {
    return str.find(substr) == 0;
}

bool end_with(const string &str, const string &substr) {
    auto pos = str.rfind(substr);
    return pos != string::npos && pos == str.size() - substr.size();
}

bool fileExists(const std::string& filename) {
    if (access(filename.c_str(), F_OK) == -1) {
        return false; 
    }
    return true;
}

bool removeFile(const std::string& filename) {
    if (remove(filename.c_str()) != 0) {
        perror("remove");
        return false;
    }
    return true;
}
