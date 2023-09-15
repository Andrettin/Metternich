#pragma once

#include "economy/resource.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

class resource_condition final : public condition<site>
{
public:
	explicit resource_condition(const resource *resource, const gsml_operator condition_operator = gsml_operator::assignment)
		: condition<site>(condition_operator), resource(resource)
	{
	}

	explicit resource_condition(const std::string &value, const gsml_operator condition_operator)
		: resource_condition(resource::get(value), condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "resource";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_resource() == this->resource;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} resource", this->resource->get_name());
	}

private:
	const metternich::resource *resource = nullptr;
};

}
