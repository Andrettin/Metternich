#pragma once

namespace archimedes {
	enum class direction;
}

namespace metternich {

class country;
class province;
class resource;
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

	void set_tile(const short tile)
	{
		this->tile_frame = tile;
	}

	const metternich::province *get_province() const
	{
		return this->province;
	}

	void set_province(const metternich::province *province)
	{
		this->province = province;
	}

	const country *get_owner() const;

	const metternich::site *get_site() const
	{
		return this->site;
	}

	void set_site(const metternich::site *site)
	{
		this->site = site;
	}

	const metternich::site *get_settlement() const;

	const metternich::resource *get_resource() const
	{
		return this->resource;
	}

	void set_resource(const metternich::resource *resource)
	{
		this->resource = resource;
	}

	int get_development_level() const
	{
		return this->development_level;
	}

	void set_development_level(const int level);

	const std::vector<direction> &get_border_directions() const
	{
		return this->border_directions;
	}

	void add_border_direction(const direction direction)
	{
		this->border_directions.push_back(direction);
	}

	void sort_border_directions()
	{
		std::sort(this->border_directions.begin(), this->border_directions.end());
	}

	bool has_graphical_border() const
	{
		return !this->get_border_directions().empty();
	}

private:
	const terrain_type *terrain = nullptr;
	short base_tile_frame = 0;
	short tile_frame = 0;
	const metternich::province *province = nullptr;
	const metternich::site *site = nullptr;
	const metternich::resource *resource = nullptr;
	int development_level = 0;
	std::vector<direction> border_directions; //used for graphical borders; this does not include e.g. borders with water tiles for land ones
};

}
