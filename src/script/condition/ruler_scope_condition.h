#pragma once

#include "character/character.h"
#include "script/condition/scope_condition.h"

namespace metternich {

class character;

template <typename upper_scope_type>
class ruler_scope_condition final : public scope_condition<upper_scope_type, character>
{
public:
	explicit ruler_scope_condition(const gsml_operator condition_operator)
		: scope_condition<upper_scope_type, character>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "ruler";
		return class_identifier;
	}

	virtual const character *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const country *country = condition<upper_scope_type>::get_scope_country(upper_scope);

		if (country == nullptr) {
			return nullptr;
		}

		return country->get_game_data()->get_ruler();
	}

	virtual std::string get_scope_name() const override
	{
		return "Ruler";
	}
};

}
