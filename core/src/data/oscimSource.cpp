#include <algorithm>

#include "platform.h"
#include "tileID.h"

#include <sstream>
#include <fstream>
#include <iostream>

#include "oscimSource.h"
#include "oscimTags.h"

OpenScienceMapSource::OpenScienceMapSource(int zmin, int zmax, bool s3db)
    : zmin(zmin), zmax(zmax), s3db(s3db) {
    m_urlTemplate = s3db
                        ? "http://opensciencemap.org/tiles/s3db/[z]/[x]/[y].vtm"
                        : "http://opensciencemap.org/tiles/vtm/[z]/[x]/[y].vtm";
}

#define VERSION 1
#define TIMESTAMP 2
#define WATER_TILE 3

#define NUM_TAGS 11
#define NUM_KEYS 12
#define NUM_VALS 13

#define KEYS 14
#define VALS 15
#define TAGS 16

#define ELEM_LINES 21
#define ELEM_POLY 22
#define ELEM_POINT 23
#define ELEM_MESH 24

#define ELEM_NUM_INDICES 1
#define ELEM_NUM_TAGS 2
#define ELEM_TAGS 11
#define ELEM_INDICES 12
#define ELEM_COORDINATES 13
#define ELEM_OSM_LAYER 21

const static int32_t tileExtent = 4096;
const static double invTileExtent = 1.0 / 4096.0;

static double EARTH_CIRCUMFERENCE = 40075016.686;

static std::vector<Point> parsePoints3D(protobuf::message& it,
                                        float heightScale) {
    std::vector<Point> points;
    int32_t x = 0, y = 0, z = 0, cur = 0;

    while (it.getData() < it.getEnd()) {
        int32_t s = it.svarint();

        if (cur == 0) {
            x = x + s;
        } else if (cur == 1) {
            y = y + s;
        } else {
            z = z + s;

            points.emplace_back(invTileExtent * (double)(2 * x - tileExtent),
                                invTileExtent * (double)(tileExtent - 2 * y),
                                z / 10.0 / heightScale);
        }
        cur = (cur + 1) % 3;
    }
    return points;
}

static std::vector<Point> parsePoints(protobuf::message& it, size_t len,
                                      int32_t& lastX, int32_t& lastY) {
    std::vector<Point> points;
    points.reserve(len);

    uint32_t cnt = 0;
    int32_t x = lastX, y = lastY;

    while (it.getData() < it.getEnd() && cnt < len * 2) {
        int32_t s = it.svarint();
        if ((cnt++ % 2) == 0) {
            x = x + s;
        } else {
            y = y + s;
            points.emplace_back(invTileExtent * (double)(2 * x - tileExtent),
                                invTileExtent * (double)(tileExtent - 2 * y),
                                0.0);
        }
    }
    lastX = x;
    lastY = y;
    return points;
}

static std::vector<Line> parseLines(protobuf::message& it,
                                    std::vector<uint32_t> indices, size_t& idx,
                                    int32_t& lastX, int32_t& lastY) {
    std::vector<Line> lines;

    for (; idx < indices.size(); idx++) {
        auto numPts = indices[idx];
        // polygon end marker
        if (numPts == 0) break;

        std::vector<Point> pts = parsePoints(it, numPts, lastX, lastY);

        if (pts.size() != numPts)
            logMsg("wrong number of points: %d / %d\n", pts.size(), numPts);

        lines.push_back(pts);
    }
    return lines;
}

static std::vector<Polygon> parsePolys(protobuf::message& it,
                                       std::vector<uint32_t> indices,
                                       size_t& idx) {
    std::vector<Polygon> polys;

    int32_t lastX = 0;
    int32_t lastY = 0;

    for (; idx < indices.size(); idx++) {
        auto numPts = indices[idx];
        if (numPts == 0) break;

        polys.push_back(parseLines(it, indices, idx, lastX, lastY));
    }
    return polys;
}

static void readUintArray(protobuf::message it, std::vector<uint32_t>& tags) {
    while (it.getData() < it.getEnd()) {
        uint32_t tag = it.int64();
        tags.emplace_back(tag);
    }
}

static std::string getKey(uint32_t k, const std::vector<std::string>& keys) {
    if (k < oscim::ATTRIB_OFFSET) {
        if (k < oscim::MAX_KEY) return oscim::keys[k];
    } else {
        k -= oscim::ATTRIB_OFFSET;
        if (k < keys.size()) return keys[k];
    }
    return "invalid";
}

static std::string getValue(uint32_t k, const std::vector<std::string>& vals) {
    if (k < oscim::ATTRIB_OFFSET) {
        if (k < oscim::MAX_VAL) return oscim::values[k];
    } else {
        k -= oscim::ATTRIB_OFFSET;
        if (k < vals.size()) return vals[k];
    }
    return "invalid";
}

static Feature* addFeature(TileData& tileData, const std::string& layerName) {
    for (auto& layer : tileData.layers) {
        if (layerName == layer.name) {
            layer.features.emplace_back();
            return &layer.features.back();
        }
    }

    tileData.layers.emplace_back(layerName);
    auto& layer = tileData.layers.back();

    layer.features.emplace_back();
    return &layer.features.back();
}

bool OpenScienceMapSource::extractFeature(
    protobuf::message it, GeometryType geomType, const std::vector<TagId>& tags,
    const std::vector<std::string>& keys, const std::vector<std::string>& vals,
    TileData& tileData, const MapTile& _tile) {
    uint32_t numIndices = 1;
    uint32_t numTags = 1;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> tagIds;
    std::vector<Point> coordinates;

    Feature* feature = nullptr;

    bool take = true;
    float lat = (2.0 * atan(exp((_tile.getOrigin().y / R_EARTH))) - PI * 0.5) *
                180 / M_PI;
    float heightScale = cos(lat * (PI / 180)) * EARTH_CIRCUMFERENCE /
      ((1 << _tile.getID().z) * 2);
    
    while (it.next()) {
        if (!take) {
            it.skip();
            continue;
        }

        switch (it.tag) {
            case ELEM_NUM_INDICES:
                numIndices = it.int64();
                break;

            case ELEM_NUM_TAGS:
                numTags = it.int64();
                tagIds.reserve(numTags);
                break;

            case ELEM_TAGS: {
                readUintArray(it.getMessage(), tagIds);

                for (auto i : tagIds) {
                    TagId t = tags[i];
                    auto k = getKey(t.key, keys);
                    auto v = getValue(t.val, vals);

                    // convert osm tags styled layers
                    if (k == "natural" && v == "water") {
                        feature = addFeature(tileData, "water");
                        break;
                    }
                    if (k == "highway") {
                        feature = addFeature(tileData, "roads");
                        if (v == "motorway" || v == "motorway_link" ||
                            v == "trunk" || v == "trunk_link" ||
                            v == "primary" || v == "primary_link")
                            feature->props.stringProps.emplace("kind",
                                                               "highway");
                        else
                            feature->props.stringProps.emplace("kind",
                                                               "minor_road");
                        break;
                    }
                    if (k == "building" || k == "roof") {
                        if (s3db)
                            feature = addFeature(tileData, "s3db");
                        else
                            feature = addFeature(tileData, "buildings");

                        feature->props.stringProps.emplace("kind", v);
                        break;
                    }
                    if (k == "landuse" || k == "natural") {
                        feature = addFeature(tileData, "landuse");

                        feature->props.stringProps.emplace("kind", v);
                        break;
                    }
                }
                if (feature) {
                    for (auto i : tagIds) {
                        TagId t = tags[i];
                        auto key = getKey(t.key, keys);
                        auto val = getValue(t.val, vals);

                        if (key == "height" || key == "mint_height") {
                            float numVal = atoi(val.c_str());
                            numVal *= _tile.getInverseScale();
                            numVal /= 100.0;
                            feature->props.numericProps.emplace(key, numVal);
                        } else {
                            feature->props.stringProps.emplace(key, val);
                        }
                    }
                } else {
                    take = false;
                }
                break;
            }

            case ELEM_INDICES:
                indices.reserve(numIndices);
                readUintArray(it.getMessage(), indices);
                break;

            case ELEM_COORDINATES: {
                if (!feature) {
                    it.skip();
                    break;
                }

                auto geom = it.getMessage();
                size_t curIdx = 0;
                int32_t lastX = 0;
                int32_t lastY = 0;

                if (geomType == LINES)
                    feature->lines =
                        parseLines(geom, indices, curIdx, lastX, lastY);
                else if (geomType == POLYGONS)
                    feature->polygons = parsePolys(geom, indices, curIdx);
                else if (geomType == POINTS) {
                    int numPts = indices.empty() ? 1 : indices[0];
                    feature->points = parsePoints(geom, numPts, lastX, lastY);
                } else if (geomType == TRIANGLES) {
                    feature->points = parsePoints3D(geom, heightScale);
                    // feature->triangles = std::vector<Triangle>();
                    // int *tris =  reinterpret_cast<int*>(indices.data());

                    feature->indices.insert(feature->indices.begin(),
                                            indices.begin(), indices.end());
                }

                feature->geometryType = geomType;

                break;
            }
            case ELEM_OSM_LAYER:
                it.skip();
                break;

            default:
                it.skip();
        }
    }

    if (take && !s3db) {
        feature = addFeature(tileData, "earth");
        feature->polygons.push_back(
            {{{-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}}});
    }

    return take;
}

void OpenScienceMapSource::readTags(protobuf::message it,
                                    std::vector<TagId>& tags) {
    while (it.getData() < it.getEnd()) {
        uint32_t key = it.varint();
        uint32_t val = it.varint();
        tags.emplace_back(key, val);
    }
}

#if TESTING
std::shared_ptr<TileData> OpenScienceMapSource::parseTest(
    std::stringstream& _in) {
#else

std::shared_ptr<TileData> OpenScienceMapSource::parse(const MapTile& _tile,
                                                      std::stringstream& _in) {
#endif

    double timeStart = getTime();

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();
    // if (_tile.getID().z > zmax || _tile.getID().z < zmin)
    //   return tileData;

    std::string buffer(std::istreambuf_iterator<char>(_in.rdbuf()),
                       (std::istreambuf_iterator<char>()));

    // skipping payload length header
    protobuf::message item(buffer.data() + 4, buffer.size() - 4);

    std::vector<std::string> keys;
    std::vector<std::string> vals;
    std::vector<TagId> tags;

    uint32_t numKeys = 0;
    uint32_t numVals = 0;
    uint32_t numTags = 0;
    GeometryType geomType;

    while (item.next()) {
        switch (item.tag) {
            case VERSION:
                item.skip();
                break;

            case TIMESTAMP:
                item.skip();
                break;

            case WATER_TILE:
                item.skip();
                break;

            case NUM_TAGS:
                numTags = item.int64();
                tags.reserve(numTags);
                break;

            case NUM_KEYS:
                numKeys = item.int64();
                keys.reserve(numKeys);
                break;

            case NUM_VALS:
                numVals = item.int64();
                vals.reserve(numVals);
                break;

            case KEYS:
                keys.emplace_back(item.string());
                break;

            case VALS:
                vals.emplace_back(item.string());
                break;

            case TAGS:
                readTags(item.getMessage(), tags);
                break;

            case ELEM_LINES:
                geomType = LINES;
            case ELEM_POLY:
                if (item.tag == ELEM_POLY) geomType = POLYGONS;
            case ELEM_POINT:
                if (item.tag == ELEM_POINT) geomType = POINTS;
            case ELEM_MESH:
                if (item.tag == ELEM_MESH) geomType = TRIANGLES;

                extractFeature(item.getMessage(), geomType, tags, keys, vals,
                               *tileData, _tile);
                break;
            default:
                item.skip();
        }
    }
    logMsg("PARSE TOOK %f", getTime() - timeStart);

    return tileData;
}
