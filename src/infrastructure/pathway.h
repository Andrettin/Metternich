#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "map/terrain_type_container.h"

Q_MOC_INCLUDE("technology/technology.h")

namespace archimedes {
	enum class direction;
}

namespace metternich {

class technology;
class tile;

class pathway final : public named_data_entry, public data_type<pathway>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path image_filepath MEMBER image_filepath WRITE set_image_filepath)
	Q_PROPERTY(int transport_level MEMBER transport_level READ get_transport_level NOTIFY changed)
	Q_PROPERTY(metternich::pathway* required_pathway MEMBER required_pathway NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* river_crossing_required_technology MEMBER river_crossing_required_technology NOTIFY changed)
	Q_PROPERTY(int wealth_cost MEMBER wealth_cost READ get_wealth_cost NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "pathway";
	static constexpr const char property_class_identifier[] = "metternich::pathway*";
	static constexpr const char database_folder[] = "pathways";

	explicit pathway(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const std::filesystem::path &get_image_filepath() const
	{
		return this->image_filepath;
	}

	void set_image_filepath(const std::filesystem::path &filepath);

	int get_transport_level() const
	{
		return this->transport_level;
	}

	const pathway *get_required_pathway() const
	{
		return this->required_pathway;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const technology *get_river_crossing_required_technology() const
	{
		return this->river_crossing_required_technology;
	}

	const technology *get_terrain_required_technology(const terrain_type *terrain) const
	{
		const auto find_iterator = this->terrain_required_technologies.find(terrain);
		if (find_iterator != this->terrain_required_technologies.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	int get_wealth_cost() const
	{
		return this->wealth_cost;
	}

	const commodity_map<int> &get_commodity_costs() const
	{
		return this->commodity_costs;
	}

	bool is_buildable_on_tile(const tile *tile, const direction direction) const;

signals:
	void changed();

private:
	std::filesystem::path image_filepath;
	int transport_level = 0;
	pathway *required_pathway = nullptr;
	technology *required_technology = nullptr;
	technology *river_crossing_required_technology = nullptr;
	terrain_type_map<const technology *> terrain_required_technologies;
	int wealth_cost = 0;
	commodity_map<int> commodity_costs;
};

}
