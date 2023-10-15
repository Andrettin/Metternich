#pragma once

#include "country/country.h"
#include "game/game.h"
#include "script/condition/condition.h"
#include "util/vector_util.h"

namespace metternich {

template <typename scope_type>
class country_exists_condition final : public condition<scope_type>
{
public:
	explicit country_exists_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->country = country::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "country_exists";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);

		return vector::contains(game::get()->get_countries(), this->country);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->country->get_game_data()->get_name() + " exists";
	}

private:
	const metternich::country *country = nullptr;
};

}
