#ifndef TILEMAP_H
#define TILEMAP_H

#include <string>
#include <vector>
#include "pixel.hpp"
#include "graphics.hpp"

static const float PIXELS_PER_TILE = 16;
static const float UNITS_PER_TILE = PIXELS_PER_TILE / PIXELS_PER_UNIT;

struct TileLayer {
	std::vector<int> data;
	int height;
	int width;
	int xOffset;
	int yOffset;
	bool impassable;
};

class Tilemap {
public:
	Tilemap();
	~Tilemap();

	void LoadMap(std::string path);
	void DrawMap(Canvas canvas, int cameraX, int cameraY);

// private:
	std::vector<TileLayer> mapLayers;
	std::vector<Tileset> tilesets;
	std::vector<Vec2> enemySpawnPoints;
};

#endif // TILEMAP_H