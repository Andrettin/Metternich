#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "map/site_container.h"
#include "util/decimillesimal_int.h"

Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class building_type;
class character;
class domain;
class improvement;
class portrait;
class province;
class site;
class technology;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class journal_entry final : public named_data_entry, public data_type<journal_entry>
{
	Q_OBJECT

	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(archimedes::decimillesimal_int completion_random_chance MEMBER completion_random_chance READ get_completion_random_chance)

public:
	static constexpr const char class_identifier[] = "journal_entry";
	static constexpr const char property_class_identifier[] = "metternich::journal_entry*";
	static constexpr const char database_folder[] = "journal_entries";
	static constexpr int ai_building_desire_modifier = 100;
	static constexpr int ai_technology_desire_modifier = 100;
	static constexpr int ai_advisor_desire_modifier = 1000;
	static constexpr int ai_leader_desire_modifier = 1000;

	explicit journal_entry(const std::string &identifier);
	~journal_entry();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const std::string &get_description() const
	{
		return this->description;
	}

	Q_INVOKABLE void set_description(const std::string &description)
	{
		this->description = description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	bool check_preconditions(const domain *domain) const;
	bool check_conditions(const domain *domain) const;
	bool check_completion_conditions(const domain *domain, const bool ignore_random_chance) const;
	Q_INVOKABLE QString get_completion_conditions_string() const;

	const decimillesimal_int &get_completion_random_chance() const
	{
		return this->completion_random_chance;
	}

	const and_condition<domain> *get_failure_conditions() const
	{
		return this->failure_conditions.get();
	}

	Q_INVOKABLE QString get_failure_conditions_string() const;

	const effect_list<const domain> *get_completion_effects() const
	{
		return this->completion_effects.get();
	}

	Q_INVOKABLE QString get_completion_effects_string(metternich::domain *domain) const;

	const effect_list<const domain> *get_failure_effects() const
	{
		return this->failure_effects.get();
	}

	Q_INVOKABLE QString get_failure_effects_string(metternich::domain *domain) const;

	const modifier<const domain> *get_active_modifier() const
	{
		return this->active_modifier.get();
	}

	const modifier<const domain> *get_completion_modifier() const
	{
		return this->completion_modifier.get();
	}

	const std::vector<const building_type *> &get_built_buildings() const
	{
		return this->built_buildings;
	}

	std::vector<const building_type *> get_built_buildings_with_requirements() const;

	const site_map<std::vector<const building_type *>> &get_built_settlement_buildings() const
	{
		return this->built_settlement_buildings;
	}

	site_map<std::vector<const building_type *>> get_built_settlement_buildings_with_requirements() const;

	const site_map<std::vector<const improvement *>> &get_built_site_improvements() const
	{
		return this->built_site_improvements;
	}

	site_map<std::vector<const improvement *>> get_built_site_improvements_with_requirements() const;

	const site_map<int> &get_built_resource_site_levels() const
	{
		return this->built_resource_site_levels;
	}

	const std::vector<const technology *> &get_researched_technologies() const
	{
		return this->researched_technologies;
	}

	const std::vector<const character *> &get_recruited_characters() const
	{
		return this->recruited_characters;
	}

signals:
	void changed();

private:
	metternich::portrait *portrait = nullptr;
	std::string description;
	std::unique_ptr<const and_condition<domain>> preconditions;
	std::unique_ptr<const and_condition<domain>> conditions;
	std::unique_ptr<const and_condition<domain>> completion_conditions;
	decimillesimal_int completion_random_chance;
	std::unique_ptr<const and_condition<domain>> failure_conditions;
	std::unique_ptr<const effect_list<const domain>> completion_effects;
	std::unique_ptr<const effect_list<const domain>> failure_effects;
	std::unique_ptr<const modifier<const domain>> active_modifier;
	std::unique_ptr<const modifier<const domain>> completion_modifier;
	std::vector<const province *> owned_provinces;
	std::vector<const site *> owned_sites;
	std::vector<const building_type *> built_buildings;
	site_map<std::vector<const building_type *>> built_settlement_buildings;
	site_map<std::vector<const improvement *>> built_site_improvements;
	site_map<int> built_resource_site_levels;
	std::vector<const technology *> researched_technologies;
	std::vector<const character *> recruited_characters;
};

}
