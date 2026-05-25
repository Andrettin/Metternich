#pragma once

#include "economy/commodity_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "population/population_type_container.h"

Q_MOC_INCLUDE("economy/commodity.h")
Q_MOC_INCLUDE("technology/technology.h")

namespace metternich {

class commodity;
class technology;

template <typename scope_type>
class modifier;

class employment_type final : public named_data_entry, public data_type<employment_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::commodity *output_commodity MEMBER output_commodity READ get_output_commodity NOTIFY changed)
	Q_PROPERTY(qint64 monthly_output_value MEMBER monthly_output_value READ get_monthly_output_value NOTIFY changed)
	Q_PROPERTY(qint64 base_employment_size MEMBER base_employment_size READ get_base_employment_size NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "employment_type";
	static constexpr const char property_class_identifier[] = "metternich::employment_type*";
	static constexpr const char database_folder[] = "employment_types";

	explicit employment_type(const std::string &identifier);
	~employment_type();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const population_type_set &get_employee_types() const
	{
		return this->employee_types;
	}

	bool can_employ(const population_type *population_type, const metternich::population_type *&employed_population_type) const;

	const commodity_map<int64_t> &get_input_commodities() const
	{
		return this->input_commodities;
	}

	int64_t get_input_for_employment_size(const commodity *commodity, const int64_t employment_size, const int throughput_modifier) const;
	int64_t get_employment_size_for_input(const commodity *commodity, const int64_t input, const int throughput_modifier) const;

	const commodity *get_output_commodity() const
	{
		return this->output_commodity;
	}

	int64_t get_monthly_output_value() const
	{
		return this->monthly_output_value;
	}

	int64_t get_base_employment_size() const
	{
		return this->base_employment_size;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const modifier<const site> *get_modifier() const
	{
		return this->modifier.get();
	}

	const modifier<const domain> *get_domain_modifier() const
	{
		return this->domain_modifier.get();
	}

	bool is_available_for_site(const site *site) const;

signals:
	void changed();

private:
	population_type_set employee_types;
	commodity_map<int64_t> input_commodities;
	int64_t input_wealth_value = 0;
	commodity_map<int> input_commodity_weights;
	const commodity *output_commodity = nullptr;
	int64_t monthly_output_value = 0;
	int64_t base_employment_size = 0; //the employment size used for calculating the employee output
	technology *required_technology = nullptr;
	std::unique_ptr<metternich::modifier<const site>> modifier;
	std::unique_ptr<metternich::modifier<const domain>> domain_modifier;
};

}
