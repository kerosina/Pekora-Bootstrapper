#include "stdafx.h"

#include <sstream>
#include <stdexcept>

#include "InfluxDbHelper.h"
#include "format_string.h"

// appended to all reports
std::string reporter;
std::string location;
std::string appVersion;

std::string hostName;
std::string database;
std::string user;
std::string password;
bool canSend = false;
std::string url;
std::string path;

void InfluxDb::init(const std::string& _reporter, const std::string& _url, const std::string& _database, const std::string& _user, const std::string& _pw)
{

}

void InfluxDb::setLocation(const std::string& loc)
{
	location = loc;
}

void InfluxDb::setAppVersion(const std::string& ver)
{
	appVersion = ver;
}

const std::string& InfluxDb::getReportingUrl()
{
	return url;
}

const std::string& InfluxDb::getUrlHost()
{
	return hostName;
}

const std::string& InfluxDb::getUrlPath()
{
	return path;
}

void InfluxDb::addPoint(const std::string& name, const rapidjson::Value& value)
{
	
}


