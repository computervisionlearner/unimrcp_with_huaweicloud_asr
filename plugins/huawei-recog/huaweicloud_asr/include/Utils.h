#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <string>
#include <ctime>
#include <iostream>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <iterator>
#include <sstream>


namespace speech {
namespace huawei_asr {

namespace defaults {
static const std::string sdk_signing_algorithm = "SDK-HMAC-SHA256";
static const std::string dateFormat = "X-Sdk-Date";
static const std::string host = "Host";
}

enum WebsocketStatus {
    WB_PRE_START, WB_START, WB_END, WB_BLOCKING, WB_CONNECT, WB_CLOSE, WB_ERROR
};

std::string trim(std::string str);
std::string toLowerCaseStr(std::string str);
std::string toISO8601Time(std::time_t& time);
std::string uriDecode(std::string& str);
std::string uriEncode(std::string& str, bool path=false);

int ReadBinary(const std::string& filePath, std::string& audioContent);
std::string WebsocketStatusToStr(WebsocketStatus status);
}
}



#endif /* SRC_UTILS_H_ */
