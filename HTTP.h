#pragma once
#include<string>
class HTTPTool
{
public:
	struct Range {
		long long begin;
		long long end;
	};
	enum RequestType
	{
		HTML, IMAGE, Media
	};
	struct Request
	{
		RequestType type;
		std::string path;
		Range range;
		bool isDownloadCommand = false;
		bool webMain = false;
	};
	static std::string HTTP200(const std::string& contentType, long long contentLength);
	static std::string HTTP206(const std::string& contentType, unsigned long long fullSize, Range range);
	static Request ParseRequest(const std::string& root, const std::string& request);
	static std::string GetType(const std::string& path);
	static Range GetRange(const std::string& request, unsigned long long fullSize);
	static std::string Find(const std::string& text,
		const std::string& key,
		const std::string& value);
private:
	static std::string GET(const std::string& request);
	static std::string MatchHTTPCommand(std::string& request);
	static bool IsHTTPCommand(const std::string& command);
	static RequestType GetRequestType(const std::string& request);
	static bool IsHTML(const std::string& file);
	static bool IsImage(const std::string& file);
	static bool IsAudio(const std::string& file);
	static bool IsVideo(const std::string& file);
};

