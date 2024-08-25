#pragma once

#include "country/culture_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "infrastructure/pathway_container.h"

Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class building_type;
class character;
class civilian_unit_type;
class commodity;
class country;
class cultural_group;
class culture;
class government_type;
class icon;
class improvement;
class law;
class military_unit_type;
class pathway;
class portrait;
class production_type;
class resource;
class technological_period;
class terrain_type;
class tradition;
class transporter_type;
class wonder;
enum class character_role;
enum class technology_category;

template <typename scope_type>
class factor;

template <typename scope_type>
class modifier;

class technology final : public named_data_entry, public data_type<technology>
{
	Q_OBJECT

	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(metternich::technology_category category MEMBER category NOTIFY changed)
	Q_PROPERTY(int category_index READ get_category_index NOTIFY changed)
	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(int cost MEMBER cost READ get_cost NOTIFY changed)
	Q_PROPERTY(bool discovery MEMBER discovery READ is_discovery NOTIFY changed)
	Q_PROPERTY(int year MEMBER year READ get_year NOTIFY changed)
	Q_PROPERTY(const metternich::technological_period* period MEMBER period READ get_period NOTIFY changed)
	Q_PROPERTY(bool free_technology MEMBER free_technology READ grants_free_technology NOTIFY changed)
	Q_PROPERTY(int shared_prestige MEMBER shared_prestige READ get_shared_prestige NOTIFY changed)
	Q_PROPERTY(QVariantList prerequisites READ get_prerequisites_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_buildings READ get_enabled_buildings_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_improvements READ get_enabled_improvements_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_pathways READ get_enabled_pathways_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_civilian_units READ get_enabled_civilian_units_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_military_units READ get_enabled_military_units_qvariant_list NOTIFY changed)
	Q_PROPERTY(const QObject* tree_parent READ get_tree_parent CONSTANT)
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

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	bool is_available_for_country(const country *country) const;

	int get_cost() const
	{
		if (this->cost > 0) {
			return this->cost;
		}

		return (this->get_total_prerequisite_depth() + 1) * technology::base_cost;
	}

	Q_INVOKABLE int get_cost_for_country(const metternich::country *country) const;

	const factor<country> *get_cost_factor() const
	{
		return this->cost_factor.get();
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

	bool grants_free_technology() const
	{
		return this->free_technology;
	}

	int get_shared_prestige() const
	{
		return this->shared_prestige;
	}

	int get_shared_prestige_for_country(const country *country) const;

	const std::vector<technology *> get_prerequisites() const
	{
		return this->prerequisites;
	}

	QVariantList get_prerequisites_qvariant_list() const;

	bool requires_technology(const technology *technology) const;

	int get_total_prerequisite_depth() const
	{
		return this->total_prerequisite_depth;
	}

	void calculate_total_prerequisite_depth();

	const std::vector<const technology *> &get_leads_to() const
	{
		return this->leads_to;
	}

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

	const std::vector<const wonder *> &get_enabled_wonders() const
	{
		return this->enabled_wonders;
	}

	std::vector<const wonder *> get_enabled_wonders_for_country(const country *country) const;

	void add_enabled_wonder(const wonder *wonder)
	{
		this->enabled_wonders.push_back(wonder);
	}

	const std::vector<const wonder *> &get_disabled_wonders() const
	{
		return this->disabled_wonders;
	}

	std::vector<const wonder *> get_disabled_wonders_for_country(const country *country) const;

	void add_disabled_wonder(const wonder *wonder)
	{
		this->disabled_wonders.push_back(wonder);
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

	const std::vector<const pathway *> &get_enabled_pathways() const
	{
		return this->enabled_pathways;
	}

	QVariantList get_enabled_pathways_qvariant_list() const;

	void add_enabled_pathway(const pathway *pathway)
	{
		this->enabled_pathways.push_back(pathway);
	}

	const std::vector<const pathway *> &get_enabled_river_crossing_pathways() const
	{
		return this->enabled_river_crossing_pathways;
	}

	void add_enabled_river_crossing_pathway(const pathway *pathway)
	{
		this->enabled_river_crossing_pathways.push_back(pathway);
	}

	const pathway_map<std::vector<const terrain_type *>> &get_enabled_pathway_terrains() const
	{
		return this->enabled_pathway_terrains;
	}

	void add_enabled_pathway_terrain(const pathway *pathway, const terrain_type *terrain)
	{
		this->enabled_pathway_terrains[pathway].push_back(terrain);
	}

	const std::vector<const civilian_unit_type *> &get_enabled_civilian_units() const
	{
		return this->enabled_civilian_units;
	}

	QVariantList get_enabled_civilian_units_qvariant_list() const;
	std::vector<const civilian_unit_type *> get_enabled_civilian_units_for_culture(const culture *culture) const;
	void add_enabled_civilian_unit(const civilian_unit_type *military_unit);

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

	const std::vector<const law *> &get_enabled_laws() const
	{
		return this->enabled_laws;
	}

	void add_enabled_law(const law *law);

	const std::vector<const tradition *> &get_enabled_traditions() const
	{
		return this->enabled_traditions;
	}

	std::vector<const tradition *> get_enabled_traditions_for_country(const country *country) const;
	void add_enabled_tradition(const tradition *tradition);

	const std::vector<const character *> &get_enabled_characters(const character_role role) const
	{
		static const std::vector<const character *> empty_vector;

		const auto find_iterator = this->enabled_characters.find(role);
		if (find_iterator != this->enabled_characters.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	std::vector<const character *> get_enabled_characters_for_country(const character_role role, const country *country) const;
	void add_enabled_character(const character_role role, const character *character);

	const std::vector<const character *> &get_retired_characters(const character_role role) const
	{
		static const std::vector<const character *> empty_vector;

		const auto find_iterator = this->retired_characters.find(role);
		if (find_iterator != this->retired_characters.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	std::vector<const character *> get_retired_characters_for_country(const character_role role, const country *country) const;
	void add_retired_character(const character_role role, const character *character);

	const metternich::modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	std::string get_modifier_string(const country *country) const;
	Q_INVOKABLE QString get_effects_string(metternich::country *country) const;
	void write_character_effects_string(const character_role role, const std::string_view &role_name, const country *country, std::string &str) const;

	virtual named_data_entry *get_tree_parent() const override
	{
		if (!this->get_prerequisites().empty()) {
			return this->get_prerequisites().front();
		}

		return nullptr;
	}

	QVariantList get_secondary_tree_parents() const
	{
		QVariantList secondary_tree_parents;

		for (size_t i = 1; i < this->get_prerequisites().size(); ++i) {
			secondary_tree_parents.push_back(QVariant::fromValue(this->get_prerequisites()[i]));
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

	virtual bool is_hidden_in_tree() const override;

signals:
	void changed();

private:
	std::string description;
	technology_category category;
	metternich::portrait *portrait = nullptr;
	metternich::icon *icon = nullptr;
	culture_set cultures;
	std::vector<const cultural_group *> cultural_groups;
	int cost = 0;
	std::unique_ptr<const factor<country>> cost_factor;
	bool discovery = false;
	int year = 0; //the historical year that this technology was discovered
	const technological_period *period = nullptr;
	bool free_technology = false; //grants free technology for first one to research
	int shared_prestige = 0;
	std::vector<technology *> prerequisites;
	int total_prerequisite_depth = 0;
	std::vector<const technology *> leads_to;
	std::vector<const commodity *> enabled_commodities;
	std::vector<const resource *> enabled_resources;
	std::vector<const building_type *> enabled_buildings;
	std::vector<const wonder *> enabled_wonders;
	std::vector<const wonder *> disabled_wonders;
	std::vector<const production_type *> enabled_production_types;
	std::vector<const improvement *> enabled_improvements;
	std::vector<const pathway *> enabled_pathways;
	std::vector<const pathway *> enabled_river_crossing_pathways;
	pathway_map<std::vector<const terrain_type *>> enabled_pathway_terrains;
	std::vector<const civilian_unit_type *> enabled_civilian_units;
	std::vector<const military_unit_type *> enabled_military_units;
	std::vector<const transporter_type *> enabled_transporters;
	std::vector<const law *> enabled_laws;
	std::vector<const tradition *> enabled_traditions;
	std::map<character_role, std::vector<const character *>> enabled_characters;
	std::map<character_role, std::vector<const character *>> retired_characters;
	std::unique_ptr<const metternich::modifier<const country>> modifier;
};

}
