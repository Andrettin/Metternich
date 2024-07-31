#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/effect/effect.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class create_military_unit_effect final : public effect<const country>
{
public:
	explicit create_military_unit_effect(const gsml_operator effect_operator)
		: effect<const country>(effect_operator)
	{
	}

	explicit create_military_unit_effect(const std::string &value, const gsml_operator effect_operator)
		: create_military_unit_effect(effect_operator)
	{
		this->type = military_unit_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "create_military_unit";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "type") {
			this->type = military_unit_type::get(value);
		} else if (key == "unit_class") {
			this->unit_class = military_unit_class::get(value);
		} else if (key == "category") {
			this->category = enum_converter<military_unit_category>::to_enum(value);
		} else if (key == "count") {
			this->count = std::stoi(value);
		} else {
			effect<const country>::process_gsml_property(property);
		}
	}

	virtual void do_assignment_effect(const country *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (scope->get_game_data()->is_under_anarchy()) {
			return;
		}

		const military_unit_type *type = this->get_type(scope);

		if (type == nullptr) {
			return;
		}

		const province *province = scope->get_game_data()->get_capital_province();

		assert_throw(province != nullptr);
		assert_throw(province->get_game_data()->is_on_map());

		const culture *culture = scope->get_culture();
		assert_throw(culture != nullptr);

		const population_type *population_type = culture->get_population_class_type(scope->get_game_data()->get_default_population_class());
		assert_throw(population_type != nullptr);

		auto military_unit = make_qunique<metternich::military_unit>(type, scope, population_type, culture, scope->get_game_data()->get_religion(), culture->get_default_phenotype(), scope->get_game_data()->get_capital());
		military_unit->set_province(province);

		scope->get_game_data()->add_military_unit(std::move(military_unit));
	}

	virtual std::string get_assignment_string(const country *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		const military_unit_type *type = this->get_type(scope);

		if (type == nullptr) {
			return std::string();
		}

		return std::format("Gain {} {} {}", this->count, string::highlight(type->get_name()), type->is_ship() ? "warship" : "regiment");
	}

	const military_unit_type *get_type(const country *scope) const
	{
		if (this->type != nullptr) {
			return this->type;
		} else if (this->unit_class != nullptr) {
			return scope->get_culture()->get_military_class_unit_type(this->unit_class);
		} else if (this->category != military_unit_category::none) {
			return scope->get_game_data()->get_best_military_unit_category_type(this->category);
		} else {
			assert_throw(false);
			return nullptr;
		}
	}

private:
	const military_unit_type *type = nullptr;
	const military_unit_class *unit_class = nullptr;
	military_unit_category category = military_unit_category::none;
	int count = 1;
};

}
