#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_rank.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class rank_condition final : public numerical_condition<domain, read_only_context>
{
public:
	explicit rank_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<domain, read_only_context>(condition_operator)
	{
		this->rank = domain_rank::get(value);
		this->set_base_value(this->rank->get_priority());
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "rank";
		return class_identifier;
	}

	virtual int get_scope_value(const domain *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (scope->get_game_data()->get_rank() == nullptr) {
			return 0;
		}

		return scope->get_game_data()->get_rank()->get_priority();
	}

	virtual std::string get_base_value_string() const override
	{
		return this->rank->get_name();
	}

	virtual std::string get_value_name() const override
	{
		return "rank";
	}

private:
	const domain_rank *rank = nullptr;
};

}
