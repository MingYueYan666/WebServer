#pragma once
#include<string>
#include<vector>
#include<unordered_map>
#include "HTTP.h"
class HTMLServer
{
public:
	struct  FileResponse
	{
		std::string requestHead; //请求头
		std::vector<char> fileData; //文件数据
		rsize_t remainder = 0;
		int pos;
		bool IsBlock = false;
		std::string fileName;
		std::vector<char> GetData() {
			std::vector<char> result;
			if (!requestHead.empty())result.insert(result.end(), requestHead.begin(), requestHead.end());
			result.insert(result.end(), fileData.begin(), fileData.end());
			return result;
		}
	};

	HTMLServer(const std::string& host,const std::string & root);
	void Set404(const std::string& path);
	void SetMain(const std::string& path);
	void SetDefaultIco(const std::string& path);
	FileResponse GetData(const std::string& request);
	std::vector<char> GetBlock(const std::string& path, int pos, int size);
private:
	FileResponse GetFileResponse(const std::string& head, std::vector<char> data,rsize_t remainder);
	bool IsSafePath(const std::string& path);
	std::vector<char> GetFile(const std::string& path,bool sw = false);
	std::vector<char> GetFile(const std::string& path,
		long long actualBegin,
		long long actualEnd);

	FileResponse GetHTML(const HTTPTool::Request& request);
	FileResponse GetImage(const HTTPTool::Request& request);
	FileResponse GetMedia(const HTTPTool::Request& request);
	std::string GetMediaHTTPHeader(
		const std::string& contentType,
		unsigned long long contentLength,
		HTTPTool::Range range);
	std::string GetType(const std::string& path);
	bool Exis(const std::string& path);
	unsigned long long GetFileSize(const std::string& path);
	std::string GetWebFile(const std::string& path);
	std::string Find(const std::string& key);

	std::string Find(
		const std::string& key,
		const std::string& value);

	std::vector<char> Get404(const std::string& path);
	std::string root;
	std::string defaultIco;
	std::string main;
	std::string html404;
	std::string host;
	std::string contextGet;
};

