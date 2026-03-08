#pragma once

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/assert_util.h"

namespace metternich {

class attribute_skill_bonus_modifier_effect final : public modifier_effect<const character>
{
public:
	attribute_skill_bonus_modifier_effect() = default;

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "attribute_skill_bonus";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "attribute") {
			this->attribute = character_attribute::get(value);
		} else if (key == "bonus") {
			this->value = centesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const character *scope, const centesimal_int &multiplier) const override
	{
		assert_throw(this->attribute != nullptr);

		//FIXME: should grant a bonus not only to skills which have their value based upon that of the attribute, but any skills for which the attribute grants the attribute modifier
		for (const skill *skill : this->attribute->get_derived_skills()) {
			scope->get_game_data()->change_skill_value(skill, (this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		assert_throw(this->attribute != nullptr);

		return std::format("{} Skills", this->attribute->get_name());
	}

private:
	const character_attribute *attribute = nullptr;
};

}
