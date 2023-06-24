#pragma once

#include "infrastructure/pathway_container.h"

namespace archimedes {
	enum class direction;
}

namespace metternich {

class civilian_unit;
class country;
class improvement;
class pathway;
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

	void set_resource(const metternich::resource *resource);

	bool is_resource_discovered() const
	{
		return this->resource_discovered;
	}

	void set_resource_discovered(const bool discovered)
	{
		this->resource_discovered = discovered;
	}

	const metternich::improvement *get_improvement() const
	{
		return this->improvement;
	}

	void set_improvement(const metternich::improvement *improvement);

	short get_improvement_variation() const
	{
		return this->improvement_variation;
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
		return !this->get_river_directions().empty();
	}

	short get_river_frame() const
	{
		return this->river_frame;
	}

	void set_river_frame(const short frame)
	{
		this->river_frame = frame;
	}

	bool is_river_crossing_direction(const direction direction) const;

	const pathway *get_direction_pathway(const direction direction) const
	{
		return this->direction_pathways[static_cast<int>(direction)];
	}

	void set_direction_pathway(const direction direction, const pathway *pathway);

	bool has_route() const
	{
		return !this->pathway_frames.empty();
	}

	const pathway_map<short> &get_pathway_frames() const
	{
		return this->pathway_frames;
	}

	short get_pathway_frame(const pathway *pathway) const
	{
		const auto find_iterator = this->pathway_frames.find(pathway);
		if (find_iterator != this->pathway_frames.end()) {
			return find_iterator->second;
		}

		return -1;
	}

	void set_pathway_frame(const pathway *pathway, const short frame)
	{
		if (frame == -1) {
			this->pathway_frames.erase(pathway);
		} else {
			this->pathway_frames[pathway] = frame;
		}
	}

	void calculate_pathway_frame(const pathway *pathway);
	void calculate_pathway_frames();

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

	int get_output_value() const;

private:
	const terrain_type *terrain = nullptr;
	short base_tile_frame = 0;
	short tile_frame = 0;
	const metternich::province *province = nullptr;
	const metternich::site *site = nullptr;
	const metternich::resource *resource = nullptr;
	bool resource_discovered = false;
	const metternich::improvement *improvement = nullptr;
	int8_t improvement_variation = 0;
	std::vector<direction> river_directions;
	short river_frame = -1;
	std::array<const pathway *, 8> direction_pathways{};
	pathway_map<short> pathway_frames;
	std::vector<direction> border_directions; //used for graphical borders; this does not include e.g. borders with water tiles for land ones
	std::vector<direction> country_border_directions;
	metternich::civilian_unit *civilian_unit = nullptr;
};

}
