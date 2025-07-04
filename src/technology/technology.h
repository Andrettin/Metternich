#pragma once

#include "country/culture_container.h"
#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "infrastructure/pathway_container.h"
#include "religion/religion_container.h"

Q_MOC_INCLUDE("game/game_rule.h")
Q_MOC_INCLUDE("technology/technology_category.h")
Q_MOC_INCLUDE("technology/technology_subcategory.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace archimedes {
	class game_rule;
}

namespace metternich {

class building_type;
class character;
class civilian_unit_type;
class commodity;
class country;
class cultural_group;
class culture;
class deity;
class government_type;
class icon;
class improvement;
class law;
class military_unit_type;
class pathway;
class portrait;
class production_type;
class religion;
class religious_group;
class research_organization;
class resource;
class technological_period;
class technology_category;
class technology_subcategory;
class terrain_type;
class transporter_type;
class wonder;
enum class character_role;

template <typename scope_type>
class factor;

template <typename scope_type>
class modifier;

class technology final : public named_data_entry, public data_type<technology>
{
	Q_OBJECT

	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(const metternich::technology_category* category READ get_category NOTIFY changed)
	Q_PROPERTY(metternich::technology_subcategory* subcategory MEMBER subcategory NOTIFY changed)
	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(bool discovery MEMBER discovery READ is_discovery NOTIFY changed)
	Q_PROPERTY(int year MEMBER year READ get_year NOTIFY changed)
	Q_PROPERTY(const metternich::technological_period* period MEMBER period READ get_period NOTIFY changed)
	Q_PROPERTY(int free_technologies MEMBER free_technologies READ get_free_technologies NOTIFY changed)
	Q_PROPERTY(int shared_prestige MEMBER shared_prestige READ get_shared_prestige NOTIFY changed)
	Q_PROPERTY(QVariantList prerequisites READ get_prerequisites_qvariant_list NOTIFY changed)
	Q_PROPERTY(int wealth_cost_weight MEMBER wealth_cost_weight NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_buildings READ get_enabled_buildings_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_improvements READ get_enabled_improvements_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_pathways READ get_enabled_pathways_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_civilian_units READ get_enabled_civilian_units_qvariant_list NOTIFY changed)
	Q_PROPERTY(QVariantList enabled_military_units READ get_enabled_military_units_qvariant_list NOTIFY changed)
	Q_PROPERTY(const QObject* tree_parent READ get_tree_parent CONSTANT)
	Q_PROPERTY(QVariantList secondary_tree_parents READ get_secondary_tree_parents CONSTANT)
	Q_PROPERTY(const archimedes::game_rule* required_game_rule MEMBER required_game_rule NOTIFY changed)
	Q_PROPERTY(bool enabled READ is_enabled NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "technology";
	static constexpr const char property_class_identifier[] = "metternich::technology*";
	static constexpr const char database_folder[] = "technologies";

	static constexpr int default_wealth_cost_weight = 1;
	static constexpr int default_commodity_cost_weight = 3;

public:
	static void initialize_all();

	static const std::vector<const technology *> &get_top_level_technologies()
	{
		return technology::top_level_technologies;
	}

	static const std::vector<const commodity *> &get_research_commodities()
	{
		return technology::research_commodities;
	}

private:
	static inline std::vector<const technology *> top_level_technologies;
	static inline std::vector<const commodity *> research_commodities;

public:
	explicit technology(const std::string &identifier);
	~technology();

	virtual void process_gsml_property(const gsml_property &property) override;
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

	const technology_category *get_category() const;

	const technology_subcategory *get_subcategory() const
	{
		return this->subcategory;
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	Q_INVOKABLE bool is_available_for_country(const metternich::country *country) const;

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

	int get_free_technologies() const
	{
		return this->free_technologies;
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

	const technology *get_parent_technology() const
	{
		return this->parent_technology;
	}
	
	const std::vector<const technology *> &get_child_technologies() const
	{
		return this->child_technologies;
	}

	int get_wealth_cost_weight() const;
	commodity_map<int> get_commodity_cost_weights() const;
	int get_total_cost_weights() const;
	centesimal_int get_cost_for_country(const country *country) const;
	Q_INVOKABLE int get_wealth_cost_for_country(const metternich::country *country) const;
	commodity_map<int> get_commodity_costs_for_country(const country *country) const;
	Q_INVOKABLE QVariantList get_commodity_costs_for_country_qvariant_list(const metternich::country *country) const;

	void calculate_cost();

	const factor<country> *get_cost_factor() const
	{
		return this->cost_factor.get();
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
	void add_enabled_civilian_unit(const civilian_unit_type *civilian_unit);

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

	const std::vector<const government_type *> &get_enabled_government_types() const
	{
		return this->enabled_government_types;
	}

	void add_enabled_government_type(const government_type *government_type);

	const std::vector<const law *> &get_enabled_laws() const
	{
		return this->enabled_laws;
	}

	void add_enabled_law(const law *law);

	const std::vector<const research_organization *> &get_enabled_research_organizations() const
	{
		return this->enabled_research_organizations;
	}

	std::vector<const research_organization *> get_enabled_research_organizations_for_country(const country *country) const;

	void add_enabled_research_organization(const research_organization *organization)
	{
		this->enabled_research_organizations.push_back(organization);
	}

	const std::vector<const research_organization *> &get_disabled_research_organizations() const
	{
		return this->disabled_research_organizations;
	}

	std::vector<const research_organization *> get_disabled_research_organizations_for_country(const country *country) const;

	void add_disabled_research_organization(const research_organization *organization)
	{
		this->disabled_research_organizations.push_back(organization);
	}

	const std::vector<const deity *> &get_enabled_deities() const
	{
		return this->enabled_deities;
	}

	std::vector<const deity *> get_enabled_deities_for_country(const country *country) const;

	void add_enabled_deity(const deity *deity)
	{
		this->enabled_deities.push_back(deity);
	}

	const std::vector<const deity *> &get_disabled_deities() const
	{
		return this->disabled_deities;
	}

	std::vector<const deity *> get_disabled_deities_for_country(const country *country) const;

	void add_disabled_deity(const deity *deity)
	{
		this->disabled_deities.push_back(deity);
	}

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
	Q_INVOKABLE QString get_effects_string(const metternich::country *country) const;
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

	bool is_enabled() const;

signals:
	void changed();

private:
	std::string description;
	technology_subcategory *subcategory = nullptr;
	metternich::portrait *portrait = nullptr;
	metternich::icon *icon = nullptr;
	culture_set cultures;
	std::vector<const cultural_group *> cultural_groups;
	religion_set religions;
	std::vector<const religious_group *> religious_groups;
	bool discovery = false;
	int year = 0; //the historical year that this technology was discovered
	const technological_period *period = nullptr;
	int free_technologies = 0; //grants free technologies for the first country to research
	int shared_prestige = 0;
	std::vector<technology *> prerequisites;
	int total_prerequisite_depth = 0;
	std::vector<const technology *> leads_to;
	const technology *parent_technology = nullptr;
	std::vector<const technology *> child_technologies;
	int cost = 0;
	int wealth_cost_weight = technology::default_wealth_cost_weight;
	commodity_map<int> commodity_cost_weights;
	std::unique_ptr<const factor<country>> cost_factor;
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
	std::vector<const government_type *> enabled_government_types;
	std::vector<const law *> enabled_laws;
	std::vector<const research_organization *> enabled_research_organizations;
	std::vector<const research_organization *> disabled_research_organizations;
	std::vector<const deity *> enabled_deities;
	std::vector<const deity *> disabled_deities;
	std::map<character_role, std::vector<const character *>> enabled_characters;
	std::map<character_role, std::vector<const character *>> retired_characters;
	std::unique_ptr<const metternich::modifier<const country>> modifier;
	const game_rule *required_game_rule = nullptr;
};

}
