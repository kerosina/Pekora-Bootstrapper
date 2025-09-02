#include "stdafx.h"
#include "HttpTools.h"
#include "SharedHelpers.h"
#include "atlutil.h"
#include "wininet.h"
#include <strstream>
#pragma comment (lib, "Wininet.lib")

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <cpr/cpr.h>
#include <iostream>

static const std::string sContentLength = "content-length: ";
static const std::string sEtag = "etag: ";

namespace HttpTools
{
	class WININETHINTERNET : boost::noncopyable
	{
		HINTERNET handle;
	public:
		WININETHINTERNET(HINTERNET handle):handle(handle) {}
		WININETHINTERNET():handle(0) {}
		WININETHINTERNET& operator = (HINTERNET handle)
		{
			::InternetCloseHandle(this->handle);
			this->handle = handle;
			return *this;
		}
		operator bool() { return handle!=0; }
		operator HINTERNET() { return handle; }
		~WININETHINTERNET()
		{
			::InternetCloseHandle(handle);
		}
	};

	int httpWinInet(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress);
	int http(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log = true);

	static void dummyProgress(int, int) {}
	const std::string getPrimaryCdnHost(IInstallerSite *site)
	{
		static std::string cdnHost;
		static bool cdnHostLoaded = false;
		static bool validCdnHost = false;

		if(!cdnHostLoaded)
		{
			if (site->ReplaceCdnTxt()) {
				std::string host = site->InstallHost();
				std::string prod ("setup.pekora.org/cdn");
				if (host.compare(prod) == 0) {
					cdnHost = "setup.pekora.org/cdn";
					validCdnHost = true;
					LLOG_ENTRY1(site->Logger(), "primaryCdn: %s", cdnHost.c_str());
				} else {
					cdnHost = host;
					validCdnHost = true;
					LLOG_ENTRY1(site->Logger(), "primaryCdn: %s", cdnHost.c_str());
				}
			} else {
				try
				{
					std::ostrstream result;
					int status_code;
					std::string eTag;
					switch(status_code = httpGet(site, site->InstallHost(), "/cdn.txt", eTag, result, false, boost::bind(&dummyProgress, _1, _2)))
					{
						case 200:
						case 304:
							result << (char) 0;
							cdnHost = result.str();
							LLOG_ENTRY1(site->Logger(), "primaryCdn: %s", cdnHost.c_str());
							validCdnHost = true;
							break;
						default:
							validCdnHost = false;
							LLOG_ENTRY1(site->Logger(), "primaryCdn failure code=%d, falling back to secondary installHost", status_code);
							break;
					}
				}
				catch(std::exception&)
				{
					//Quash exceptions, set validCdnHost to false
					validCdnHost = false;
					LLOG_ENTRY(site->Logger(), "primaryCdn exception, falling back to secondary installHost");
				}
			}
			
			//Only try to load the CDN once, then give up
			cdnHostLoaded = true;
		}

		return cdnHost;
	}

	const std::string getCdnHost(IInstallerSite *site)
	{
		static std::string cdnHost;
		static bool cdnHostLoaded = false;
		static bool validCdnHost = false;
		if(!cdnHostLoaded)
		{
			try
			{
				std::ostrstream result;
				std::string eTag;
				int statusCode = httpGet(site, site->BaseHost(), "/install/GetInstallerCdns.ashx", eTag, result, false, boost::bind(&dummyProgress, _1, _2));
				switch(statusCode)
				{
				case 200:
				case 304:
				{
					result << (char) 0;
					LLOG_ENTRY1(site->Logger(), "primaryCdns: %s", result.str());

					std::stringstream stream(result.str());
					boost::property_tree::ptree ptree;
					boost::property_tree::json_parser::read_json(stream, ptree);

					std::vector<std::pair<std::string, int>> cdns;
					int totalValue = 0;
					BOOST_FOREACH(const boost::property_tree::ptree::value_type& child, ptree.get_child(""))
					{
						int value = boost::lexical_cast<int>(child.second.data());
						if (cdns.size() > 0)
							cdns.push_back(std::make_pair(child.first.data(), cdns.back().second + value));
						else
							cdns.push_back(std::make_pair(child.first.data(), boost::lexical_cast<int>(child.second.data())));

						totalValue += value;
					}

					if (cdns.size() && (totalValue > 0))
					{
						// randomly pick a cdn
						int r = rand() % totalValue;
						for (unsigned int i = 0; i < cdns.size(); i++)
						{
							if (r < cdns[i].second)
							{
								cdnHost = cdns[i].first;
								validCdnHost = true;
								break;
							}
						}
					}

					break;
				}
				default:
					validCdnHost = false;
					LLOG_ENTRY1(site->Logger(), "primaryCdn failure code=%d, falling back to secondary installHost", statusCode);
					break;
				}
			}
			catch(std::exception& e)
			{
				LLOG_ENTRY(site->Logger(), "primaryCdn exception, falling back to secondary installHost");
			}
			//Only try to load the CDN once, then give up
			cdnHostLoaded = true;
		}

		return cdnHost;
	}

	using boost::asio::ip::tcp;

	class CInternet : boost::noncopyable
	{
		HINTERNET handle;
	public:
		CInternet(HINTERNET handle):handle(handle) {}
		CInternet():handle(0) {}
		CInternet& operator = (HINTERNET handle)
		{
			::InternetCloseHandle(handle);
			this->handle = handle;
			return *this;
		}
		operator bool() { return handle!=0; }
		operator HINTERNET() { return handle; }
		~CInternet()
		{
			::InternetCloseHandle(handle);
		}
	};

	class Buffer : boost::noncopyable
	{
		void* const data;
	public:
		Buffer(size_t size):data(malloc(size)) {}
		~Buffer() { free(data); }
		operator const void*() const { return data; }
		operator void*() { return data; }
		operator char*() { return (char*)data; }
	};

	int httpGet(IInstallerSite *site, std::string host, std::string path, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log)
	{
		try
		{
			std::string tmp = etag;
			std::strstream input;
			int i = http(site, "GET", host, path, input, NULL, tmp, result, ignoreCancel, progress, log);
			etag = tmp;
			return i;
		}
		catch (silent_exception&)
		{
			throw;
		}
		catch (std::exception& e)
		{
			LLOG_ENTRY2(site->Logger(), "WARNING: First HTTP GET error for %s: %s", path.c_str(), exceptionToString(e).c_str());
			std::strstream input;
			result.seekp(0);
			result.clear();
			// try again
			return http(site, "GET", host, path, input, NULL, etag, result, ignoreCancel, progress, log);
		}
	}

	std::string httpGetString(const std::string& url)
	{
		cpr::Response response = cpr::Get(cpr::Url{ url });
		return response.text;
	}

	int httpWinInet(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress)
	{
		CUrl u;
		BOOL urlCracked;
		#ifdef UNICODE
			urlCracked = u.CrackUrl(convert_s2w(host).c_str());
		#else
			urlCracked = u.CrackUrl(host.c_str());
		#endif

		// Initialize the User Agent
		CInternet session = InternetOpen(_T("Roblox/WinInet"), PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
		if (!session)
			throw std::runtime_error(format_string("InternetOpen failed for %s http://%s%s, Error Code: %d", method, host.c_str(), path.c_str(), GetLastError()).c_str());

		CInternet connection;
		if (urlCracked)
			connection = ::InternetConnect(session, u.GetHostName(), u.GetPortNumber(), NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		else
			connection = ::InternetConnect(session, convert_s2w(host).c_str(), 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);

		if (!connection)
			throw std::runtime_error(format_string("InternetConnect failed for %s http://%s%s, Error Code: %d, Port Number: %d", method, host.c_str(), path.c_str(), GetLastError() , urlCracked ? u.GetPortNumber() : 80).c_str());

		//   1. Open HTTP Request (pass method type [get/post/..] and URL path (except server name))
		CInternet request = ::HttpOpenRequest(
			connection, convert_s2w(method).c_str(), convert_s2w(path).c_str(), NULL, NULL, NULL, 
			INTERNET_FLAG_KEEP_CONNECTION |
			INTERNET_FLAG_EXISTING_CONNECT |
			INTERNET_FLAG_NEED_FILE, // ensure that it gets cached
			1); 
		if (!request)
		{
			DWORD errorCode = GetLastError();

			throw std::runtime_error(format_string("HttpOpenRequest failed for %s http://%s%s, Error Code: %d", method, host.c_str(), path.c_str(), errorCode).c_str());
		}

		if (contentType)
		{
			std::string header = format_string("Content-Type: %s\r\n", contentType);
			throwLastError(::HttpAddRequestHeaders(request, convert_s2w(header).c_str(), header.size(), HTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");
		}

		size_t uploadSize;
		{
			size_t x = input.tellg();
			input.seekg (0, std::ios::end);
			size_t y = input.tellg();
			uploadSize = y - x;
			input.seekg (0, std::ios::beg);
		}

		if (uploadSize==0)
		{
			throwLastError(::HttpSendRequest(request, NULL, 0, 0, 0), "HttpSendRequest failed");
		}
		else
		{
			Buffer uploadBuffer(uploadSize);

			input.read((char*)uploadBuffer, uploadSize);

			// Send the request
			{
				INTERNET_BUFFERS buffer;
				memset(&buffer, 0, sizeof(buffer));
				buffer.dwStructSize = sizeof(buffer);
				buffer.dwBufferTotal = uploadSize;
				if (!HttpSendRequestEx(request, &buffer, NULL, 0, 0))
					throw std::runtime_error("HttpSendRequestEx failed");

				try
				{
					DWORD bytesWritten;
					throwLastError(::InternetWriteFile(request, uploadBuffer, uploadSize, &bytesWritten), "InternetWriteFile failed");

					if (bytesWritten!=uploadSize)
						throw std::runtime_error("Failed to upload content");
				}
				catch (std::exception&)
				{
					::HttpEndRequest(request, NULL, 0, 0);
					throw;
				}

				throwLastError(::HttpEndRequest(request, NULL, 0, 0), "HttpEndRequest failed");
			}
		}

		// Check the return HTTP Status Code
		DWORD statusCode;
		{
			DWORD dwLen = sizeof(DWORD);
			throwLastError(::HttpQueryInfo(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &dwLen, NULL), "HttpQueryInfo HTTP_QUERY_STATUS_CODE failed");
		}
		DWORD contentLength = 0;
		{
			DWORD dwLen = sizeof(DWORD);
			::HttpQueryInfo(request, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &dwLen, NULL);
		}
		{
			CHAR buffer[256];
			DWORD dwLen = sizeof(buffer);
			if (::HttpQueryInfo(request, HTTP_QUERY_ETAG, &buffer, &dwLen, NULL))
			{
				etag = buffer;
				etag = etag.substr(1, etag.size()-2);	// remove the quotes
			}
			else
				etag.clear();
		}

		DWORD readSoFar = 0;
		while (true)
		{
			DWORD numBytes;
			if (!::InternetQueryDataAvailable(request, &numBytes, 0, 0))
				numBytes = 0;
			if (numBytes==0)
				break; // EOF

			char buffer[1024];
			DWORD bytesRead;
			throwLastError(::InternetReadFile(request, (LPVOID) buffer, 1024, &bytesRead), "InternetReadFile failed");
			result.write(buffer, bytesRead);
			readSoFar += bytesRead;
			progress(bytesRead, contentLength);
		}

		if (statusCode!=HTTP_STATUS_OK)
		{
			TCHAR buffer[512];
			DWORD length = 512;
			if (::HttpQueryInfo(request, HTTP_QUERY_STATUS_TEXT, (LPVOID) buffer, &length, 0))
				throw std::runtime_error(convert_w2s(buffer).c_str());
			else
				throw std::runtime_error(format_string("statusCode = %d", statusCode));
		}

		return statusCode;
	}

	int httpCpr(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log)
	{
		std::string url = "https://" + host + path;

		std::string body((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
		size_t uploadSize = body.size();
		cpr::Header headers;
		/* Meow almost forgot the user agent */
		headers["User-Agent"] = "Roblox/WinInet";

		if (contentType)
			headers["Content-Type"] = contentType;

		if (!etag.empty())
			headers["If-None-Match"] = "\"" + etag + "\"";

		headers["Accept"] = "*/*";
		cpr::Response response;
		try {
			if (!strcmp(method, "GET")) {
				response = cpr::Get(
					cpr::Url{ url },
					headers,
					cpr::Timeout{ 30000 }
				);
			}
			else if (!strcmp(method, "POST")) {
				response = cpr::Post(
					cpr::Url{ url },
					headers,
					cpr::Body{ body },
					cpr::Timeout{ 30000 }
				);
			}
			else {
				throw std::runtime_error("Unsupported HTTP method");
			}
		}
		catch (const std::exception& ex)
		{
			MessageBoxA(nullptr, "Failed here", "failed here", MB_ICONERROR);
			if (log)
			{
				LLOG_ENTRY3(site->Logger(), "Trying WinInet for %s http://%s%s", method, host.c_str(), path.c_str());
			}
			return httpWinInet(site, method, host, path, input, contentType, etag, result, ignoreCancel, progress);
		}

		switch (response.status_code)
		{
		case cpr::status::HTTP_OK:
		case cpr::status::HTTP_ACCEPTED:
		case cpr::status::HTTP_NOT_MODIFIED:
			break;
		default:
		{
			if (log)
			{
				LLOG_ENTRY3(site->Logger(), "Trying WinInet for %s http://%s%s", method, host.c_str(), path.c_str());
			}
			return httpWinInet(site, method, host, path, input, contentType, etag, result, ignoreCancel, progress);
		}
		}

		auto it = response.header.find("etag");
		if (it != response.header.end())
		{
			etag = it->second;
			if (!etag.empty() && etag.front() == '"')
				etag = etag.substr(1, etag.size() - 2);
		}

		if (response.status_code != cpr::status::HTTP_NOT_MODIFIED)
		{
			result.write(response.text.c_str(), response.text.size());
			progress(static_cast<int>(result.tellp()), static_cast<int>(response.text.size()));
		}
		else
		{
			progress(1, 1);
		}

		return static_cast<int>(response.status_code);
	}

	int http(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log)
	{
		try
		{
			return httpCpr(site, method, host, path, input, contentType, etag, result, ignoreCancel, progress, log);
		}
		catch (std::exception&)
		{
			LLOG_ENTRY(site->Logger(), "httpCpr failed, falling back to winInet");
			return httpWinInet(site, method, host, path, input, contentType, etag, result, ignoreCancel, progress);
		}
	}

	int httpPost(IInstallerSite *site, std::string host, std::string path, std::istream& input, const char* contentType, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log)
	{
		std::string etag;
		return http(site, "POST", host, path, input, contentType, etag, result, ignoreCancel, progress, log);
	}

	int httpGetCdn(IInstallerSite *site, std::string secondaryHost, std::string path, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress)
	{
		std::string cdnHost;
		if (site->UseNewCdn())
			cdnHost = getCdnHost(site);
		
		if (cdnHost.empty())
			cdnHost = getPrimaryCdnHost(site);
		
		if(!cdnHost.empty()){
			try
			{
				std::string tmp = etag;
				int status_code = httpGet(site, cdnHost, path, tmp, result, ignoreCancel, progress);
				switch(status_code){
					case 200:
					case 304:
						//We succeeded so save the etag and return success
						etag = tmp;
						return status_code;
					default:
						LLOG_ENTRY3(site->Logger(), "Failure getting '%s' from cdnHost='%s', falling back to secondaryHost='%s'", path.c_str(), cdnHost.c_str(), secondaryHost.c_str());
						//Failure of some kind, fall back to secondaryHost below
						break;
				}
			}
			catch(std::exception&)
			{ 
				//Trap first exception and try again with secondaryHost
			}
		}

		//Reset our result vector
		result.seekp(0);
		result.clear();

		return httpGet(site, secondaryHost,path,etag,result,ignoreCancel,progress);
	}

}
