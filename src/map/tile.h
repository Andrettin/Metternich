#pragma once

#include "economy/commodity_container.h"
#include "infrastructure/pathway_container.h"
#include "util/centesimal_int.h"

namespace archimedes {
	enum class direction;
}

namespace metternich {

class civilian_unit;
class domain;
class holding_type;
class improvement;
class pathway;
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

	const metternich::province *get_province() const
	{
		return this->province;
	}

	void set_province(const metternich::province *province)
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

	const pathway *get_direction_pathway(const direction direction) const
	{
		return this->direction_pathways[static_cast<int>(direction)];
	}

	void set_direction_pathway(const direction direction, const pathway *pathway);

	bool has_route() const
	{
		for (const metternich::pathway *direction_pathway : this->direction_pathways) {
			if (direction_pathway != nullptr) {
				return true;
			}
		}

		return false;
	}

	bool has_pathway(const pathway *pathway) const
	{
		for (const metternich::pathway *direction_pathway : this->direction_pathways) {
			if (direction_pathway == pathway) {
				return true;
			}
		}

		return false;
	}

	const pathway *get_best_pathway() const;

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

	const std::vector<direction> &get_country_border_directions() const
	{
		return this->country_border_directions;
	}

	void add_country_border_direction(const direction direction)
	{
		this->country_border_directions.push_back(direction);
	}

	void clear_country_border_directions()
	{
		this->country_border_directions.clear();
	}

	bool has_graphical_country_border() const
	{
		return !this->get_country_border_directions().empty();
	}
	
	metternich::civilian_unit *get_civilian_unit() const
	{
		return this->civilian_unit;
	}

	void set_civilian_unit(metternich::civilian_unit *civilian_unit)
	{
		this->civilian_unit = civilian_unit;
	}

	const commodity_map<centesimal_int> &get_commodity_outputs() const;
	void calculate_commodity_outputs();

	bool produces_commodity(const commodity *commodity) const;

private:
	const terrain_type *terrain = nullptr;
	const metternich::province *province = nullptr;
	const metternich::site *site = nullptr;
	int8_t improvement_variation = 0;
	bool inner_river = false; //whether the tile has an in-tile river
	std::vector<direction> river_directions;
	std::array<const pathway *, 8> direction_pathways{};
	std::vector<direction> border_directions; //used for graphical borders; this does not include e.g. borders with water tiles for land ones
	std::vector<direction> country_border_directions;
	metternich::civilian_unit *civilian_unit = nullptr;
};

}
