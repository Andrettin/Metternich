#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "population/population_type_container.h"

Q_MOC_INCLUDE("economy/commodity.h")

namespace metternich {

class commodity;

class employment_type final : public named_data_entry, public data_type<employment_type>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList input_commodities READ get_input_commodities_qvariant_list NOTIFY changed)
	Q_PROPERTY(int input_wealth MEMBER input_wealth READ get_input_wealth NOTIFY changed)
	Q_PROPERTY(metternich::commodity* output_commodity MEMBER output_commodity NOTIFY changed)
	Q_PROPERTY(int output_value MEMBER output_value READ get_output_value NOTIFY changed)
	Q_PROPERTY(bool industrial READ is_industrial CONSTANT)

public:
	static constexpr const char class_identifier[] = "employment_type";
	static constexpr const char property_class_identifier[] = "metternich::employment_type*";
	static constexpr const char database_folder[] = "employment_types";

	explicit employment_type(const std::string &identifier) : named_data_entry(identifier)
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

	int get_output_value() const
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
	int output_value = 1;
	population_type_set employees;
	bool industrial = false;
};

}
