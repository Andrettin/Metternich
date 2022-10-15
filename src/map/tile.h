#pragma once

namespace metternich {

class commodity;
class country;
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

	const commodity *get_resource() const
	{
		return this->resource;
	}

	void set_resource(const commodity *resource)
	{
		this->resource = resource;
	}

	int get_development_level() const
	{
		return this->development_level;
	}

	void set_development_level(const int level);

private:
	const terrain_type *terrain = nullptr;
	short base_tile_frame = 0;
	short tile_frame = 0;
	const metternich::province *province = nullptr;
	const metternich::site *site = nullptr;
	const commodity *resource = nullptr;
	int development_level = 0;
};

}
