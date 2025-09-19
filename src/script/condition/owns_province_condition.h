#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

class owns_province_condition final : public condition<domain>
{
public:
	explicit owns_province_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<domain>(condition_operator)
	{
		this->province = province::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "owns_province";
		return class_identifier;
	}

	virtual bool check_assignment(const domain *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return this->province->get_game_data()->get_owner() == scope;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return "Owns the " + this->province->get_game_data()->get_current_cultural_name() + " province";
	}

private:
	const metternich::province *province = nullptr;
};

}
