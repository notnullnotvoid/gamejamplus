#ifndef TILEMAP_H
#define TILEMAP_H

#include <iostream>

class Tilemap {
public:
	Tilemap();
	~Tilemap();

	void LoadMap(std::string path);
};
#endif // TILEMAP_H