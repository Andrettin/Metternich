#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "util/fractional_int.h"

namespace metternich {

class commodity;
class population_class;

class employment_type final : public named_data_entry, public data_type<employment_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::commodity* output_commodity MEMBER output_commodity NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int output_multiplier MEMBER output_multiplier READ get_output_multiplier NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "employment_type";
	static constexpr const char property_class_identifier[] = "metternich::employment_type*";
	static constexpr const char database_folder[] = "employment_types";

	explicit employment_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const commodity_map<centesimal_int> &get_input_commodities() const
	{
		return this->input_commodities;
	}

	const commodity *get_output_commodity() const
	{
		return this->output_commodity;
	}

	const centesimal_int &get_output_multiplier() const
	{
		return this->output_multiplier;
	}

	const std::vector<const population_class *> &get_employees() const
	{
		return this->employees;
	}

signals:
	void changed();

private:
	commodity_map<centesimal_int> input_commodities;
	commodity *output_commodity = nullptr;
	centesimal_int output_multiplier = centesimal_int(1);
	std::vector<const population_class *> employees;
};

}
