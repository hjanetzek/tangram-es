#pragma once

#include <memory>
#include <future>

#include "util/tileID.h"
#include "data/dataSource.h"
#include "mapTile.h"

class TileWorker {
    
public:
    
    TileWorker();
    
    void load(const TileID& _tile,
              const std::vector<std::unique_ptr<DataSource>>& _dataSources,
              const std::vector<std::unique_ptr<Style>>& _styles,
              const View& _view);
    
    void abort();
    
    bool isFinished() const { return m_finished; }

    bool isFree() const { return m_free; }
    
    const TileID& getTileID() const { return m_tileID ? *m_tileID : NOT_A_TILE; }
    
    std::shared_ptr<MapTile> getTileResult();
    
private:
    
    std::unique_ptr<TileID> m_tileID;
    
    bool m_free;
    bool m_aborted;
    bool m_finished;
    
    int m_id;
    std::future< std::shared_ptr<MapTile> > m_future;
};
