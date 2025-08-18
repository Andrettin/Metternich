#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "map/terrain_adjacency.h"
#include "util/color_container.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;
enum class elevation_type;
enum class forestation_type;
enum class moisture_type;
enum class temperature_type;

class terrain_type final : public named_data_entry, public data_type<terrain_type>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(std::filesystem::path image_filepath MEMBER image_filepath WRITE set_image_filepath)
	Q_PROPERTY(bool water MEMBER water READ is_water NOTIFY changed)
	Q_PROPERTY(bool desert MEMBER desert READ is_desert NOTIFY changed)
	Q_PROPERTY(bool forest MEMBER forest READ is_forest NOTIFY changed)
	Q_PROPERTY(bool hills MEMBER hills READ is_hills NOTIFY changed)
	Q_PROPERTY(bool mountains MEMBER mountains READ is_mountains NOTIFY changed)
	Q_PROPERTY(bool wetland MEMBER wetland READ is_wetland NOTIFY changed)
	Q_PROPERTY(metternich::elevation_type elevation_type MEMBER elevation_type READ get_elevation_type NOTIFY changed)
	Q_PROPERTY(metternich::temperature_type temperature_type MEMBER temperature_type READ get_temperature_type NOTIFY changed)
	Q_PROPERTY(metternich::moisture_type moisture_type MEMBER moisture_type READ get_moisture_type NOTIFY changed)
	Q_PROPERTY(metternich::forestation_type forestation_type MEMBER forestation_type READ get_forestation_type NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "terrain_type";
	static constexpr const char property_class_identifier[] = "metternich::terrain_type*";
	static constexpr const char database_folder[] = "terrain_types";

	static terrain_type *get_by_color(const QColor &color)
	{
		terrain_type *terrain_type = terrain_type::try_get_by_color(color);

		if (terrain_type == nullptr) {
			throw std::runtime_error("No terrain type found for color: (" + std::to_string(color.red()) + ", " + std::to_string(color.green()) + ", " + std::to_string(color.blue()) + ").");
		}

		return terrain_type;
	}

	static terrain_type *try_get_by_color(const QColor &color)
	{
		const auto find_iterator = terrain_type::terrain_types_by_color.find(color);
		if (find_iterator != terrain_type::terrain_types_by_color.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static terrain_type *get_by_biome(const metternich::elevation_type elevation_type, const metternich::temperature_type temperature_type, const metternich::moisture_type moisture_type, const metternich::forestation_type forestation_type);
	static terrain_type *try_get_by_biome(const metternich::elevation_type elevation_type, const metternich::temperature_type temperature_type, const metternich::moisture_type moisture_type, const metternich::forestation_type forestation_type);

	static void clear()
	{
		data_type::clear();
		terrain_type::terrain_types_by_color.clear();
	}

private:
	static inline color_map<terrain_type *> terrain_types_by_color;
	static inline std::map<elevation_type, std::map<temperature_type, std::map<moisture_type, std::map<forestation_type, terrain_type *>>>> terrain_types_by_biome;

public:
	explicit terrain_type(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const QColor &get_color() const
	{
		return this->color;
	}

	void set_color(const QColor &color)
	{
		if (color == this->get_color()) {
			return;
		}

		if (terrain_type::try_get_by_color(color) != nullptr) {
			throw std::runtime_error("Color is already used by another terrain type.");
		}

		this->color = color;
		terrain_type::terrain_types_by_color[color] = this;
	}
	
	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const std::filesystem::path &get_image_filepath() const
	{
		return this->image_filepath;
	}

	void set_image_filepath(const std::filesystem::path &filepath);

	bool is_water() const
	{
		return this->water;
	}

	bool is_desert() const
	{
		return this->desert;
	}

	bool is_forest() const
	{
		return this->forest;
	}

	bool is_hills() const
	{
		return this->hills;
	}

	bool is_mountains() const
	{
		return this->mountains;
	}

	bool is_wetland() const
	{
		return this->wetland;
	}

	elevation_type get_elevation_type() const
	{
		return this->elevation_type;
	}

	temperature_type get_temperature_type() const
	{
		return this->temperature_type;
	}

	moisture_type get_moisture_type() const
	{
		return this->moisture_type;
	}

	forestation_type get_forestation_type() const
	{
		return this->forestation_type;
	}

	void assign_to_biome(const elevation_type elevation_type, const temperature_type temperature_type, const moisture_type moisture_type, const forestation_type forestation_type);

	const std::vector<const terrain_type *> &get_fallback_terrains() const
	{
		return this->fallback_terrains;
	}

	const std::vector<int> &get_tiles() const
	{
		return this->tiles;
	}

	bool has_adjacency_tiles() const
	{
		return !this->adjacency_tiles.empty();
	}

	const std::vector<int> &get_adjacency_tiles(const terrain_adjacency &adjacency) const
	{
		const auto find_iterator = this->adjacency_tiles.find(adjacency);
		if (find_iterator != this->adjacency_tiles.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("Failed to get adjacency tiles for a given terrain adjacency for the \"" + this->get_identifier() + "\" terrain type.");
	}

	void set_adjacency_tiles(const terrain_adjacency &adjacency, const std::vector<int> &tiles);

	const std::vector<int> &get_subtiles() const
	{
		return this->subtiles;
	}

	bool has_adjacency_subtiles() const
	{
		return !this->adjacency_subtiles.empty();
	}

	const std::vector<int> &get_adjacency_subtiles(const terrain_adjacency &adjacency) const
	{
		const auto find_iterator = this->adjacency_subtiles.find(adjacency);
		if (find_iterator != this->adjacency_subtiles.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error(std::format("Failed to get adjacency subtiles for a given terrain adjacency for the \"{}\" terrain type.", this->get_identifier()));
	}

	void set_adjacency_subtiles(const terrain_adjacency &adjacency, const std::vector<int> &subtiles);

signals:
	void changed();

private:
	QColor color;
	const metternich::icon *icon = nullptr;
	std::filesystem::path image_filepath;
	bool water = false;
	bool desert = false;
	bool forest = false;
	bool hills = false;
	bool mountains = false;
	bool wetland = false;
	metternich::elevation_type elevation_type;
	metternich::temperature_type temperature_type;
	metternich::moisture_type moisture_type;
	metternich::forestation_type forestation_type;
	std::vector<const terrain_type *> fallback_terrains;
	std::vector<int> tiles;
	std::map<terrain_adjacency, std::vector<int>> adjacency_tiles;
	std::vector<int> subtiles;
	std::map<terrain_adjacency, std::vector<int>> adjacency_subtiles;
};

}
