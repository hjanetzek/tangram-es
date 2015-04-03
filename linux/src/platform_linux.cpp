#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <mutex>
#include <thread>

#include <ctime>
#include <curl/curl.h>

#include "platform.h"

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

std::string stringFromResource(const char* _path) {
    std::string into;

    std::ifstream file;
    std::string buffer;

    file.open(_path);
    if (!file.is_open()) {
        logMsg("Failed to open file at path: %s\n", _path);
        return std::string();
    }

    while (!file.eof()) {
        getline(file, buffer);
        into += buffer + "\n";
    }

    file.close();
    return into;
}

unsigned char* bytesFromResource(const char* _path, unsigned int* _size) {
    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if (!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        *_size = 0;
        return nullptr;
    }

    *_size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*)malloc(sizeof(char) * (*_size));

    resource.read(cdata, *_size);
    resource.close();

    return reinterpret_cast<unsigned char*>(cdata);
}

// write_data call back from CURLOPT_WRITEFUNCTION
// responsible to read and fill "stream" with the data.
static size_t write_data(void* _ptr, size_t _size, size_t _nmemb,
                         void* _stream) {
    ((std::stringstream*)_stream)
        ->write(reinterpret_cast<char*>(_ptr), _size * _nmemb);
    return _size * _nmemb;
}

static std::mutex m_mutex;
static std::vector<CURL*> curlHandles;
static int nHandle = 0;

void platformInit() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void platformDispose() {
  for (auto handle :  curlHandles)
    curl_easy_cleanup(handle);

  curl_global_cleanup();
}

bool fetchData(std::unique_ptr<std::string> _url, std::stringstream& _rawData) {
    CURL* curlHandle = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!curlHandles.empty()) {
            curlHandle = curlHandles.back();
            curlHandles.pop_back();
        }
    }

    if (!curlHandle) {
         nHandle++;
      
        // this fixes the initial timeouts for some of
        // the first requests..
        int delay = nHandle * 200 ;
        logMsg("CREATE CURL HANDLE in %d ms\n", delay);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        curlHandle = curl_easy_init();

        // set up curl to perform fetch
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curlHandle, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curlHandle, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);

        curl_easy_setopt(curlHandle, CURLOPT_TCP_KEEPALIVE, 10L);
        curl_easy_setopt(curlHandle, CURLOPT_TCP_KEEPIDLE, 10L);
        curl_easy_setopt(curlHandle, CURLOPT_TCP_KEEPINTVL, 100L);
        curl_easy_setopt(curlHandle, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, 30);
        curl_easy_setopt(curlHandle, CURLOPT_CONNECTTIMEOUT, 10);
        curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "gzip");
        curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "tangram-es");
    }

    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &_rawData);
    curl_easy_setopt(curlHandle, CURLOPT_URL, _url->c_str());

    logMsg("Fetching URL with curl: %s\n", _url->c_str());

    CURLcode result = curl_easy_perform(curlHandle);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        curlHandles.push_back(curlHandle);
        // curl_easy_cleanup(curlHandle);
    }

    if (result != CURLE_OK) {
        logMsg("curl_easy_perform failed: %s\n", curl_easy_strerror(result));
        return false;
    } else {
        return true;
    }
}

double getTime() {
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (double)tp.tv_sec + tp.tv_nsec * (double)1e-9;
}
