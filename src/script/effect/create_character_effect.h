#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/monster_type.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class create_character_effect final : public effect<const domain>
{
public:
	explicit create_character_effect(const gsml_operator effect_operator)
		: effect<const domain>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "create_character";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "character") {
			this->character = character::get(value);
		} else if (key == "monster_type") {
			this->monster_type = monster_type::get(value);
		} else if (key == "hit_points") {
			this->hit_points = std::stoi(value);
		} else if (key == "saved_scope") {
			this->saved_scope_name = value;
		} else {
			effect<const domain>::process_gsml_property(property);
		}
	}

	virtual void check() const override
	{
		assert_throw(this->character != nullptr || this->monster_type != nullptr);
	}

	virtual void do_assignment_effect(const domain *scope, context &ctx) const override
	{
		const metternich::character *created_character = nullptr;

		if (this->character != nullptr) {
			this->character->get_game_data()->set_start_date(game::get()->get_date());
			const QDate birth_date = this->character->generate_birth_date_from_start_date(this->character->get_game_data()->get_start_date());
			this->character->get_game_data()->set_birth_date(birth_date);
			if (!this->character->is_immortal()) {
				const QDate death_date = this->character->generate_death_date_from_birth_date(this->character->get_game_data()->get_birth_date());
				this->character->get_game_data()->set_death_date(death_date);
			}
			this->character->get_game_data()->set_dead(false);

			created_character = this->character;
		} else if (this->monster_type != nullptr) {
			created_character = character::generate(this->monster_type, nullptr, nullptr, nullptr, false);
		} else {
			assert_throw(false);
		}

		if (this->hit_points > 0) {
			created_character->get_game_data()->set_hit_points(this->hit_points);
		}

		created_character->get_game_data()->set_domain(scope);

		if (!this->saved_scope_name.empty()) {
			ctx.get_saved_scopes<const metternich::character>()[this->saved_scope_name] = created_character;
		}
	}

	virtual std::string get_assignment_string(const domain *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		if (this->character != nullptr) {
			return std::format("{} ({} {} {}) will join your domain", this->character->get_full_name(), this->character->get_species()->get_name(), this->character->get_game_data()->get_character_class()->get_name(), this->character->get_game_data()->get_level());
		} else if (this->monster_type != nullptr) {
			return std::format("{} {} will join your domain", string::get_indefinite_article(this->monster_type->get_name()), this->monster_type->get_name());
		} else {
			assert_throw(false);
			return {};
		}
	}

private:
	const metternich::character *character = nullptr;
	const metternich::monster_type *monster_type = nullptr;
	int hit_points = 0; //the hit points the character should start with (in case they would be hurt)
	std::string saved_scope_name;
};

}
