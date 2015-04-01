#pragma once

#include "dataSource.h"
#include "mapTile.h"
#include "tileData.h"

#ifdef TESTING

class OpenScienceMapSource  {

 public:
  std::shared_ptr<TileData> parseTest(std::stringstream &_in);

#else

class OpenScienceMapSource : public NetworkDataSource {

 protected:
  virtual std::shared_ptr<TileData> parse(const MapTile& _tile,
      std::stringstream &_in) override;

#endif

public:
  OpenScienceMapSource();
};
