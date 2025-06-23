#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "species/phenotype_container.h"
#include "util/fractional_int.h"

Q_MOC_INCLUDE("country/cultural_group.h")
Q_MOC_INCLUDE("country/culture.h")
Q_MOC_INCLUDE("economy/commodity.h")
Q_MOC_INCLUDE("game/game_rule.h")
Q_MOC_INCLUDE("population/population_class.h")
Q_MOC_INCLUDE("population/profession.h")
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
class profession;

template <typename scope_type>
class modifier;

class population_type final : public named_data_entry, public data_type<population_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::population_class* population_class MEMBER population_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(metternich::profession* profession MEMBER profession NOTIFY changed)
	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(bool literate MEMBER literate READ is_literate NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::icon* small_icon MEMBER small_icon NOTIFY changed)
	Q_PROPERTY(int everyday_wealth_consumption MEMBER everyday_wealth_consumption READ get_everyday_wealth_consumption NOTIFY changed)
	Q_PROPERTY(metternich::commodity* output_commodity MEMBER output_commodity NOTIFY changed)
	Q_PROPERTY(int output_value MEMBER output_value READ get_output_value NOTIFY changed)
	Q_PROPERTY(int resource_output_value MEMBER resource_output_value READ get_resource_output_value NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int max_modifier_multiplier MEMBER max_modifier_multiplier READ get_max_modifier_multiplier NOTIFY changed)
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

	const metternich::profession *get_profession() const
	{
		return this->profession;
	}

	const QColor &get_color() const
	{
		return this->color;
	}

	bool is_literate() const
	{
		return this->literate;
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

	int get_everyday_wealth_consumption() const
	{
		return this->everyday_wealth_consumption;
	}

	const commodity_map<centesimal_int> &get_everyday_consumption() const
	{
		return this->everyday_consumption;
	}

	const centesimal_int &get_everyday_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->get_everyday_consumption().find(commodity);
		if (find_iterator != this->get_everyday_consumption().end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	const commodity_map<centesimal_int> &get_luxury_consumption() const
	{
		return this->luxury_consumption;
	}

	const centesimal_int &get_luxury_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->get_luxury_consumption().find(commodity);
		if (find_iterator != this->get_luxury_consumption().end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	const commodity_map<decimillesimal_int> &get_commodity_demands() const
	{
		return this->commodity_demands;
	}

	const decimillesimal_int &get_commodity_demand(const commodity *commodity) const
	{
		const auto find_iterator = this->get_commodity_demands().find(commodity);
		if (find_iterator != this->get_commodity_demands().end()) {
			return find_iterator->second;
		}

		static const decimillesimal_int zero;
		return zero;
	}

	const commodity *get_output_commodity() const
	{
		return this->output_commodity;
	}

	int get_output_value() const
	{
		return this->output_value;
	}

	int get_resource_output_value() const
	{
		return this->resource_output_value;
	}

	const centesimal_int &get_max_modifier_multiplier() const
	{
		return this->max_modifier_multiplier;
	}

	const modifier<const country> *get_country_modifier() const
	{
		return this->country_modifier.get();
	}

	Q_INVOKABLE QString get_country_modifier_string(const metternich::country *country) const;

	bool is_enabled() const;

signals:
	void changed();

private:
	population_class *population_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::profession *profession = nullptr;
	QColor color;
	bool literate = false;
	metternich::icon *icon = nullptr;
	metternich::icon *small_icon = nullptr;
	phenotype_map<const metternich::icon *> phenotype_icons;
	phenotype_map<const metternich::icon *> phenotype_small_icons;
	int everyday_wealth_consumption = 0;
	commodity_map<centesimal_int> everyday_consumption;
	commodity_map<centesimal_int> luxury_consumption;
	commodity_map<decimillesimal_int> commodity_demands;
	commodity *output_commodity = nullptr;
	int output_value = 1;
	int resource_output_value = 0;
	centesimal_int max_modifier_multiplier = centesimal_int(0);
	std::unique_ptr<modifier<const country>> country_modifier;
	const game_rule *required_game_rule = nullptr;
};

}
