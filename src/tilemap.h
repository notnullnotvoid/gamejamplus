#ifndef TILEMAP_H
#define TILEMAP_H

#include <iostream>
#include <vector>
#include "pixel.hpp"

struct TileLayer {
	std::vector<int> data;
	int height;
	int width;
	int xOffset;
	int yOffset;
};
/*
struct TileSet {
	Texture texture;
	int columns;
	int tileCount;
	int firstId;
};
*/
class Tilemap {
public:
	Tilemap();
	~Tilemap();

	void LoadMap(std::string path);
	void DrawMap(Canvas canvas, int cameraX, int cameraY);

private:
	std::vector<TileLayer> mapLayers;
	std::vector<Tileset> tilesets;
};

#endif // TILEMAP_H