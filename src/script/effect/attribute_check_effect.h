#pragma once

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_game_data.h"
#include "domain/domain.h"
#include "domain/domain_attribute.h"
#include "domain/domain_game_data.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "engine_interface.h"
#include "script/context.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "util/assert_util.h"
#include "util/number_util.h"
#include "util/random.h"

namespace metternich {

template <typename scope_type> 
class attribute_check_effect final : public effect<scope_type>
{
public:
	using attribute_type = std::conditional_t<std::is_same_v<scope_type, const domain>, domain_attribute, character_attribute>;

	explicit attribute_check_effect(const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "attribute_check";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "attribute") {
			this->attribute = attribute_type::get(value);
		} else if (key == "roll_modifier") {
			this->roll_modifier = std::stoi(value);
		} else {
			effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "on_success") {
			this->success_effects = std::make_unique<effect_list<scope_type>>();
			this->success_effects->process_gsml_data(scope);
		} else if (tag == "on_failure") {
			this->failure_effects = std::make_unique<effect_list<scope_type>>();
			this->failure_effects->process_gsml_data(scope);
		} else {
			effect<scope_type>::process_gsml_scope(scope);
		}
	}

	virtual void check() const override
	{
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		const bool success = scope->get_game_data()->do_attribute_check(this->attribute, this->roll_modifier);

		bool is_player = false;
		if constexpr (std::is_same_v<scope_type, const character>) {
			is_player = scope == game::get()->get_player_character();
		} else if constexpr (std::is_same_v<scope_type, const domain>) {
			is_player = scope == game::get()->get_player_country();
		}

		if (is_player) {
			const domain *domain = effect<scope_type>::get_scope_domain(scope);
			const portrait *interior_minister_portrait = domain->get_government()->get_interior_minister_portrait();

			const std::string effects_string = success ? this->get_success_effects_string(scope, ctx, 0, "") : this->get_failure_effects_string(scope, ctx, 0, "");

			if (success) {
				engine_interface::get()->add_notification("Attribute Check Successful!", interior_minister_portrait, std::format("You have succeeded in {} {} attribute check!{}", string::get_indefinite_article(this->attribute->get_name()), this->attribute->get_name(), !effects_string.empty() ? ("\n\n" + effects_string) : ""));
			} else {
				engine_interface::get()->add_notification("Attribute Check Failed!", interior_minister_portrait, std::format("You have failed {} {} attribute check!{}", string::get_indefinite_article(this->attribute->get_name()), this->attribute->get_name(), !effects_string.empty() ? ("\n\n" + effects_string) : ""));
			}
		}

		if (success) {
			if (this->success_effects != nullptr) {
				this->success_effects->do_effects(scope, ctx);
			}
		} else {
			if (this->failure_effects != nullptr) {
				this->failure_effects->do_effects(scope, ctx);
			}
		}
	}

	virtual std::string get_assignment_string(scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		std::string str = std::format("{} Check ({}% Chance)", this->attribute->get_name(), scope->get_game_data()->get_attribute_check_chance(this->attribute, this->roll_modifier));

		if (this->roll_modifier != 0) {
			str += "\n" + std::string(indent + 1, '\t') + std::format("Roll Modifier: {}", number::to_signed_string(this->roll_modifier));
		}

		const std::string success_effects_string = this->get_success_effects_string(scope, ctx, indent + 1, prefix);
		if (!success_effects_string.empty()) {
			str += "\n" + std::string(indent, '\t') + "If successful:\n" + success_effects_string;
		}

		const std::string failure_effects_string = this->get_failure_effects_string(scope, ctx, indent + 1, prefix);
		if (!failure_effects_string.empty()) {
			str += "\n" + std::string(indent, '\t') + "If failed:\n" + failure_effects_string;
		}

		return str;
	}

	std::string get_success_effects_string(scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
	{
		if (this->success_effects != nullptr) {
			return this->success_effects->get_effects_string(scope, ctx, indent, prefix);
		}

		return {};
	}

	std::string get_failure_effects_string(scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
	{
		if (this->failure_effects != nullptr) {
			return this->failure_effects->get_effects_string(scope, ctx, indent, prefix);
		}

		return {};
	}

private:
	const attribute_type *attribute = nullptr;
	int roll_modifier = 0;
	std::unique_ptr<effect_list<scope_type>> success_effects;
	std::unique_ptr<effect_list<scope_type>> failure_effects;
};

}
