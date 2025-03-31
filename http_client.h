#pragma once

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <map>
#include <vector>

#pragma comment(lib, "winhttp.lib")

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    // Initialize the HTTP client with user agent
    bool Initialize(const std::wstring& userAgent = L"WinHttp Client/1.0");

    // Set request headers
    void SetHeaders(const std::map<std::wstring, std::wstring>& headers);

    // Perform HTTP GET request
    bool Get(const std::wstring& url, std::string& responseBody, DWORD& statusCode);

    // Perform HTTP POST request
    bool Post(const std::wstring& url, const std::string& requestBody,
        const std::wstring& contentType, std::string& responseBody, DWORD& statusCode);

    // Get the last error message
    std::wstring GetLastErrorMessage() const;

private:
    // Parse URL into components
    bool ParseUrl(const std::wstring& url, std::wstring& hostname, std::wstring& path, INTERNET_PORT& port, bool& isSecure);

    // Read response data
    bool ReadResponse(HINTERNET hRequest, std::string& responseBody);

    // Close handles and cleanup
    void Cleanup();

    // Member variables
    HINTERNET m_hSession;
    std::map<std::wstring, std::wstring> m_headers;
    std::wstring m_lastErrorMessage;
};
#endif