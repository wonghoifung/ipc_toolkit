#pragma once

#include "base.h"

std::string hexdump(const void *buf, size_t len);
std::string hexmem(const void* buf, size_t len);
std::string exePath(bool isExe = true);
std::string exeDir(bool isExe = true);
std::string exeName(bool isExe = true);

std::vector<std::string> split(const std::string& s, const char *delim);
//去除前后的空格、回车符、制表符...
std::string& trim(std::string &s,const std::string &chars=" \r\n\t");
std::string trim(std::string &&s,const std::string &chars=" \r\n\t");
// string转小写
std::string &strToLower(std::string &str);
std::string strToLower(std::string &&str);
// string转大写
std::string &strToUpper(std::string &str);
std::string strToUpper(std::string &&str);
//替换子字符串
void replace(std::string &str, const std::string &old_str, const std::string &new_str, std::string::size_type b_pos = 0) ;

//字符串是否以xx开头
bool start_with(const std::string &str, const std::string &substr);
//字符串是否以xx结尾
bool end_with(const std::string &str, const std::string &substr);
 
bool fileExists(const std::string& filename);
bool removeFile(const std::string& filename);
