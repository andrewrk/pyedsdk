#include "Utils.h"
#include <sstream>

int Utils::stringToInt(std::string value)
{
    std::stringstream ss;
    ss << value;
    int out;
    ss >> out;
    return out;
}

std::string Utils::intToString(int value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}


