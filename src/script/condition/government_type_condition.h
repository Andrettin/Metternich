#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/government_type.h"
#include "script/condition/condition.h"

namespace metternich {

class government_type_condition final : public condition<domain>
{
public:
	explicit government_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<domain>(condition_operator)
	{
		this->government_type = government_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "government_type";
		return class_identifier;
	}

	virtual bool check_assignment(const domain *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_government_type() == this->government_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} government type", string::highlight(this->government_type->get_name()));
	}

private:
	const metternich::government_type *government_type = nullptr;
};

}
