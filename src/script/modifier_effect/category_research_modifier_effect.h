#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "technology/technology_category.h"
#include "util/number_util.h"

namespace metternich {

template <typename scope_type>
class category_research_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit category_research_modifier_effect(const technology_category category, const std::string &value)
		: category(category)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "category_research_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_category_research_modifier(this->category, (this->quantity * multiplier).to_int());
	}

	virtual std::string get_string(const centesimal_int &multiplier) const override
	{
		return std::format("{} Research: {}%", get_technology_category_name(this->category), number::to_signed_string((this->quantity * multiplier).to_int()));
	}

	virtual int get_score() const override
	{
		return this->quantity;
	}

private:
	const technology_category category = technology_category::none;
	int quantity = 0;
};

}
