#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "population/population_type_container.h"
#include "species/phenotype_container.h"
#include "util/centesimal_int.h"
#include "util/decimillesimal_int.h"

Q_MOC_INCLUDE("culture/cultural_group.h")
Q_MOC_INCLUDE("culture/culture.h")
Q_MOC_INCLUDE("economy/commodity.h")
Q_MOC_INCLUDE("game/game_rule.h")
Q_MOC_INCLUDE("population/population_class.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace archimedes {
	class game_rule;
}

namespace metternich {

class cultural_group;
class culture;
class icon;
class phenotype;
class population_class;
class population_unit;
class province;
enum class population_strata;

template <typename scope_type>
class factor;

template <typename scope_type>
class modifier;

class population_type final : public named_data_entry, public data_type<population_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::population_class* population_class MEMBER population_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::icon* small_icon MEMBER small_icon NOTIFY changed)
	Q_PROPERTY(metternich::population_strata strata MEMBER strata READ get_strata NOTIFY changed)
	Q_PROPERTY(bool educator MEMBER educator READ is_educator NOTIFY changed)
	Q_PROPERTY(metternich::commodity* output_commodity MEMBER output_commodity NOTIFY changed)
	Q_PROPERTY(qint64 output_value MEMBER output_value READ get_output_value NOTIFY changed)
	Q_PROPERTY(int output_modifier MEMBER output_modifier READ get_output_modifier NOTIFY changed)
	Q_PROPERTY(int resource_output_bonus MEMBER resource_output_bonus READ get_resource_output_bonus NOTIFY changed)
	Q_PROPERTY(int daily_research MEMBER daily_research READ get_daily_research NOTIFY changed)
	Q_PROPERTY(archimedes::decimillesimal_int max_research_population_percent MEMBER max_research_population_percent READ get_max_research_population_percent NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int max_modifier_multiplier MEMBER max_modifier_multiplier READ get_max_modifier_multiplier NOTIFY changed)
	Q_PROPERTY(qint64 base_modifier_population_size MEMBER base_modifier_population_size READ get_base_modifier_population_size NOTIFY changed)
	Q_PROPERTY(const archimedes::game_rule* required_game_rule MEMBER required_game_rule NOTIFY changed)
	Q_PROPERTY(bool enabled READ is_enabled NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "population_type";
	static constexpr const char property_class_identifier[] = "metternich::population_type*";
	static constexpr const char database_folder[] = "population_types";

	static const std::set<std::string> database_dependencies;

public:
	explicit population_type(const std::string &identifier);
	~population_type();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const population_class *get_population_class() const
	{
		return this->population_class;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::cultural_group *get_cultural_group() const
	{
		return this->cultural_group;
	}

	const QColor &get_color() const
	{
		return this->color;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const metternich::icon *get_small_icon() const
	{
		return this->small_icon;
	}

	const metternich::icon *get_phenotype_icon(const phenotype *phenotype) const
	{
		const auto find_iterator = this->phenotype_icons.find(phenotype);
		if (find_iterator != this->phenotype_icons.end()) {
			return find_iterator->second;
		}

		return this->get_icon();
	}

	const metternich::icon *get_phenotype_small_icon(const phenotype *phenotype) const
	{
		const auto find_iterator = this->phenotype_small_icons.find(phenotype);
		if (find_iterator != this->phenotype_small_icons.end()) {
			return find_iterator->second;
		}

		return this->get_small_icon();
	}

	population_strata get_strata() const
	{
		return this->strata;
	}

	bool is_educator() const
	{
		return this->educator;
	}

	const commodity *get_output_commodity() const
	{
		return this->output_commodity;
	}

	int64_t get_output_value() const
	{
		return this->output_value;
	}

	int get_output_modifier() const
	{
		return this->output_modifier;
	}

	int get_resource_output_bonus() const
	{
		return this->resource_output_bonus;
	}

	int get_daily_research() const
	{
		return this->daily_research;
	}

	const decimillesimal_int &get_max_research_population_percent() const
	{
		return this->max_research_population_percent;
	}

	const centesimal_int &get_max_modifier_multiplier() const
	{
		return this->max_modifier_multiplier;
	}

	int64_t get_base_modifier_population_size() const
	{
		return this->base_modifier_population_size;
	}

	const modifier<const province> *get_province_modifier() const
	{
		return this->province_modifier.get();
	}

	const modifier<const domain> *get_domain_modifier() const
	{
		return this->domain_modifier.get();
	}

	Q_INVOKABLE QString get_domain_modifier_string(const metternich::domain *domain) const;

	const std::vector<const population_type *> &get_equivalent_population_types() const
	{
		return this->equivalent_population_types;
	}

	const commodity_map<decimillesimal_int> &get_life_needs() const
	{
		return this->life_needs;
	}

	const commodity_map<decimillesimal_int> &get_everyday_needs() const
	{
		return this->everyday_needs;
	}

	const commodity_map<decimillesimal_int> &get_luxury_needs() const
	{
		return this->luxury_needs;
	}

	const population_type_map<std::unique_ptr<factor<population_unit>>> &get_promotion_factors() const
	{
		return this->promotion_factors;
	}

	bool is_enabled() const;

signals:
	void changed();

private:
	population_class *population_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	QColor color;
	metternich::icon *icon = nullptr;
	metternich::icon *small_icon = nullptr;
	population_strata strata{};
	bool educator = false;
	phenotype_map<const metternich::icon *> phenotype_icons;
	phenotype_map<const metternich::icon *> phenotype_small_icons;
	commodity *output_commodity = nullptr;
	int64_t output_value = 0;
	int output_modifier = 0;
	int resource_output_bonus = 0;
	int daily_research = 0;
	decimillesimal_int max_research_population_percent;
	centesimal_int max_modifier_multiplier;
	int64_t base_modifier_population_size = 0;
	std::unique_ptr<modifier<const province>> province_modifier;
	std::unique_ptr<modifier<const domain>> domain_modifier;
	std::vector<const population_type *> equivalent_population_types;
	commodity_map<decimillesimal_int> life_needs;
	commodity_map<decimillesimal_int> everyday_needs;
	commodity_map<decimillesimal_int> luxury_needs;
	population_type_map<std::unique_ptr<factor<population_unit>>> promotion_factors; //selects the population type to be promoted or demoted to
	const game_rule *required_game_rule = nullptr;
};

}
