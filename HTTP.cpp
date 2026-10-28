#include "HTTP.h"
#include<iostream>
#include<filesystem>
#include<unordered_map>
namespace fs = std::filesystem;
std::string HTTPTool::HTTP200(const std::string& contentType, long long contentLength) {
	std::string result = "HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Type: " + GetType(contentType) + "\r\n"
		"Content-Length: " + std::to_string(contentLength) + "\r\n"
		"Connection: close\r\n"
		"\r\n";
	return result;
}
std::string HTTPTool::HTTP206(const std::string& contentType, unsigned long long fullSize, Range range) {
	long long contentLength = range.end - range.begin + 1;
	std::string response = "HTTP/1.1 206 Partial Content\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Type: " + GetType(contentType) + "\r\n"
		"Content-Range: bytes " + std::to_string(range.begin) + "-"
		+ std::to_string(range.end) + "/" + std::to_string(fullSize) + "\r\n"
		"Content-Length: " + std::to_string(contentLength) + "\r\n"
		"Connection: close\r\n"
		"\r\n";
	return response;
}
std::string HTTPTool::GetType(const std::string& path) {
	static const std::unordered_map<std::string, std::string> mimeMap = {
		// 图片
		{".jpg",  "image/jpeg"},
		{".jpeg", "image/jpeg"},
		{".png",  "image/png"},
		{".gif",  "image/gif"},
		{".ico",  "image/x-icon"},
		// 音频
		{".mp3",  "audio/mpeg"},
		{".wav",  "audio/wav"},
		{".ogg",  "audio/ogg"},
		// 文本/代码
		{".html", "text/html"},
		{".htm",  "text/html"},
		{".css",  "text/css"},
		{".js",   "application/javascript"},
		// 视频
		{".mp4",  "video/mp4"},
		{".webm", "video/webm"},
		{".avi",  "video/x-msvideo"},
		{".mov",  "video/quicktime"},
		{".mkv",  "video/x-matroska"},
		{".flv",  "video/x-flv"},
		{".mpeg", "video/mpeg"},
		{".ogv",  "video/ogg"},
		{".ts",   "video/MP2T"}
	};
    std::string extension = fs::path(path).extension().string();
    auto it = mimeMap.find(extension);
    if (it != mimeMap.end()) {
        return it->second;
    }
    return "application/octet-stream";
}
HTTPTool::Range HTTPTool::GetRange(const std::string& request, unsigned long long fullSize) {
	// 1. 解析 Range 头
	std::string range = Find(request, "Range: bytes=", "\r\n");
	size_t dashPos = range.find('-');
	if (dashPos == std::string::npos) {
		return {}; // 无效 Range
	}

	std::string beginStr = range.substr(0, dashPos);
	std::string endStr = range.substr(dashPos + 1);

	long long begin = -1;
	long long end = -1;

	try {
		if (!beginStr.empty()) begin = std::stoll(beginStr);
		if (!endStr.empty())   end = std::stoll(endStr);
	}
	catch (...) {
		return {}; // 解析失败
	}

	// 2. 规范化区间（实际要读取的 begin / end）
	long long actualBegin = begin;
	long long actualEnd = end;

	// 完整文件请求：无 Range 或 Range: bytes=0-
	bool isFullFile = (begin == -1 && end == -1) || (begin == 0 && end == -1);
	if (isFullFile) {
		actualBegin = 0;
		actualEnd = fullSize - 1;
	}
	// 后缀范围: bytes=-end
	else if (begin == -1 && end > 0) {
		actualEnd = fullSize - 1;
		actualBegin = fullSize - end;
		if (actualBegin < 0) actualBegin = 0;
	}
	// 前缀范围: bytes=begin-
	else if (end == -1) {
		actualBegin = begin;
		actualEnd = fullSize - 1;
	}
	// 明确区间: bytes=begin-end
	else {
		actualBegin = begin;
		actualEnd = end;
	}

	// 3. 边界检查与截断（按文件最大大小处理）
	if (actualBegin >= fullSize) {
		// 起始越界 -> 无效请求
		return {};
	}
	if (actualEnd >= fullSize) {
		actualEnd = fullSize - 1;
	}
	if (actualBegin > actualEnd) {
		return {};
	}
	if (actualBegin == 0&& actualEnd == 0) {
		actualBegin = 0;
		actualEnd = fullSize - 1;
	}
	// 返回规范化后的区间
	return HTTPTool::Range{ actualBegin, actualEnd };
}
std::string HTTPTool::Find(const std::string& text,
	const std::string& key,
	const std::string& value) {
	size_t begin = text.find(key) + key.length();
	if (begin == std::string::npos)return "NULL 1";
	size_t end = text.find(value, begin);
	if (end == std::string::npos)return "NULL 2";
	std::string result = text.substr(begin, end - begin);
	return result;
}

HTTPTool::Request HTTPTool::ParseRequest(const std::string& root,const std::string & request) {
	std::cout << "ParseRequest: \n" << request << std::endl;
	std::string path = root + GET(request);
	auto temp = MatchHTTPCommand(path);
	bool isDownloadCommand = !temp.empty();
	bool isWebMain = (path == root + "/");
	if (isDownloadCommand) path = temp;
	RequestType type = GetRequestType(path);
	return Request{ type, path, GetRange(request, 0), isDownloadCommand, isWebMain };
}
HTTPTool::RequestType HTTPTool::GetRequestType(const std::string& path) {
	if (IsHTML(path)) {
		std::cout << "is document file" << std::endl;
		return HTML;
	} //主页或html文件
	if (IsImage(path)) {
		std::cout << "is image file" << std::endl;
		return IMAGE;
	}
	if (IsAudio(path)|| IsVideo(path)) {
		std::cout << "is media file" << std::endl;
		return Media;
	}
	return HTML; // 默认返回HTML
}
bool HTTPTool::IsHTML(const std::string& file) {
	if (file == "/") return true;  // 主页
	if (file.find(".html") != std::string::npos ||
		file.find(".htm") != std::string::npos) return true;
	return false;
}
bool HTTPTool::IsImage(const std::string& file) {
	if (file.find(".jpg") != std::string::npos ||
		file.find(".jpeg") != std::string::npos ||
		file.find(".png") != std::string::npos ||
		file.find(".gif") != std::string::npos ||
		file.find(".ico") != std::string::npos) return true;
	return false;
}
bool HTTPTool::IsAudio(const std::string& file) {
	if (file.find(".mp3") != std::string::npos ||
		file.find(".wav") != std::string::npos ||
		file.find(".ogg") != std::string::npos) return true;
	return false;
}
bool HTTPTool::IsVideo(const std::string& file) {
	if (file.find(".mp4") != std::string::npos ||
		file.find(".avi") != std::string::npos ||
		file.find(".mov") != std::string::npos ||
		file.find(".mkv") != std::string::npos) return true;
	return false;
}
std::string HTTPTool::GET(const std::string& request) {
	return Find(request, "GET ", " ");
}

std::string HTTPTool::MatchHTTPCommand(std::string& request) {
	std::cout << "MatchHTTPCommand: " << request << std::endl;
	//download?file=D:/104678109_p2_master1200.jpg
	if(request.find("/download?file=") != std::string::npos) {
		std::cout << "is download command" << std::endl;
		request += " ";
		std::string fileName = Find(request,"=", " ");
		return fileName; // 目前仅支持 GET，直接返回路径
	}
	return {}; // 目前仅支持 GET，直接返回路径
}