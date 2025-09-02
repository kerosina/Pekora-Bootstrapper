#include "string"
#include "vector"
#include "stdafx.h"
#include "TrustCheck.h"

bool TrustCheck::trustCheck(std::string clientVersion)
{
    std::vector<std::string> allowedVersions;
    
    // Always allow 2016E
    allowedVersions.push_back("2016E");
    
#if VER_2017
    allowedVersions.push_back("2017L");
#endif
#if VER_2018
    allowedVersions.push_back("2018L");
#endif
#if VER_2020
    allowedVersions.push_back("2020L");
#endif
#if VER_2021
    allowedVersions.push_back("2021M");
#endif
    
    auto it = std::find(allowedVersions.begin(), allowedVersions.end(), clientVersion);
    return it != allowedVersions.end();
}