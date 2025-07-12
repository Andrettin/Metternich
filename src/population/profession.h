#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "population/population_type_container.h"
#include "util/fractional_int.h"

Q_MOC_INCLUDE("economy/commodity.h")

namespace metternich {

class commodity;

class profession final : public named_data_entry, public data_type<profession>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList input_commodities READ get_input_commodities_qvariant_list NOTIFY changed)
	Q_PROPERTY(int input_wealth MEMBER input_wealth READ get_input_wealth NOTIFY changed)
	Q_PROPERTY(metternich::commodity* output_commodity MEMBER output_commodity NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int output_value MEMBER output_value READ get_output_value NOTIFY changed)
	Q_PROPERTY(bool industrial READ is_industrial CONSTANT)

public:
	static constexpr const char class_identifier[] = "profession";
	static constexpr const char property_class_identifier[] = "metternich::profession*";
	static constexpr const char database_folder[] = "professions";

	explicit profession(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const commodity_map<int> &get_input_commodities() const
	{
		return this->input_commodities;
	}

	QVariantList get_input_commodities_qvariant_list() const;

	int get_input_wealth() const
	{
		return this->input_wealth;
	}

	const commodity *get_output_commodity() const
	{
		return this->output_commodity;
	}

	const centesimal_int &get_output_value() const
	{
		return this->output_value;
	}

	bool can_employ(const population_type *population_type) const
	{
		return this->employees.contains(population_type);
	}

	bool is_industrial() const
	{
		return this->industrial;
	}

signals:
	void changed();

private:
	commodity_map<int> input_commodities;
	int input_wealth = 0;
	commodity *output_commodity = nullptr;
	centesimal_int output_value = centesimal_int(1);
	std::set<const population_type *> employees;
	bool industrial = false;
};

}
