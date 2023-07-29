#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "population/phenotype_container.h"
#include "util/fractional_int.h"

namespace metternich {

class population_class;
class cultural_group;
class culture;
class icon;
class phenotype;

class population_type final : public named_data_entry, public data_type<population_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::population_class* population_class MEMBER population_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(bool literate MEMBER literate READ is_literate NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::icon* small_icon MEMBER small_icon NOTIFY changed)
	Q_PROPERTY(metternich::commodity* output_commodity MEMBER output_commodity NOTIFY changed)
	Q_PROPERTY(int output_value MEMBER output_value READ get_output_value NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "population_type";
	static constexpr const char property_class_identifier[] = "metternich::population_type*";
	static constexpr const char database_folder[] = "population_types";

public:
	explicit population_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

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

	const commodity_map<int> &get_consumed_commodities() const
	{
		return this->consumed_commodities;
	}

	int get_commodity_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->get_consumed_commodities().find(commodity);
		if (find_iterator != this->get_consumed_commodities().end()) {
			return find_iterator->second;
		}

		return 0;
	}

	const commodity *get_output_commodity() const
	{
		return this->output_commodity;
	}

	int get_output_value() const
	{
		return this->output_value;
	}

signals:
	void changed();

private:
	population_class *population_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	QColor color;
	bool literate = false;
	metternich::icon *icon = nullptr;
	metternich::icon *small_icon = nullptr;
	phenotype_map<const metternich::icon *> phenotype_icons;
	phenotype_map<const metternich::icon *> phenotype_small_icons;
	commodity_map<int> consumed_commodities;
	commodity *output_commodity = nullptr;
	int output_value = 1;
};

}
