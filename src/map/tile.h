#pragma once

#include "economy/commodity_container.h"
#include "util/centesimal_int.h"

namespace archimedes {
	enum class direction;
}

namespace metternich {

class civilian_unit;
class domain;
class holding_type;
class improvement;
class province;
class resource;
class site;
class terrain_type;

class tile final
{
public:
	explicit tile(const terrain_type *terrain);

	const terrain_type *get_terrain() const
	{
		return this->terrain;
	}

	void set_terrain(const terrain_type *terrain);

	metternich::province *get_province() const
	{
		return this->province;
	}

	void set_province(metternich::province *province)
	{
		this->province = province;
	}

	const domain *get_owner() const;

	const metternich::site *get_site() const
	{
		return this->site;
	}

	void set_site(const metternich::site *site);

	const metternich::site *get_settlement() const;
	const holding_type *get_holding_type() const;

	const metternich::resource *get_resource() const;
	bool is_resource_discovered() const;

	void on_main_improvement_changed();

	short get_improvement_variation() const
	{
		return this->improvement_variation;
	}

	void clear_improvement_variation()
	{
		this->improvement_variation = 0;
	}

	bool has_inner_river() const
	{
		return this->inner_river;
	}

	void set_inner_river(const bool value)
	{
		this->inner_river = value;
	}

	const std::vector<direction> &get_river_directions() const
	{
		return this->river_directions;
	}

	void add_river_direction(const direction direction);

	void sort_river_directions()
	{
		std::sort(this->river_directions.begin(), this->river_directions.end());
	}

	bool has_river() const
	{
		return this->has_inner_river() || !this->get_river_directions().empty();
	}

	bool is_river_crossing_direction(const direction direction) const;

	const std::vector<civilian_unit *> &get_civilian_units() const
	{
		return this->civilian_units;
	}

	void add_civilian_unit(civilian_unit *civilian_unit);
	void remove_civilian_unit(civilian_unit *civilian_unit);

	void clear_civilian_units()
	{
		this->civilian_units.clear();
	}

	const commodity_map<centesimal_int> &get_commodity_outputs() const;
	void calculate_commodity_outputs();

	bool produces_commodity(const commodity *commodity) const;

private:
	const terrain_type *terrain = nullptr;
	metternich::province *province = nullptr;
	const metternich::site *site = nullptr;
	int8_t improvement_variation = 0;
	bool inner_river = false; //whether the tile has an in-tile river
	std::vector<direction> river_directions;
	std::vector<civilian_unit *> civilian_units;
};

}
