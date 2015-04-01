#pragma once

#include "dataSource.h"
#include "mapTile.h"
#include "tileData.h"
#include "pbfParser.h"

class OpenScienceMapSource : public NetworkDataSource {
   protected:
    virtual std::shared_ptr<TileData> parse(const MapTile& _tile,
                                            std::stringstream& _in) override;

    struct TagId {
        uint32_t key;
        uint32_t val;
        TagId(uint32_t key, uint32_t val) : key(key), val(val) {}
    };

    int zmin, zmax;
    bool s3db;

    bool extractFeature(protobuf::message it, GeometryType geomType,
                        const std::vector<TagId>& tags,
                        const std::vector<std::string>& keys,
                        const std::vector<std::string>& vals,
                        TileData& tileData, const MapTile& _tile);

    void readTags(protobuf::message it, std::vector<TagId>& tags);

   public:
    OpenScienceMapSource(int zmin, int zmax, bool s3db);

    int minZoom() const override {
        return zmin;
    };
    int maxZoom() const override {
        return zmax;
    };
};
