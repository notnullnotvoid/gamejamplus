#include <fstream>
#include "tilemap.h"
#include "level.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

Tilemap::Tilemap() {}
Tilemap::~Tilemap() {}


void Tilemap::LoadMap(std::string path) {
	
	std::ifstream mapFile(path);
	json j;
	if (mapFile >> j) {
		auto layers = j["layers"];
		
		for (int i = 0; i < layers.size(); i++) {
			json l = layers[i];
			auto data = l["data"];
			int h = l["height"];
			int w = l["width"];
			int x = l["x"];
			int y = l["y"];

			std::cout << h << "," << w << "," << x << "," << y << "," << data.size() << std::endl;
		}
	}
	else {
		std::cout << "Could not read mapfile\n";
	}
	/*
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			mapFile.get(tile);
			Level::AddTile(atoi(&tile), x * 16, y * 16);
			mapFile.ignore();
		}
	}

	mapFile.close();
	*/
}