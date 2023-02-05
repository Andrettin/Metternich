#pragma once

#include "country/religion.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class religion_condition final : public condition<scope_type>
{
public:
	explicit religion_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->religion = religion::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "religion";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const metternich::religion *religion = nullptr;

		if constexpr (std::is_same_v<scope_type, character> || std::is_same_v<scope_type, population_unit>) {
			religion = scope->get_religion();
		} else {
			religion = scope->get_game_data()->get_religion();
		}

		return religion == this->religion;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->religion->get_name() + " religion";
	}

private:
	const metternich::religion *religion = nullptr;
};

}
