#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "technology/technology_category.h"

namespace metternich {

template <typename scope_type>
class category_research_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit category_research_modifier_effect(const technology_category category, const std::string &value)
		: modifier_effect<scope_type>(value), category(category)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "category_research_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_category_research_modifier(this->category, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} Research", get_technology_category_name(this->category));
	}

	virtual bool is_percent() const override
	{
		return true;
	}

private:
	const technology_category category = technology_category::none;
};

}
