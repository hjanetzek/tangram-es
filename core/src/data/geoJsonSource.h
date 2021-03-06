#pragma once

#include "dataSource.h"
#include "mapTile.h"
#include "tileData.h"


/* Extends NetworkDataSource class to read Mapzen's GeoJSON vector tiles */
class GeoJsonSource: public DataSource {
    
protected:
    
    virtual std::shared_ptr<TileData> parse(const MapTile& _tile, std::vector<char>& _rawData) const override;
    
public:
    
    GeoJsonSource(const std::string& _name, const std::string& _urlTemplate);
    
};
