#include "stdafx.h"

#include "CountersClient.h"
#include "windows.h"
#include "wininet.h"
#include "atlutil.h"
#include "RobloxServicesTools.h"

std::set<std::wstring> CountersClient::_events;

CountersClient::CountersClient(std::string baseUrl, std::string key, simple_logger<wchar_t>* logger) :
_logger(logger)
{

}

void CountersClient::registerEvent(std::wstring eventName, bool fireImmediately)
{

}

void CountersClient::reportEvents()
{

}

void CountersClient::reportEvents(std::set<std::wstring> &events)
{

}
