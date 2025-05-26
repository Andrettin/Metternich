#pragma once

#include "character/character.h"
#include "script/condition/condition.h"
#include "script/target_variant.h"
#include "util/assert_util.h"

namespace metternich {

class character_condition final : public condition<character>
{
public:
	explicit character_condition(const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator)
	{
		this->character_target = string_to_target_variant<const character>(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "character";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope == this->get_character(ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (std::holds_alternative<const character *>(this->character_target)) {
			const std::string character_name = std::get<const character *>(this->character_target)->get_name();

			return std::format("Is {}", character_name);
		} else if (std::holds_alternative<special_target_type>(this->character_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->character_target);
			return string::capitalized(std::string(magic_enum::enum_name<special_target_type>(target_type))) + " scope character";
		} else {
			assert_throw(false);
			return std::string();
		}
	}

	const character *get_character(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const character *>(this->character_target)) {
			return std::get<const character *>(this->character_target);
		} else if (std::holds_alternative<std::string>(this->character_target)) {
			return ctx.get_saved_scope<const character>(std::get<std::string>(this->character_target));
		} else if (std::holds_alternative<special_target_type>(this->character_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->character_target);
			const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

			return std::visit([](auto &&target_scope) -> const character * {
				using target_scope_type = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(target_scope)>>>;

				if constexpr (std::is_same_v<target_scope_type, std::monostate>) {
					assert_throw(false);
					return nullptr;
				} else if constexpr (std::is_same_v<target_scope_type, const character>) {
					return target_scope;
				} else {
					assert_throw(false);
					return nullptr;
				}
			}, target_scope_variant);
		}

		assert_throw(false);
		return nullptr;
	}

private:
	target_variant<const character> character_target;
};

}
