#pragma once

#include "character/character.h"
#include "character/mythic_path.h"
#include "script/condition/condition.h"

namespace metternich {

class mythic_path_condition final : public condition<character>
{
public:
	explicit mythic_path_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->mythic_path = mythic_path::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "mythic_path";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_mythic_path() == this->mythic_path;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} mythic path", this->mythic_path->get_name());
	}

private:
	const metternich::mythic_path *mythic_path = nullptr;
};

}
