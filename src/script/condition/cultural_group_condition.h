#pragma once

#include "country/cultural_group.h"
#include "country/culture.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class cultural_group_condition final : public condition<scope_type>
{
public:
	explicit cultural_group_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->cultural_group = cultural_group::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "cultural_group";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const culture *culture = nullptr;

		if constexpr (std::is_same_v<scope_type, character> || std::is_same_v<scope_type, country> || std::is_same_v<scope_type, military_unit> || std::is_same_v<scope_type, population_unit>) {
			culture = scope->get_culture();
		} else {
			culture = scope->get_game_data()->get_culture();
		}

		if (culture != nullptr) {
			return culture->is_part_of_group(this->cultural_group);
		}

		return this->cultural_group == nullptr;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return string::highlight(this->cultural_group->get_name()) + " cultural group";
	}

private:
	const metternich::cultural_group *cultural_group = nullptr;
};

}
