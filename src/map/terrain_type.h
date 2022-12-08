#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "map/terrain_adjacency.h"
#include "util/color_container.h"

namespace metternich {

enum class elevation_type;
enum class forestation_type;
enum class temperature_type;

class terrain_type final : public named_data_entry, public data_type<terrain_type>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color NOTIFY changed)
	Q_PROPERTY(std::filesystem::path image_filepath MEMBER image_filepath WRITE set_image_filepath)
	Q_PROPERTY(bool water MEMBER water READ is_water NOTIFY changed)
	Q_PROPERTY(metternich::elevation_type elevation_type MEMBER elevation_type READ get_elevation_type NOTIFY changed)
	Q_PROPERTY(metternich::forestation_type forestation_type MEMBER forestation_type READ get_forestation_type NOTIFY changed)
	Q_PROPERTY(metternich::temperature_type temperature_type MEMBER temperature_type READ get_temperature_type NOTIFY changed)

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

	static terrain_type *get_by_biome(const metternich::elevation_type elevation_type, const metternich::temperature_type temperature_type, const metternich::forestation_type forestation_type)
	{
		terrain_type *terrain_type = terrain_type::try_get_by_biome(elevation_type, temperature_type, forestation_type);

		if (terrain_type == nullptr) {
			throw std::runtime_error("No terrain type found for biome.");
		}

		return terrain_type;
	}

	static terrain_type *try_get_by_biome(const metternich::elevation_type elevation_type, const metternich::temperature_type temperature_type, const metternich::forestation_type forestation_type);

	static void clear()
	{
		data_type::clear();
		terrain_type::terrain_types_by_color.clear();
	}

private:
	static inline color_map<terrain_type *> terrain_types_by_color;
	static inline std::map<elevation_type, std::map<temperature_type, std::map<forestation_type, terrain_type *>>> terrain_types_by_biome;

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

	const std::filesystem::path &get_image_filepath() const
	{
		return this->image_filepath;
	}

	void set_image_filepath(const std::filesystem::path &filepath);
	
	bool is_water() const
	{
		return this->water;
	}

	elevation_type get_elevation_type() const
	{
		return this->elevation_type;
	}

	forestation_type get_forestation_type() const
	{
		return this->forestation_type;
	}

	temperature_type get_temperature_type() const
	{
		return this->temperature_type;
	}

	void assign_to_biome(const elevation_type elevation_type, const temperature_type temperature_type, const forestation_type forestation_type);

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

signals:
	void changed();

private:
	QColor color;
	std::filesystem::path image_filepath;
	bool water = false;
	metternich::elevation_type elevation_type;
	metternich::forestation_type forestation_type;
	metternich::temperature_type temperature_type;
	std::vector<int> tiles;
	std::map<terrain_adjacency, std::vector<int>> adjacency_tiles;
};

}
