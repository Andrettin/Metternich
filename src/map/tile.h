#pragma once

namespace metternich {

class province;
class site;
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

	const metternich::province *get_province() const
	{
		return this->province;
	}

	void set_province(const metternich::province *province)
	{
		this->province = province;
	}

	const site *get_settlement() const
	{
		return this->settlement;
	}

	void set_settlement(const site *settlement)
	{
		this->settlement = settlement;
	}

private:
	const terrain_type *terrain = nullptr;
	short base_tile_frame = 0;
	short tile_frame = 0;
	const metternich::province *province = nullptr;
	const site *settlement = nullptr;
};

}
