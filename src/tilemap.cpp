#include <fstream>
#include "tilemap.h"

#include "nlohmann/json.hpp"

using json = nlohmann::json;

Tilemap::Tilemap() {}
Tilemap::~Tilemap() {}

std::vector<TileLayer> mapLayers;
std::vector<Tileset> tilesets;

void Tilemap::LoadMap(std::string path) {
	
	std::ifstream mapFile(path);
	json j;
	if (mapFile >> j) {

		auto readLayers = j["layers"];
		for (int i = 0; i < readLayers.size(); i++) {
			json l = readLayers[i];

			TileLayer layer = {	};
			layer.data = l["data"].get<std::vector<int>>();
			layer.height = l["height"];
			layer.width = l["width"];
			layer.xOffset = l["x"];
			layer.yOffset = l["y"];
			mapLayers.emplace_back(layer);
		}

		auto readTilesets = j["tilesets"];
		for (int i = 0; i < readTilesets.size(); i++) {
			json t = readTilesets[i];

			std::string imgName = t["name"];
			std::string fullName = "res/" + imgName; // manually added file extension to value; isn't native to the tiled output

			Tileset set = load_tileset(&fullName[0], 16, 16);
			tilesets.emplace_back(set);
		}
	}
	else {
		std::cout << "Could not read mapfile\n";
	}
}

void Tilemap::DrawMap(Canvas canvas, int cameraX, int cameraY) {

	for (int i = 0; i < mapLayers.size(); i++) {
		auto d = mapLayers[i].data;
		int row = 0;
		int col = 0;

		for (int j = 0; j < d.size(); j++) {
			int id = d[j];

			if (id != 0) {
				// assuming 1 tileset
				int tsX = (id - 1) % tilesets[0].width;
				int tsY = (id - 1) / tilesets[0].width;

				draw_tile(canvas, tilesets[0], tsX, tsY, col * 16, row * 16);
			}


			col++;
			if (col == mapLayers[i].width) {
				col = 0;
				row++;
			}
		}
	}
}