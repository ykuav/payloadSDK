#include "http_client.h"
#include <sstream>

HttpClient::HttpClient() : m_hSession(NULL) {
}

HttpClient::~HttpClient() {
    Cleanup();
}

bool HttpClient::Initialize(const std::wstring& userAgent) {
    // Create session handle
    m_hSession = WinHttpOpen(
        userAgent.c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!m_hSession) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpOpen failed with error: " << error;
        m_lastErrorMessage = ss.str();
        return false;
    }

    // Set timeouts (optional)
    WinHttpSetTimeouts(m_hSession, 10000, 10000, 10000, 10000);

    return true;
}

void HttpClient::SetHeaders(const std::map<std::wstring, std::wstring>& headers) {
    m_headers = headers;
}

bool HttpClient::Get(const std::wstring& url, std::string& responseBody, DWORD& statusCode) {
    std::wstring hostname, path;
    INTERNET_PORT port;
    bool isSecure;

    // Parse URL
    if (!ParseUrl(url, hostname, path, port, isSecure)) {
        return false;
    }

    // Connect to server
    HINTERNET hConnect = WinHttpConnect(m_hSession, hostname.c_str(), port, 0);
    if (!hConnect) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpConnect failed with error: " << error;
        m_lastErrorMessage = ss.str();
        return false;
    }

    // Create request handle
    DWORD flags = isSecure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        path.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);

    if (!hRequest) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpOpenRequest failed with error: " << error;
        m_lastErrorMessage = ss.str();
        WinHttpCloseHandle(hConnect);
        return false;
    }

    // Add headers
    for (const auto& header : m_headers) {
        std::wstring headerLine = header.first + L": " + header.second;
        WinHttpAddRequestHeaders(
            hRequest,
            headerLine.c_str(),
            static_cast<DWORD>(-1),
            WINHTTP_ADDREQ_FLAG_ADD);
    }

    // Send request
    BOOL result = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0);

    if (!result) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpSendRequest failed with error: " << error;
        m_lastErrorMessage = ss.str();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return false;
    }

    // Receive response
    result = WinHttpReceiveResponse(hRequest, NULL);
    if (!result) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpReceiveResponse failed with error: " << error;
        m_lastErrorMessage = ss.str();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return false;
    }

    // Get status code
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX);

    // Read response data
    bool readSuccess = ReadResponse(hRequest, responseBody);

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);

    return readSuccess;
}

bool HttpClient::Post(const std::wstring& url, const std::string& requestBody,
    const std::wstring& contentType, std::string& responseBody, DWORD& statusCode) {
    std::wstring hostname, path;
    INTERNET_PORT port;
    bool isSecure;

    // Parse URL
    if (!ParseUrl(url, hostname, path, port, isSecure)) {
        return false;
    }

    // Connect to server
    HINTERNET hConnect = WinHttpConnect(m_hSession, hostname.c_str(), port, 0);
    if (!hConnect) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpConnect failed with error: " << error;
        m_lastErrorMessage = ss.str();
        return false;
    }

    // Create request handle
    DWORD flags = isSecure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        path.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);

    if (!hRequest) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpOpenRequest failed with error: " << error;
        m_lastErrorMessage = ss.str();
        WinHttpCloseHandle(hConnect);
        return false;
    }

    // Add content type header
    std::wstring contentTypeHeader = L"Content-Type: " + contentType;
    WinHttpAddRequestHeaders(
        hRequest,
        contentTypeHeader.c_str(),
        static_cast<DWORD>(-1),
        WINHTTP_ADDREQ_FLAG_ADD);

    // Add other headers
    for (const auto& header : m_headers) {
        std::wstring headerLine = header.first + L": " + header.second;
        WinHttpAddRequestHeaders(
            hRequest,
            headerLine.c_str(),
            static_cast<DWORD>(-1),
            WINHTTP_ADDREQ_FLAG_ADD);
    }

    // Send request with data
    BOOL result = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)requestBody.c_str(),
        static_cast<DWORD>(requestBody.size()),
        static_cast<DWORD>(requestBody.size()),
        0);

    if (!result) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpSendRequest failed with error: " << error;
        m_lastErrorMessage = ss.str();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return false;
    }

    // Receive response
    result = WinHttpReceiveResponse(hRequest, NULL);
    if (!result) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpReceiveResponse failed with error: " << error;
        m_lastErrorMessage = ss.str();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return false;
    }

    // Get status code
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX);

    // Read response data
    bool readSuccess = ReadResponse(hRequest, responseBody);

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);

    return readSuccess;
}

bool HttpClient::ParseUrl(const std::wstring& url, std::wstring& hostname, std::wstring& path, INTERNET_PORT& port, bool& isSecure) {
    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);

    // Set required component lengths to non-zero 
    // so that they are cracked
    urlComp.dwSchemeLength = -1;
    urlComp.dwHostNameLength = -1;
    urlComp.dwUrlPathLength = -1;

    // Crack the URL
    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.length()), 0, &urlComp)) {
        DWORD error = GetLastError();
        std::wstringstream ss;
        ss << L"WinHttpCrackUrl failed with error: " << error;
        m_lastErrorMessage = ss.str();
        return false;
    }

    // Extract components
    hostname = std::wstring(urlComp.lpszHostName, urlComp.dwHostNameLength);
    path = std::wstring(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    port = urlComp.nPort;

    // Check if HTTPS
    std::wstring scheme(urlComp.lpszScheme, urlComp.dwSchemeLength);
    isSecure = (_wcsicmp(scheme.c_str(), L"https") == 0);

    return true;
}

bool HttpClient::ReadResponse(HINTERNET hRequest, std::string& responseBody) {
    responseBody.clear();

    DWORD bytesAvailable = 0;
    DWORD bytesRead = 0;
    std::vector<char> buffer;

    do {
        // Check how many bytes are available
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            DWORD error = GetLastError();
            std::wstringstream ss;
            ss << L"WinHttpQueryDataAvailable failed with error: " << error;
            m_lastErrorMessage = ss.str();
            return false;
        }

        // No more data available
        if (bytesAvailable == 0) {
            break;
        }

        // Allocate buffer
        buffer.resize(bytesAvailable + 1); // +1 for null terminator

        // Read data
        if (!WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
            DWORD error = GetLastError();
            std::wstringstream ss;
            ss << L"WinHttpReadData failed with error: " << error;
            m_lastErrorMessage = ss.str();
            return false;
        }

        // Null terminate (for safety)
        buffer[bytesRead] = '\0';

        // Append to response
        responseBody.append(buffer.data(), bytesRead);

    } while (bytesAvailable > 0);

    return true;
}

void HttpClient::Cleanup() {
    if (m_hSession) {
        WinHttpCloseHandle(m_hSession);
        m_hSession = NULL;
    }
}

std::wstring HttpClient::GetLastErrorMessage() const {
    return m_lastErrorMessage;
}