#include "geoJson.h"
#include "dataSource.h"
#include "platform.h"
#include "tileID.h"
#include "tileData.h"
#include "mapTile.h"
#include "tileManager.h"

#include <chrono>
#include <thread>

//---- DataSource Implementation----

bool DataSource::hasTileData(const TileID& _tileID) {
    
    return m_tileStore.find(_tileID) != m_tileStore.end();
}

std::shared_ptr<TileData> DataSource::getTileData(const TileID& _tileID) {
    
    if (hasTileData(_tileID)) {
        std::shared_ptr<TileData> tileData = m_tileStore[_tileID];
        return tileData;
    } else {
        return nullptr;
    }
}

void DataSource::clearData() {
    for (auto& mapValue : m_tileStore) {
        mapValue.second->layers.clear();
    }
    m_tileStore.clear();
}

//---- NetworkDataSource Implementation----

#if 0 
//write_data call back from CURLOPT_WRITEFUNCTION
//responsible to read and fill "stream" with the data.
static size_t write_data(void *_ptr, size_t _size, size_t _nmemb, void *_stream) {
    ((std::string*) _stream)->append(reinterpret_cast<char *>(_ptr), _size * _nmemb);
    return _size * _nmemb;
}

NetworkDataSource::NetworkDataSource()
    : m_curlHandles(TileManager::MAX_WORKERS) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

NetworkDataSource::~NetworkDataSource() {
    for (auto curlHandle : m_curlHandles) curl_easy_cleanup(curlHandle);

    curl_global_cleanup();
#else
NetworkDataSource::NetworkDataSource() {
}

NetworkDataSource::~NetworkDataSource() {
#endif
}

std::unique_ptr<std::string> NetworkDataSource::constructURL(const TileID& _tileCoord) {

    std::unique_ptr<std::string> urlPtr(new std::string(m_urlTemplate)); // Make a copy of our template

    size_t xpos = urlPtr->find("[x]");
    urlPtr->replace(xpos, 3, std::to_string(_tileCoord.x));
    
    size_t ypos = urlPtr->find("[y]");
    urlPtr->replace(ypos, 3, std::to_string(_tileCoord.y));
    
    size_t zpos = urlPtr->find("[z]");
    urlPtr->replace(zpos, 3, std::to_string(_tileCoord.z));
    
    if (xpos == std::string::npos || ypos == std::string::npos || zpos == std::string::npos) {
        logMsg("Bad URL template!!\n");
    }
    
    return std::move(urlPtr);
}

bool NetworkDataSource::loadTileData(int workerId, const MapTile& _tile) {

    bool success = true; // Begin optimistically

    if (hasTileData(_tile.getID())) {
        // Tile has been fetched already!
        return success;
    }

    std::unique_ptr<std::string> url = constructURL(_tile.getID());
    
    std::stringstream rawData;

#if 0
    CURL* curlHandle = m_curlHandles[workerId];

    if (!curlHandle) {
        // this fixes the initial timeouts for some of
        // the first requests..
        std::this_thread::sleep_for(std::chrono::milliseconds(200) * workerId);

        curlHandle = curl_easy_init();
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

        m_curlHandles[workerId] = curlHandle;
    }

    // out will store the stringStream contents from curl
    std::string buf;
    buf.reserve(32 * 1024);

    // set up curl to perform fetch
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curlHandle, CURLOPT_URL, url->c_str());

    // struct curl_slist* requestHeaders = 0;
    // requestHeaders = curl_slist_append(requestHeaders, "Connection: Keep-Alive");
    // requestHeaders = curl_slist_append(requestHeaders, "User-Agent: tangram");
    // curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, requestHeaders);

    double timeStart = getTime();

    logMsg("Worker %d, Fetching URL with curl: %s\n", workerId, url->c_str());

    CURLcode result = curl_easy_perform(curlHandle);

    if (result != CURLE_OK) {

      logMsg("Worker %d, curl_easy_perform failed: %s\n", workerId, curl_easy_strerror(result));
        success = false;

    } else {
        std::stringstream out(buf);

        logMsg("Worker %d, Fetching took: %f\n", workerId, getTime() - timeStart);

        // parse fetched data
        std::shared_ptr<TileData> tileData = parse(_tile, out);

#else
    success = fetchData(std::move(url), rawData);

    if(rawData) {

        // parse fetched data
        std::shared_ptr<TileData> tileData = parse(_tile, rawData);
        
#endif
        // Lock our mutex so that we can safely write to the tile store
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tileStore[_tile.getID()] = tileData;
        }

#if 0
    }

    //curl_slist_free_all(requestHeaders);

    //curl_easy_cleanup(curlHandle);

    return success;
}

//---- MapzenVectorTileJson Implementation----
#else
    } else {
        success = false;
    }
    
    
    return success;
}
#endif
