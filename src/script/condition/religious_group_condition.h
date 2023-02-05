#pragma once

#include "country/religion.h"
#include "country/religious_group.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class religious_group_condition final : public condition<scope_type>
{
public:
	explicit religious_group_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->religious_group = religious_group::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "religious_group";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const religion *religion = nullptr;

		if constexpr (std::is_same_v<scope_type, character> || std::is_same_v<scope_type, population_unit>) {
			religion = scope->get_religion();
		} else {
			religion = scope->get_game_data()->get_religion();
		}

		const metternich::religious_group *religious_group = nullptr;
		if (religion != nullptr) {
			religious_group = religion->get_group();
		}

		return religious_group == this->religious_group;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->religious_group->get_name() + " religious group";
	}

private:
	const metternich::religious_group *religious_group = nullptr;
};

}
