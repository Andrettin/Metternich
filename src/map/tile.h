#pragma once

namespace metternich {

class terrain_type;

class tile final
{
public:
	explicit tile(const terrain_type *base_terrain, const terrain_type *terrain);

	const terrain_type *get_terrain() const
	{
		return this->terrain;
	}

	void set_terrain(const terrain_type *terrain);

	short get_base_tile() const
	{
		return this->base_tile_frame;
	}

	short get_tile() const
	{
		return this->tile_frame;
	}

private:
	const terrain_type *terrain = nullptr;
	short base_tile_frame = 0;
	short tile_frame = 0;
};

}