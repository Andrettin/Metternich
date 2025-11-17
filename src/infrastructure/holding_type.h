#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"

Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class icon;
class population_class;
class population_type;
class portrait;
class site;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class holding_type final : public named_data_entry, public data_type<holding_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(std::filesystem::path image_filepath MEMBER image_filepath WRITE set_image_filepath NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "holding_type";
	static constexpr const char property_class_identifier[] = "metternich::holding_type*";
	static constexpr const char database_folder[] = "holding_types";

	static const std::set<std::string> database_dependencies;

	explicit holding_type(const std::string &identifier);
	~holding_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const std::filesystem::path &get_image_filepath() const
	{
		return this->image_filepath;
	}

	void set_image_filepath(const std::filesystem::path &filepath);

	const commodity_map<int> &get_level_commodity_costs() const
	{
		return this->level_commodity_costs;
	}

	const commodity_map<int> &get_level_commodity_costs_per_level() const
	{
		return this->level_commodity_costs_per_level;
	}

	const std::string &get_level_name(const int level) const;

	int get_tier_level(const std::string &tier) const
	{
		const auto find_iterator = this->tier_levels.find(tier);

		if (find_iterator != this->tier_levels.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error(std::format("Invalid tier for holding type \"{}\": \"{}\".", this->get_identifier(), tier));
	}

	const std::vector<const holding_type *> &get_base_holding_types() const
	{
		return this->base_holding_types;
	}

	const std::vector<const holding_type *> &get_upgraded_holding_types() const
	{
		return this->upgraded_holding_types;
	}

	int get_level() const
	{
		return this->level;
	}

	void calculate_level();

	const std::vector<const population_class *> &get_population_classes() const
	{
		return this->population_classes;
	}

	bool can_have_population_type(const population_type *population_type) const;

	const and_condition<site> *get_conditions() const
	{
		return this->conditions.get();
	}

	const and_condition<site> *get_build_conditions() const
	{
		return this->build_conditions.get();
	}

	const modifier<const site> *get_modifier() const
	{
		return this->modifier.get();
	}

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	const metternich::portrait *portrait = nullptr;
	std::filesystem::path image_filepath;
	commodity_map<int> level_commodity_costs;
	commodity_map<int> level_commodity_costs_per_level;
	std::map<int, std::string> level_names;
	std::map<std::string, int> tier_levels; //identifiers for particular levels
	std::vector<const holding_type *> base_holding_types;
	std::vector<const holding_type *> upgraded_holding_types;
	int level = 0;
	std::vector<const population_class *> population_classes;
	std::unique_ptr<const and_condition<site>> conditions;
	std::unique_ptr<const and_condition<site>> build_conditions;
	std::unique_ptr<modifier<const site>> modifier;
};

}
