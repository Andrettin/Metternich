#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class building_type;
class character;
class country;
class culture;
class icon;
class improvement;
class military_unit_type;
enum class technology_category;

template <typename scope_type>
class modifier;

class technology final : public named_data_entry, public data_type<technology>
{
	Q_OBJECT

	Q_PROPERTY(metternich::technology_category category MEMBER category NOTIFY changed)
	Q_PROPERTY(int category_index READ get_category_index NOTIFY changed)
	Q_PROPERTY(metternich::icon* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(int cost MEMBER cost READ get_cost NOTIFY changed)
	Q_PROPERTY(bool discovery MEMBER discovery READ is_discovery NOTIFY changed)
	Q_PROPERTY(QVariantList prerequisites READ get_prerequisites_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_buildings READ get_enabled_buildings_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_improvements READ get_enabled_improvements_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_military_units READ get_enabled_military_units_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_advisors READ get_enabled_advisors_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList retired_advisors READ get_retired_advisors_qvariant_list NOTIFY changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string CONSTANT)
	Q_PROPERTY(QObject* tree_parent READ get_tree_parent CONSTANT)

public:
	static constexpr const char class_identifier[] = "technology";
	static constexpr const char property_class_identifier[] = "metternich::technology*";
	static constexpr const char database_folder[] = "technologies";

	static constexpr int base_cost = 10;

	explicit technology(const std::string &identifier);
	~technology();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	technology_category get_category() const
	{
		return this->category;
	}

	int get_category_index() const
	{
		return static_cast<int>(this->get_category());
	}

	const icon *get_portrait() const
	{
		return this->portrait;
	}

	int get_cost() const
	{
		if (this->cost > 0) {
			return this->cost;
		}

		return (this->get_total_prerequisite_depth() + 1) * technology::base_cost;
	}

	bool is_discovery() const
	{
		return this->discovery;
	}

	const std::vector<const technology *> get_prerequisites() const
	{
		return this->prerequisites;
	}

	QVariantList get_prerequisites_qvariant_list() const;

	bool requires_technology(const technology *technology) const;
	int get_total_prerequisite_depth() const;

	const std::vector<const building_type *> get_enabled_buildings() const
	{
		return this->enabled_buildings;
	}

	QVariantList get_enabled_buildings_qvariant_list() const;
	Q_INVOKABLE QVariantList get_enabled_buildings_for_culture(metternich::culture *culture) const;

	void add_enabled_building(const building_type *building)
	{
		this->enabled_buildings.push_back(building);
	}

	const std::vector<const improvement *> get_enabled_improvements() const
	{
		return this->enabled_improvements;
	}

	QVariantList get_enabled_improvements_qvariant_list() const;

	void add_enabled_improvement(const improvement *improvement)
	{
		this->enabled_improvements.push_back(improvement);
	}

	const std::vector<const military_unit_type *> get_enabled_military_units() const
	{
		return this->enabled_military_units;
	}

	QVariantList get_enabled_military_units_qvariant_list() const;
	Q_INVOKABLE QVariantList get_enabled_military_units_for_culture(metternich::culture *culture) const;
	void add_enabled_military_unit(const military_unit_type *military_unit);

	const std::vector<const character *> get_enabled_advisors() const
	{
		return this->enabled_advisors;
	}

	QVariantList get_enabled_advisors_qvariant_list() const;
	Q_INVOKABLE QVariantList get_enabled_advisors_for_country(metternich::country *country) const;
	void add_enabled_advisor(const character *advisor);

	const std::vector<const character *> get_retired_advisors() const
	{
		return this->retired_advisors;
	}

	QVariantList get_retired_advisors_qvariant_list() const;
	Q_INVOKABLE QVariantList get_retired_advisors_for_country(metternich::country *country) const;
	void add_retired_advisor(const character *advisor);

	const metternich::modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	QString get_modifier_string() const;

	virtual named_data_entry *get_tree_parent() const override
	{
		if (!this->get_prerequisites().empty()) {
			const technology *chosen_prerequisite = nullptr;

			for (const technology *prerequisite : this->get_prerequisites()) {
				if (chosen_prerequisite == nullptr || prerequisite->get_total_prerequisite_depth() > chosen_prerequisite->get_total_prerequisite_depth()) {
					chosen_prerequisite = prerequisite;
				}
			}

			return const_cast<technology *>(chosen_prerequisite);
		}

		return nullptr;
	}

	virtual std::vector<const named_data_entry *> get_top_tree_elements() const override
	{
		std::vector<const named_data_entry *> top_tree_elements;

		for (const technology *technology : technology::get_all()) {
			if (!technology->get_prerequisites().empty()) {
				continue;
			}

			top_tree_elements.push_back(technology);
		}

		return top_tree_elements;
	}

signals:
	void changed();

private:
	technology_category category;
	icon *portrait = nullptr;
	int cost = 0;
	bool discovery = false;
	std::vector<const technology *> prerequisites;
	std::vector<const building_type *> enabled_buildings;
	std::vector<const improvement *> enabled_improvements;
	std::vector<const military_unit_type *> enabled_military_units;
	std::vector<const character *> enabled_advisors;
	std::vector<const character *> retired_advisors;
	std::unique_ptr<const metternich::modifier<const country>> modifier;
};

}
