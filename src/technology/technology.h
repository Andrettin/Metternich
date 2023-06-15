#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class building_type;
class character;
class commodity;
class country;
class culture;
class improvement;
class military_unit_type;
class portrait;
class production_type;
class resource;
class technological_period;
class transporter_type;
enum class technology_category;

template <typename scope_type>
class modifier;

class technology final : public named_data_entry, public data_type<technology>
{
	Q_OBJECT

	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(metternich::technology_category category MEMBER category NOTIFY changed)
	Q_PROPERTY(int category_index READ get_category_index NOTIFY changed)
	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(int cost MEMBER cost READ get_cost NOTIFY changed)
	Q_PROPERTY(bool discovery MEMBER discovery READ is_discovery NOTIFY changed)
	Q_PROPERTY(int year MEMBER year READ get_year NOTIFY changed)
	Q_PROPERTY(QVariantList prerequisites READ get_prerequisites_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_buildings READ get_enabled_buildings_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_improvements READ get_enabled_improvements_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_military_units READ get_enabled_military_units_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_advisors READ get_enabled_advisors_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList retired_advisors READ get_retired_advisors_qvariant_list NOTIFY changed)
	Q_PROPERTY(QObject* tree_parent READ get_tree_parent CONSTANT)
	Q_PROPERTY(QVariantList secondary_tree_parents READ get_secondary_tree_parents CONSTANT)

public:
	static constexpr const char class_identifier[] = "technology";
	static constexpr const char property_class_identifier[] = "metternich::technology*";
	static constexpr const char database_folder[] = "technologies";

	static constexpr int base_cost = 10;

	static void initialize_all();

	explicit technology(const std::string &identifier);
	~technology();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

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

	technology_category get_category() const
	{
		return this->category;
	}

	int get_category_index() const
	{
		return static_cast<int>(this->get_category());
	}

	const metternich::portrait *get_portrait() const
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

	int get_year() const
	{
		return this->year;
	}

	const technological_period *get_period() const
	{
		return this->period;
	}

	const std::vector<const technology *> get_prerequisites() const
	{
		return this->prerequisites;
	}

	QVariantList get_prerequisites_qvariant_list() const;

	bool requires_technology(const technology *technology) const;
	int get_total_prerequisite_depth() const;

	const std::vector<const commodity *> &get_enabled_commodities() const
	{
		return this->enabled_commodities;
	}

	void add_enabled_commodity(const commodity *commodity)
	{
		this->enabled_commodities.push_back(commodity);
	}

	const std::vector<const resource *> &get_enabled_resources() const
	{
		return this->enabled_resources;
	}

	void add_enabled_resource(const resource *resource)
	{
		this->enabled_resources.push_back(resource);
	}

	const std::vector<const building_type *> &get_enabled_buildings() const
	{
		return this->enabled_buildings;
	}

	QVariantList get_enabled_buildings_qvariant_list() const;
	std::vector<const building_type *> get_enabled_buildings_for_culture(const culture *culture) const;

	void add_enabled_building(const building_type *building)
	{
		this->enabled_buildings.push_back(building);
	}

	const std::vector<const production_type *> &get_enabled_production_types() const
	{
		return this->enabled_production_types;
	}

	void add_enabled_production_type(const production_type *production_type)
	{
		this->enabled_production_types.push_back(production_type);
	}

	const std::vector<const improvement *> &get_enabled_improvements() const
	{
		return this->enabled_improvements;
	}

	QVariantList get_enabled_improvements_qvariant_list() const;

	void add_enabled_improvement(const improvement *improvement)
	{
		this->enabled_improvements.push_back(improvement);
	}

	const std::vector<const military_unit_type *> &get_enabled_military_units() const
	{
		return this->enabled_military_units;
	}

	QVariantList get_enabled_military_units_qvariant_list() const;
	std::vector<const military_unit_type *> get_enabled_military_units_for_culture(const culture *culture) const;
	void add_enabled_military_unit(const military_unit_type *military_unit);

	const std::vector<const transporter_type *> &get_enabled_transporters() const
	{
		return this->enabled_transporters;
	}

	QVariantList get_enabled_transporters_qvariant_list() const;
	std::vector<const transporter_type *> get_enabled_transporters_for_culture(const culture *culture) const;
	void add_enabled_transporter(const transporter_type *transporter);

	const std::vector<const character *> &get_enabled_advisors() const
	{
		return this->enabled_advisors;
	}

	QVariantList get_enabled_advisors_qvariant_list() const;
	std::vector<const character *> get_enabled_advisors_for_country(const country *country) const;
	void add_enabled_advisor(const character *advisor);

	const std::vector<const character *> &get_retired_advisors() const
	{
		return this->retired_advisors;
	}

	QVariantList get_retired_advisors_qvariant_list() const;
	std::vector<const character *> get_retired_advisors_for_country(const country *country) const;
	void add_retired_advisor(const character *advisor);

	const metternich::modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	std::string get_modifier_string() const;
	Q_INVOKABLE QString get_effects_string(metternich::country *country) const;

	virtual named_data_entry *get_tree_parent() const override
	{
		if (!this->get_prerequisites().empty()) {
			return const_cast<technology *>(this->get_prerequisites().front());
		}

		return nullptr;
	}

	QVariantList get_secondary_tree_parents() const
	{
		QVariantList secondary_tree_parents;

		for (size_t i = 1; i < this->get_prerequisites().size(); ++i) {
			secondary_tree_parents.push_back(QVariant::fromValue(const_cast<technology *>(this->get_prerequisites()[i])));
		}

		return secondary_tree_parents;
	}

	virtual int get_tree_y() const override
	{
		return this->get_total_prerequisite_depth();
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
	std::string description;
	technology_category category;
	metternich::portrait *portrait = nullptr;
	int cost = 0;
	bool discovery = false;
	int year = 0; //the historical year that this technology was discovered
	const technological_period *period = nullptr;
	std::vector<const technology *> prerequisites;
	std::vector<const commodity *> enabled_commodities;
	std::vector<const resource *> enabled_resources;
	std::vector<const building_type *> enabled_buildings;
	std::vector<const production_type *> enabled_production_types;
	std::vector<const improvement *> enabled_improvements;
	std::vector<const military_unit_type *> enabled_military_units;
	std::vector<const transporter_type *> enabled_transporters;
	std::vector<const character *> enabled_advisors;
	std::vector<const character *> retired_advisors;
	std::unique_ptr<const metternich::modifier<const country>> modifier;
};

}
