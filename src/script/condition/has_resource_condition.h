#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/resource.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class has_resource_condition final : public condition<scope_type>
{
public:
	explicit has_resource_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->resource = resource::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_resource";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_resource_counts().contains(this->resource);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("Has {} resource", this->resource->get_name());
	}

private:
	const metternich::resource *resource = nullptr;
};

}
