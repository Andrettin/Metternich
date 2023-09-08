#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/effect/effect.h"
#include "unit/military_unit.h"
#include "unit/military_unit_domain.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class create_military_unit_effect final : public effect<const country>
{
public:
	explicit create_military_unit_effect(const std::string &value, const gsml_operator effect_operator) : effect<const country>(effect_operator)
	{
		this->type = military_unit_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "create_military_unit";
		return class_identifier;
	}

	virtual void do_assignment_effect(const country *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		const province *province = nullptr;
		if (scope->get_game_data()->is_under_anarchy()) {
			province = scope->get_game_data()->get_provinces().at(0);
		} else {
			province = scope->get_capital_province();
		}

		assert_throw(province != nullptr);
		assert_throw(province->get_game_data()->is_on_map());

		const culture *culture = scope->get_culture();
		assert_throw(culture != nullptr);

		const population_type *population_type = culture->get_population_class_type(defines::get()->get_default_population_class());
		assert_throw(population_type != nullptr);

		auto military_unit = make_qunique<metternich::military_unit>(this->type, scope, population_type, culture, scope->get_game_data()->get_religion(), culture->get_default_phenotype(), province->get_capital_settlement());
		military_unit->set_province(province);

		scope->get_game_data()->add_military_unit(std::move(military_unit));
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Gain {} {} {}", this->count, this->type->get_name(), this->type->get_domain() == military_unit_domain::water ? "ship" : "regiment");
	}

	virtual int get_score() const override
	{
		return this->type->get_score() * this->count;
	}

private:
	military_unit_type *type = nullptr;
	int count = 1;
};

}
