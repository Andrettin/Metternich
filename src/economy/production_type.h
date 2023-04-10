#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"

namespace metternich {

class commodity;
class technology;

class production_type final : public named_data_entry, public data_type<production_type>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList input_commodities READ get_input_commodities_qvariant_list NOTIFY changed)
	Q_PROPERTY(metternich::commodity* output_commodity MEMBER output_commodity NOTIFY changed)
	Q_PROPERTY(int output_value MEMBER output_value READ get_output_value NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "production_type";
	static constexpr const char property_class_identifier[] = "metternich::production_type*";
	static constexpr const char database_folder[] = "production_types";

	explicit production_type(const std::string &identifier) : named_data_entry(identifier)
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

	const commodity *get_output_commodity() const
	{
		return this->output_commodity;
	}

	int get_output_value() const
	{
		return this->output_value;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

signals:
	void changed();

private:
	commodity_map<int> input_commodities;
	commodity *output_commodity = nullptr;
	int output_value = 1;
	technology *required_technology = nullptr;
};

}
