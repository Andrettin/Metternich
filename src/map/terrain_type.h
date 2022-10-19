#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "map/terrain_adjacency.h"
#include "util/color_container.h"

namespace metternich {

class terrain_type final : public named_data_entry, public data_type<terrain_type>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color)
	Q_PROPERTY(std::filesystem::path image_filepath MEMBER image_filepath WRITE set_image_filepath)

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

	static void clear()
	{
		data_type::clear();
		terrain_type::terrain_types_by_color.clear();
	}

private:
	static inline color_map<terrain_type *> terrain_types_by_color;

public:
	explicit terrain_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

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

	const std::vector<int> &get_tiles() const
	{
		return this->tiles;
	}

	const std::vector<int> &get_adjacency_tiles(const terrain_adjacency &adjacency) const
	{
		const auto find_iterator = this->adjacency_tiles.find(adjacency);
		if (find_iterator != this->adjacency_tiles.end()) {
			return find_iterator->second;
		}

		return this->get_tiles();
	}

	void set_adjacency_tiles(const terrain_adjacency &adjacency, const std::vector<int> &tiles);

private:
	QColor color;
	std::filesystem::path image_filepath;
	std::vector<int> tiles;
	std::map<terrain_adjacency, std::vector<int>> adjacency_tiles;
};

}
