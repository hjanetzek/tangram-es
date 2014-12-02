#include "tileManager.h"
#include "scene/scene.h"
#include "tile/mapTile.h"
#include "view/view.h"

#include <chrono>

TileManager::TileManager() {
}

TileManager::TileManager(TileManager&& _other) :
    m_view(std::move(_other.m_view)),
    m_tileSet(std::move(_other.m_tileSet)),
    m_bufferedTileSet(std::move(_other.m_bufferedTileSet)),
    m_dataSources(std::move(_other.m_dataSources)),
    m_incomingTiles(std::move(_other.m_incomingTiles)),
    m_incomingBuffTiles(std::move(_other.m_incomingBuffTiles)) {
}

TileManager::~TileManager() {
    m_dataSources.clear();
    m_tileSet.clear();
    m_bufferedTileSet.clear();
    m_incomingTiles.clear();
    m_incomingBuffTiles.clear();
}

bool TileManager::updateTileSet() {
    
    bool tileSetChanged = false;
    
    // Check if any incoming tiles are finished
    {
        auto incomingTilesIter = m_incomingTiles.begin();
        
        while (incomingTilesIter != m_incomingTiles.end()) {
            auto& tileFuture = *incomingTilesIter;
            std::chrono::milliseconds span(0);
            if(tileFuture.wait_for(span) == std::future_status::ready) {
                auto tile = tileFuture.get();
                const auto& id = tile->getID();
                logMsg("Visible Tile [%d, %d, %d] finished loading\n", id.z, id.x, id.y);
                
                // lock to write to m_tileSet
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_tileSet[id].reset(tile);
                }
                
                tileSetChanged = true;
                incomingTilesIter = m_incomingTiles.erase(incomingTilesIter);
                // Visible tile loaded, construct async threads for its hierarchy tiles
                generateBufferTiles(id);
            }
            else {
                incomingTilesIter++;
            }
        }
    }

    // Check if any incoming buffer tiles are finished
    {
        // Finishing processing all visible tiles before grabbing any buffer tile
        if(m_incomingTiles.size() == 0) {
            auto incomingBuffTilesIter = m_incomingBuffTiles.begin();
            while (incomingBuffTilesIter != m_incomingBuffTiles.end()) {
                auto& buffTileFuture = *incomingBuffTilesIter;
                std::chrono::milliseconds span (0);
                if (buffTileFuture.wait_for(span) == std::future_status::ready) {
                    auto tile = buffTileFuture.get();
                    const auto& id = tile->getID();
                    logMsg("Buffer Tile [%d, %d, %d] finished loading\n", id.z, id.x, id.y);
                    m_bufferedTileSet[id].reset(tile);
                    incomingBuffTilesIter = m_incomingBuffTiles.erase(incomingBuffTilesIter);
                }
                else {
                    incomingBuffTilesIter++;
                }
            }
        }
    }

    if (!(m_view->viewChanged()) && !tileSetChanged) {
        // No new tiles have come into view and no tiles have finished loading, 
        // so the tileset is unchanged
        return false;
    }

    const std::set<TileID>& visibleTiles = m_view->getVisibleTiles();

    auto tileSetIter = m_tileSet.begin();
    auto visTilesIter = visibleTiles.begin();

    // Diff previously visible tilesetIDs with new visible tileset IDs
    while (tileSetIter != m_tileSet.end() && visTilesIter != visibleTiles.end()) {

        TileID visTile = *visTilesIter;

        TileID tileInSet = tileSetIter->first;

        if (visTile == tileInSet) {
            // Tiles match here, nothing to do
            visTilesIter++;
            tileSetIter++;
        } else if (visTile < tileInSet) {
            // tileSet is missing an element present in visibleTiles
            addTile(visTile);
            visTilesIter++;
            tileSetChanged = true;
        } else {
            // visibleTiles is missing an element present in tileSet
            removeTile(tileSetIter);
            tileSetChanged = true;
        }

    }

    while (tileSetIter != m_tileSet.end()) {
        // All tiles in tileSet that haven't been covered yet are not in visibleTiles, so remove them
        removeTile(tileSetIter);
        tileSetChanged = true;
    }

    while (visTilesIter != visibleTiles.end()) {
        // All tiles in visibleTiles that haven't been covered yet are not in tileSet, so add them
        addTile(*visTilesIter);
        visTilesIter++;
        tileSetChanged = true;
    }
    return tileSetChanged;
}

void TileManager::addTile(const TileID& _tileID) {
    
    //check if this tile is already buffered, if yes grab from buffer asyncly
    if(m_bufferedTileSet.find(_tileID) != m_bufferedTileSet.end()) {

        auto addBufferTileFut = std::async(std::launch::async, [&]() {
            
            // lock to write to m_tileSet
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_tileSet[_tileID] = m_bufferedTileSet[_tileID];
            }
            
            for(const auto& source : m_dataSources) {
                logMsg("Adding Buffered tile [%d, %d, %d] to visible tile set\n", _tileID.z, _tileID.x, _tileID.y);
                
                //Get already loaded tile data from the source
                auto tileData = source->getTileData(_tileID);
                for(auto& style : m_scene->getStyles()) {
                    style->addData(*tileData, *(m_tileSet[_tileID]), m_view->getMapProjection());
                }
            }
            
        });
        return;
    }
    
    // lock to write to m_tileSet
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tileSet[_tileID].reset(); // Emplace a unique_ptr that is null until tile finishes loading
    }

    /*
     * Start a async thread to fetch this (_tileID) visible tile
     */
    auto incoming = std::async(std::launch::async, [&](TileID _id) {
        MapTile* tile = new MapTile(_id, m_view->getMapProjection());
        for (const auto& source : m_dataSources) {
            logMsg("Loading tile [%d, %d, %d]\n", _id.z, _id.x, _id.y);
            if ( ! source->loadTileData(*tile)) {
                logMsg("ERROR: Loading failed for tile [%d, %d, %d]\n", _id.z, _id.x, _id.y);
            }
            auto tileData = source->getTileData(_id);
            for (auto& style : m_scene->getStyles()) {
                style->addData(*tileData, *tile, m_view->getMapProjection());
            }
        }
        return tile;
    }, _tileID);
    
    m_incomingTiles.push_back(std::move(incoming));
}

void TileManager::generateBufferTiles(TileID _origTileID) {

    // Generate Hierarchy tileIDs
    std::vector<TileID> hierarchyTileIDs;
    _origTileID.getHierarchyTiles(hierarchyTileIDs, m_view->s_maxZoom);

    // Load hierarchy tiles async
    for(const auto& hierarchyTileID : hierarchyTileIDs) {
        // Skip loading buffer tile if already loaded by another set of visible tile(s)
        if(m_bufferedTileSet.find(hierarchyTileID) != m_bufferedTileSet.end()) {
            continue;
        }
        else {
            auto bufferTileIncoming = std::async(std::launch::async, [&]() {
                MapTile* newBuffTile = new MapTile(hierarchyTileID, m_view->getMapProjection());
                for(const auto& source : m_dataSources) {
                    logMsg("Loading buffer tile: [%d, %d, %d] \tcorresponding to visible tile: [%d, %d, %d]\n", hierarchyTileID.z, hierarchyTileID.x, hierarchyTileID.y, _origTileID.z, _origTileID.x, _origTileID.y);
                    if( ! source->loadTileData(*newBuffTile)) {
                        logMsg("ERROR: Loading failed for buffer tile [%d, %d, %d]\n", hierarchyTileID.z, hierarchyTileID.x, hierarchyTileID.y);
                    }
                }
                return newBuffTile;
            });
            m_incomingBuffTiles.push_back(std::move(bufferTileIncoming));
        }
    }
}

void TileManager::removeTile(std::map< TileID, std::shared_ptr<MapTile> >::iterator& _tileIter) {
    
    // Remove tile from tileSet and destruct tile
    // lock to write to m_tileSet
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        _tileIter = m_tileSet.erase(_tileIter);
    }

    // TODO: if tile is being loaded, cancel loading; For now they continue to load
    // and will be culled the next time that updateTileSet is called
    
}
