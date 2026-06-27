#pragma once

#include "character/dynasty.h"
#include "script/condition/condition.h"
#include "script/target_variant.h"

namespace metternich {

template <typename scope_type>
class dynasty_condition final : public condition<scope_type>
{
public:
	explicit dynasty_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		if (magic_enum::enum_contains<special_target_type>(value)) {
			this->dynasty_target = magic_enum::enum_cast<special_target_type>(value).value();
		} else {
			this->dynasty_target = dynasty::get(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "dynasty";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const metternich::dynasty *dynasty = nullptr;

		if constexpr (std::is_same_v<scope_type, character>) {
			dynasty = scope->get_dynasty();
		} else {
			dynasty = scope->get_game_data()->get_dynasty();
		}

		return dynasty == this->get_dynasty(ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (std::holds_alternative<const dynasty *>(this->dynasty_target)) {
			return std::get<const dynasty *>(this->dynasty_target)->get_name() + " dynasty";
		} else if (std::holds_alternative<special_target_type>(this->dynasty_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->dynasty_target);
			return string::capitalized(std::string(magic_enum::enum_name(target_type))) + " scope dynasty";
		} else {
			assert_throw(false);
			return std::string();
		}
	}

	const dynasty *get_dynasty(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const dynasty *>(this->dynasty_target)) {
			return std::get<const dynasty *>(this->dynasty_target);
		} else if (std::holds_alternative<special_target_type>(this->dynasty_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->dynasty_target);
			const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

			return std::visit([](auto &&target_scope) -> const dynasty * {
				using target_scope_type = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(target_scope)>>>;

				if constexpr (std::is_same_v<target_scope_type, std::monostate>) {
					assert_throw(false);
					return nullptr;
				} else if constexpr (std::is_same_v<target_scope_type, domain>) {
					return target_scope->get_game_data()->get_dynasty();
				} else if constexpr (std::is_same_v<target_scope_type, character>) {
					return target_scope->get_dynasty();
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
	target_variant<const dynasty> dynasty_target;
};

}
