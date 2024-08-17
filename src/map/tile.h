#pragma once

#include "economy/commodity_container.h"
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
class settlement_type;
class site;
class terrain_type;

class tile final
{
public:
	static constexpr int max_transport_level = 4;

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

	const std::array<short, 4> &get_base_subtiles() const
	{
		return this->base_subtile_frames;
	}

	short get_tile() const
	{
		return this->tile_frame;
	}

	void set_tile(const short tile)
	{
		this->tile_frame = tile;
	}

	const std::array<short, 4> &get_subtiles() const
	{
		return this->subtile_frames;
	}

	void set_subtile(const size_t index, const short subtile)
	{
		this->subtile_frames[index] = subtile;
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
	const settlement_type *get_settlement_type() const;

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

	const std::array<short, 4> &get_river_subtile_frames() const
	{
		return this->river_subtile_frames;
	}

	void set_river_subtile_frame(const size_t index, const short frame)
	{
		this->river_subtile_frames[index] = frame;
	}

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

	int get_transport_level() const
	{
		return this->transport_level;
	}

	void set_transport_level(const int level)
	{
		this->transport_level = level;
	}

	int get_sea_transport_level() const
	{
		return this->sea_transport_level;
	}

	void set_sea_transport_level(const int level)
	{
		this->sea_transport_level = level;
	}

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
	short base_tile_frame = 0;
	std::array<short, 4> base_subtile_frames;
	short tile_frame = 0;
	std::array<short, 4> subtile_frames;
	const metternich::province *province = nullptr;
	const metternich::site *site = nullptr;
	int8_t improvement_variation = 0;
	bool inner_river = false; //whether the tile has an in-tile river
	std::vector<direction> river_directions;
	std::array<short, 4> river_subtile_frames{ -1, -1, -1, -1 };
	std::array<const pathway *, 8> direction_pathways{};
	pathway_map<short> pathway_frames;
	int transport_level = 0;
	int sea_transport_level = 0;
	std::vector<direction> border_directions; //used for graphical borders; this does not include e.g. borders with water tiles for land ones
	std::vector<direction> country_border_directions;
	metternich::civilian_unit *civilian_unit = nullptr;
};

}
