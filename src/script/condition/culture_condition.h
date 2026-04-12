#pragma once

#include "culture/culture.h"
#include "script/condition/condition.h"
#include "script/target_variant.h"

namespace metternich {

template <typename scope_type>
class culture_condition final : public condition<scope_type>
{
public:
	explicit culture_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		if (magic_enum::enum_contains<special_target_type>(value)) {
			this->culture_target = magic_enum::enum_cast<special_target_type>(value).value();
		} else {
			this->culture_target = culture::get(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "culture";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const metternich::culture *culture = nullptr;

		if constexpr (std::is_same_v<scope_type, character> || std::is_same_v<scope_type, military_unit> || std::is_same_v<scope_type, population_unit>) {
			culture = scope->get_culture();
		} else {
			culture = scope->get_game_data()->get_culture();
		}

		return culture == this->get_culture(ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (std::holds_alternative<const culture *>(this->culture_target)) {
			return std::get<const culture *>(this->culture_target)->get_name() + " culture";
		} else if (std::holds_alternative<special_target_type>(this->culture_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->culture_target);
			return string::capitalized(std::string(magic_enum::enum_name(target_type))) + " scope culture";
		} else {
			assert_throw(false);
			return std::string();
		}
	}

	const culture *get_culture(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const culture *>(this->culture_target)) {
			return std::get<const culture *>(this->culture_target);
		} else if (std::holds_alternative<special_target_type>(this->culture_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->culture_target);
			const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

			return std::visit([](auto &&target_scope) -> const culture * {
				using target_scope_type = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(target_scope)>>>;

				if constexpr (std::is_same_v<target_scope_type, std::monostate>) {
					assert_throw(false);
					return nullptr;
				} else if constexpr (std::is_same_v<target_scope_type, domain> || std::is_same_v<target_scope_type, province> || std::is_same_v<target_scope_type, site>) {
					return target_scope->get_game_data()->get_culture();
				} else {
					return target_scope->get_culture();
				}
			}, target_scope_variant);
		}

		assert_throw(false);
		return nullptr;
	}

private:
	target_variant<const culture> culture_target;
};

}
