#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"

Q_MOC_INCLUDE("economy/commodity.h")
Q_MOC_INCLUDE("population/population_type.h")

namespace metternich {

class commodity;
class population_type;

class education_type final : public named_data_entry, public data_type<education_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::population_type* input_population_type MEMBER input_population_type READ get_input_population_type NOTIFY changed)
	Q_PROPERTY(QVariantList input_commodities READ get_input_commodities_qvariant_list NOTIFY changed)
	Q_PROPERTY(int input_wealth MEMBER input_wealth READ get_input_wealth NOTIFY changed)
	Q_PROPERTY(const metternich::population_type* output_population_type MEMBER output_population_type READ get_output_population_type NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "education_type";
	static constexpr const char property_class_identifier[] = "metternich::education_type*";
	static constexpr const char database_folder[] = "education_types";

	explicit education_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const population_type *get_input_population_type() const
	{
		return this->input_population_type;
	}

	const commodity_map<int> &get_input_commodities() const
	{
		return this->input_commodities;
	}

	QVariantList get_input_commodities_qvariant_list() const;

	int get_input_wealth() const
	{
		return this->input_wealth;
	}

	const population_type *get_output_population_type() const
	{
		return this->output_population_type;
	}
	
	bool is_enabled() const;

signals:
	void changed();

private:
	const population_type *input_population_type = nullptr;
	commodity_map<int> input_commodities;
	int input_wealth = 0;
	const population_type *output_population_type = nullptr;
};

}
