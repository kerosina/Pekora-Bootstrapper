#include "stdafx.h"
#include "GoogleAnalyticsHelper.h"
#include "HttpTools.h"
#include <sstream>
#include "windows.h"
#include "wininet.h"
#include "atlutil.h"
#include "SharedHelpers.h"

#include <iphlpapi.h>

namespace
{
	static bool initialized = false;
	static std::string googleAccountPropertyID;
	static std::string googleClientID;

}

namespace GoogleAnalyticsHelper
{
	// remove analytics
	void init(const std::string &accountPropertyID)
	{

	}

	bool trackEvent(simple_logger<wchar_t> &logger, const char *category, const char *action, const char *label, int value)
	{
		return true;
	}

	bool trackUserTiming(simple_logger<wchar_t> &logger, const char *category, const char *variable, int milliseconds, const char *label)
	{
		return true;
	}
}