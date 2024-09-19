#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"

Q_MOC_INCLUDE("economy/resource.h")
Q_MOC_INCLUDE("population/profession.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class commodity;
class icon;
class population_type;
class profession;
class resource;
class technology;
class terrain_type;
class tile;
enum class improvement_slot;

template <typename scope_type>
class modifier;

//tile improvement
class improvement final : public named_data_entry, public data_type<improvement>
{
	Q_OBJECT

	Q_PROPERTY(metternich::improvement_slot slot MEMBER slot READ get_slot NOTIFY changed)
	Q_PROPERTY(metternich::resource* resource MEMBER resource NOTIFY changed)
	Q_PROPERTY(bool ruins MEMBER ruins READ is_ruins NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(std::filesystem::path image_filepath MEMBER image_filepath WRITE set_image_filepath)
	Q_PROPERTY(int output_multiplier MEMBER output_multiplier READ get_output_multiplier NOTIFY changed)
	Q_PROPERTY(const metternich::profession* employment_profession MEMBER employment_profession READ get_employment_profession NOTIFY changed)
	Q_PROPERTY(int employment_capacity MEMBER employment_capacity READ get_employment_capacity NOTIFY changed)
	Q_PROPERTY(int variation_count MEMBER variation_count READ get_variation_count)
	Q_PROPERTY(metternich::improvement* required_improvement MEMBER required_improvement NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(int wealth_cost MEMBER wealth_cost READ get_wealth_cost NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "improvement";
	static constexpr const char property_class_identifier[] = "metternich::improvement*";
	static constexpr const char database_folder[] = "improvements";

	explicit improvement(const std::string &identifier);
	~improvement();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	improvement_slot get_slot() const
	{
		return this->slot;
	}

	int get_level() const
	{
		return this->level;
	}

	void calculate_level();

	const metternich::resource *get_resource() const
	{
		return this->resource;
	}

	bool is_ruins() const
	{
		return this->ruins;
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

	const std::filesystem::path &get_terrain_image_filepath(const terrain_type *terrain) const
	{
		const auto find_iterator = this->terrain_image_filepaths.find(terrain);
		if (find_iterator != this->terrain_image_filepaths.end()) {
			return find_iterator->second;
		}

		return this->get_image_filepath();
	}

	bool has_terrain_image_filepath(const terrain_type *terrain) const
	{
		return this->terrain_image_filepaths.contains(terrain);
	}

	const commodity *get_output_commodity() const;

	int get_output_multiplier() const
	{
		return this->output_multiplier;
	}

	const profession *get_employment_profession() const
	{
		return this->employment_profession;
	}

	int get_employment_capacity() const
	{
		return this->employment_capacity;
	}

	const std::vector<const terrain_type *> &get_terrain_types() const
	{
		return this->terrain_types;
	}

	int get_variation_count() const
	{
		return this->variation_count;
	}

	const improvement *get_required_improvement() const
	{
		return this->required_improvement;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	int get_wealth_cost() const
	{
		return this->wealth_cost;
	}

	const commodity_map<int> &get_commodity_costs() const
	{
		return this->commodity_costs;
	}

	const modifier<const site> *get_modifier() const
	{
		return this->modifier.get();
	}

	bool is_buildable_on_site(const site *site) const;

signals:
	void changed();

private:
	improvement_slot slot{};
	int level = 0;
	metternich::resource *resource = nullptr; //the resource for which this improvement can be built
	bool ruins = false; //if true, this improvement can be explored by troops, yielding some bonus (or malus)
	const metternich::icon *icon = nullptr;
	std::filesystem::path image_filepath;
	std::map<const terrain_type *, std::filesystem::path> terrain_image_filepaths;
	int output_multiplier = 0;
	const profession *employment_profession = nullptr;
	int employment_capacity = 0;
	std::vector<const terrain_type *> terrain_types; //the terrain types where the improvement can be built
	int variation_count = 1;
	improvement *required_improvement = nullptr;
	technology *required_technology = nullptr;
	int wealth_cost = 0;
	commodity_map<int> commodity_costs;
	std::unique_ptr<modifier<const site>> modifier;
};

}
