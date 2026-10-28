#include "HtmlServer.h"
#include<iostream>
#include<fstream>
#include<sstream>
#include<filesystem>
namespace fs = std::filesystem;
HTMLServer::HTMLServer(const std::string& host, const std::string& root)
	: host(host){
	this->root = "./" + root;
}
HTMLServer::FileResponse HTMLServer::GetData(const std::string& request) {
	auto type = HTTPTool::ParseRequest(root,request);
	if(!Exis(type.path))return GetHTML(type);
	switch (type.type)
	{
	case HTTPTool::RequestType::HTML: {
		return GetHTML(type);
	}
	case HTTPTool::RequestType::IMAGE: { 
		return GetImage(type);
	}
	case HTTPTool::RequestType::Media: {
		return GetMedia(type);
	}
		break;
	}
	return GetHTML(type);
}
bool HTMLServer::IsSafePath(const std::string& path) {
	if (!Exis(path)) return {};
	// 1. 获取并规范化 web 根目录的绝对路径
	fs::path rootPath = fs::weakly_canonical(root);   // root 为成员变量
	if (rootPath.empty()) {
		std::cerr << "Invalid web root directory" << std::endl;
		return false;
	}

	// 2. 规范化待检查的路径（path 已包含 ./root 前缀）
	fs::path requestPath;
	try {
		requestPath = fs::weakly_canonical(path);
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "Failed to canonicalize path: " << path << " -> " << e.what() << std::endl;
		return false;
	}

	// 3. 检查规范化后的请求路径是否仍以根目录为前缀
	auto [rootIt, reqIt] = std::mismatch(rootPath.begin(), rootPath.end(), requestPath.begin());
	if (rootIt != rootPath.end() || reqIt == requestPath.begin()) {
		// 路径不匹配：要么根目录的组件没有完全匹配，要么请求路径为空或完全不相关
		std::cerr << "Path escape detected: " << path <<std::endl;
		return false;
	}

	// 4. 可选：确保请求路径不是目录本身？调用者可自行判断，此处只验证安全
	return true;
}
std::vector<char> HTMLServer::GetFile(const std::string& path,bool sw) {
	std::string file = path;
	std::string NoSafePath = root + "/" + "NeverGonnaGiveYouUp.txt";
	if (!IsSafePath(path)&&!sw) {
		return GetFile(NoSafePath,true);
	}
	std::ifstream fileStream(file, std::ios::binary);
	if (!fileStream.is_open()) {
		std::cout << "No find file" + file << std::endl;
		return {};
	}
	std::cout << "Open file:" + file << std::endl;
	return std::vector<char>((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
}
std::vector<char> HTMLServer::GetFile(const std::string& path,
	long long actualBegin,
	long long actualEnd) {
	if (!IsSafePath(path)) {
		return {};
	}

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) return {};

	file.seekg(0, std::ios::end);
	std::streamsize fileSize = file.tellg();
	if (fileSize <= 0) return {};

	// 特殊规则：若请求区间为 (0,0)，则返回整个文件
	if (actualBegin == 0 && actualEnd == 0) {
		actualBegin = 0;
		actualEnd = fileSize - 1;
	}

	// 边界检查（实际区间已经在外部规范化时截断过，这里可做二次保险）
	if (actualBegin < 0) actualBegin = 0;
	if (actualEnd >= fileSize) actualEnd = fileSize - 1;
	if (actualBegin >= fileSize || actualBegin > actualEnd) return {};

	std::streamsize length = static_cast<std::streamsize>(actualEnd - actualBegin + 1);
	file.seekg(actualBegin, std::ios::beg);
	if (file.fail()) return {};

	std::vector<char> buffer(length);
	file.read(buffer.data(), length);
	if (file.gcount() != length) return {};

	std::cout << "Read file block: " << length << " bytes from " << actualBegin << " to " << actualEnd << std::endl;
	return buffer;
}
std::string HTMLServer::GetMediaHTTPHeader(const std::string& path,
	unsigned long long fullSize,
	HTTPTool::Range range) {
	// 直接使用外部已规范化的 range (actualBegin, actualEnd)
	long long actualBegin = range.begin;
	long long actualEnd = range.end;
	// 判断是否为完整文件请求（原始 begin == -1 && end == -1 或 0, -1 已在外部规范化）
	bool isFullFile = (actualBegin == 0 && actualEnd == fullSize - 1);
	if (isFullFile) {
		return HTTPTool::HTTP200(path, fullSize);
	}
	return HTTPTool::HTTP206(path, fullSize, range);
}
HTMLServer::FileResponse HTMLServer::GetImage(const HTTPTool::Request& request) {
	std::string file = request.path;
	if (!Exis(file)) {
		std::cout << "No find file:" + file << std::endl;
		file = root + "/" + defaultIco;
	}
	std::vector<char> buffer = GetFile(file);
	auto fileSize = buffer.size();
	std::string response = HTTPTool::HTTP200(file, fileSize);
	std::vector<char> result;
	std::cout << "image size:" << fileSize << "byte\n\n" << std::endl;
	int size = fileSize + response.size();
	return GetFileResponse(response, buffer, size);
}
HTMLServer::FileResponse HTMLServer::GetMedia(const HTTPTool::Request& request) {
	std::string path = request.path;
	if (!Exis(path)) return {};
	auto size = GetFileSize(path);
	auto range = request.range;
	std::vector<char> buffer = GetFile(path, range.begin, range.end);
	std::cout << "buffer size:" << buffer.size() << "byte\n\n" << std::endl;
	if (buffer.empty()) return {};
	if (range.begin == 0 && range.end == 0) {
		range.end = size - 1;
	}
	std::cout << "actual range: " << range.begin << "-" << range.end << std::endl;
	std::string response = GetMediaHTTPHeader(path, size, range);  
	return GetFileResponse(response, buffer, response.size() + buffer.size());
}
std::string HTMLServer::GetType(const std::string& path) {
	std::string extension = fs::path(path).extension().string();
	if (extension == ".jpg" || extension == ".jpeg") {
		return "image/jpeg";
	}
	else if (extension == ".png") {
		return "image/png";
	}
	else if (extension == ".gif") {
		return "image/gif";
	}
	else if (extension == ".ico") {
		return "image/x-icon";
	}else if (extension == ".mp3") {
		return "audio/mpeg";
	}
	else if (extension == ".wav") {
		return "audio/wav";
	}
	else if (extension == ".ogg") {
		return "audio/ogg";
	}
	return "application/octet-stream";
}
HTMLServer::FileResponse HTMLServer::GetFileResponse(const std::string& head,
	std::vector<char> data, 
	rsize_t remainder) {
	return FileResponse{ head , data ,remainder };
}
HTMLServer::FileResponse HTMLServer::GetHTML(const HTTPTool::Request& request) {
	std::vector<char> html;
	std::string file = request.path;
	if (request.webMain) {//主页
		file = file + main;
	}
	if (!Exis(file)) {
		html = Get404(file);
	}
	else
	{
		html = GetFile(file);
	}
	std::string response = HTTPTool::HTTP200(file, html.size());
	int size = html.size() + response.size();
	return GetFileResponse(response, html, size);
}
std::string HTMLServer::GetWebFile(const std::string& path) {
	std::ifstream file(path, std::istream::binary);
	if (!file.is_open()) { 
		std::cout << "No find file" << std::endl;
		return ""; 
	}
	std::cout << "Open file:" + path << std::endl;
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::cout << "document size:" << buffer.str().size() << "byte\n\n" << std::endl;
	return buffer.str();
}
bool HTMLServer::Exis(const std::string& file) {
	return fs::exists(file);
}
unsigned long long HTMLServer::GetFileSize(const std::string& path) {
	if (!Exis(path))return -1;
	fs::path filePath(path);
	return fs::file_size(filePath);
}
std::vector<char> HTMLServer::Get404(const std::string& path) {
	std::cout << "No find file:"+ path << std::endl;
	return GetFile(root + "/" + html404);
}
void HTMLServer::Set404(const std::string& path) {
	html404 = path;
}
void HTMLServer::SetMain(const std::string& path) {
	main = path;
}
void HTMLServer::SetDefaultIco(const std::string& path) {
	defaultIco = path;
}
std::string HTMLServer::Find(const std::string& key) {
	return  Find(key, "\r\n");
}
std::string HTMLServer::Find(
	const std::string& key,
	const std::string& value) {
	size_t begin = contextGet.find(key) + key.length();
	if (begin == std::string::npos)return "";
	size_t end = contextGet.find(value, begin);
	if (end == std::string::npos)return "";
	std::string result = contextGet.substr(begin, end - begin);
	return result;
}
std::vector<char> HTMLServer::GetBlock(const std::string& path, int pos, int size) {
	std::vector<char> buffer;
	// 参数校验
	if (size <= 0 || pos < 0) {
		return buffer;  // 空 vector
	}
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		return buffer;
	}
	// 获取文件总大小
	std::streamsize totalSize = file.tellg();
	if (pos >= totalSize) {
		// 起始位置超出文件末尾
		return buffer;
	}
	// 计算实际可读取的字节数（不能超过文件末尾）
	int maxReadable = static_cast<int>(totalSize - pos);
	int bytesToRead = (size > maxReadable) ? maxReadable : size;

	// 移动到指定位置
	file.seekg(pos, std::ios::beg);
	if (!file) {
		return buffer;
	}

	// 准备缓冲区并读取
	buffer.resize(bytesToRead);
	file.read(buffer.data(), bytesToRead);

	// 检查读取是否成功
	if (file.gcount() != bytesToRead) {
		// 读取的字节数不符合预期，但 gcount() 是实际读取的字节数
		buffer.resize(file.gcount());
	}
	return buffer;
}

