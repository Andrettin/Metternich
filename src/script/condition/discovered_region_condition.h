#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "map/region.h"
#include "script/condition/condition.h"

namespace metternich {

class discovered_region_condition final : public condition<domain>
{
public:
	explicit discovered_region_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<domain>(condition_operator)
	{
		this->region = region::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "discovered_region";
		return class_identifier;
	}

	virtual bool check_assignment(const domain *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->is_region_discovered(this->region);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return "Discovered the " + this->region->get_name() + " region";
	}

private:
	const metternich::region *region = nullptr;
};

}
