#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "script/effect/effect.h"
#include "unit/transporter.h"
#include "unit/transporter_category.h"
#include "unit/transporter_class.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class create_transporter_effect final : public effect<const country>
{
public:
	explicit create_transporter_effect(const gsml_operator effect_operator)
		: effect<const country>(effect_operator)
	{
	}

	explicit create_transporter_effect(const std::string &value, const gsml_operator effect_operator)
		: create_transporter_effect(effect_operator)
	{
		this->type = transporter_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "create_transporter";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "type") {
			this->type = transporter_type::get(value);
		} else if (key == "unit_class") {
			this->transporter_class = transporter_class::get(value);
		} else if (key == "category") {
			this->category = enum_converter<transporter_category>::to_enum(value);
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

		const transporter_type *type = this->get_type(scope);

		if (type == nullptr) {
			return;
		}

		const culture *culture = scope->get_culture();
		assert_throw(culture != nullptr);

		const population_type *population_type = culture->get_population_class_type(scope->get_game_data()->get_default_population_class());
		assert_throw(population_type != nullptr);

		auto transporter = make_qunique<metternich::transporter>(type, scope, population_type, culture, scope->get_game_data()->get_religion(), culture->get_default_phenotype(), scope->get_game_data()->get_capital());

		scope->get_game_data()->add_transporter(std::move(transporter));
	}

	virtual std::string get_assignment_string(const country *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		const transporter_type *type = this->get_type(scope);

		if (type == nullptr) {
			return std::string();
		}

		return std::format("Gain {} {} {}", this->count, string::highlight(type->get_name()), type->is_ship() ? "merchant ship" : "transporter");
	}

	const transporter_type *get_type(const country *scope) const
	{
		if (this->type != nullptr) {
			return this->type;
		} else if (this->transporter_class != nullptr) {
			return scope->get_culture()->get_transporter_class_type(this->transporter_class);
		} else if (this->category != transporter_category::none) {
			return scope->get_game_data()->get_best_transporter_category_type(this->category);
		} else {
			assert_throw(false);
			return nullptr;
		}
	}

private:
	const transporter_type *type = nullptr;
	const transporter_class *transporter_class = nullptr;
	transporter_category category = transporter_category::none;
	int count = 1;
};

}
