#include <iostream>
#include <fstream>
#include "tilemap.h"

#include "nlohmann/json.hpp"

using json = nlohmann::json;

Tilemap::Tilemap() {}
Tilemap::~Tilemap() {}

void Tilemap::LoadMap(std::string path) {
	
	std::ifstream mapFile(path);
	json j;
	if (mapFile >> j) {

		auto readLayers = j["layers"];
		for (int i = 0; i < readLayers.size(); i++) {
			json l = readLayers[i];

			// Map layers
			if (l.contains("data")) {
				TileLayer layer = {	};
				layer.data = l["data"].get<std::vector<int>>();
				layer.height = l["height"];
				layer.width = l["width"];
				layer.xOffset = l["x"];
				layer.yOffset = l["y"];

				if (l.contains("properties")) {
					json props = l["properties"];

					for (auto &p : props) {
						std::string name = p["name"];
						if (name == "impassable") {
							layer.impassable = p["value"];
							continue;
						}
					}
				}

				mapLayers.emplace_back(layer);
			}

			else if (l.contains("objects")) {
				for (auto &obj : l["objects"]) {
					Vec2 pos{};
					pos.x = obj["x"];
					pos.y = obj["y"];
					enemySpawnPoints.emplace_back(pos);
				}
			}
		}

		auto readTilesets = j["tilesets"];
		for (int i = 0; i < readTilesets.size(); i++) {
			json t = readTilesets[i];

			std::string imgName = t["image"];
			std::string fullName = "res/" + imgName; // make sure that you're embedding the tileset into the map so it passes the correct file name!

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

				draw_tile(canvas, tilesets[0], tsX, tsY, col * 16 - cameraX, row * 16 - cameraY);
			}

			col++;
			if (col == mapLayers[i].width) {
				col = 0;
				row++;
			}
		}
	}
}