#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

namespace metternich {

class commodity;
class holding_type;

template <typename scope_type>
class and_condition;

class route_type final : public named_data_entry, public data_type<route_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::commodity* output_commodity MEMBER output_commodity READ get_output_commodity NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "route_type";
	static constexpr const char property_class_identifier[] = "metternich::route_type*";
	static constexpr const char database_folder[] = "route_types";

	explicit route_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const commodity *get_output_commodity() const
	{
		return this->output_commodity;
	}

	int64_t get_output_multiplier() const
	{
		return this->output_multiplier;
	}

	const std::vector<const holding_type *> &get_holding_types() const
	{
		return this->holding_types;
	}

	bool has_holding_type(const holding_type *holding_type) const;

signals:
	void changed();

private:
	const commodity *output_commodity = nullptr;
	int64_t output_multiplier = 0;
	std::vector<const holding_type *> holding_types; //the holding types that can be connected by this route type
};

}
