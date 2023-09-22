#pragma once

#include "country/religion.h"
#include "script/condition/condition.h"
#include "script/target_variant.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class religion_condition final : public condition<scope_type>
{
public:
	explicit religion_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		if (enum_converter<special_target_type>::has_value(value)) {
			this->religion_target = enum_converter<special_target_type>::to_enum(value);
		} else {
			this->religion_target = religion::get(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "religion";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const metternich::religion *religion = nullptr;

		if constexpr (std::is_same_v<scope_type, character> || std::is_same_v<scope_type, military_unit> || std::is_same_v<scope_type, population_unit>) {
			religion = scope->get_religion();
		} else {
			religion = scope->get_game_data()->get_religion();
		}

		return religion == this->get_religion(ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (std::holds_alternative<const religion *>(this->religion_target)) {
			return std::get<const religion *>(this->religion_target)->get_name() + " religion";
		} else if (std::holds_alternative<special_target_type>(this->religion_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->religion_target);
			return string::capitalized(enum_converter<special_target_type>::to_string(target_type)) + " scope religion";
		} else {
			assert_throw(false);
			return std::string();
		}
	}

	const religion *get_religion(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const religion *>(this->religion_target)) {
			return std::get<const religion *>(this->religion_target);
		} else if (std::holds_alternative<special_target_type>(this->religion_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->religion_target);
			const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

			return std::visit([](auto &&target_scope) -> const religion * {
				using target_scope_type = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(target_scope)>>>;

				if constexpr (std::is_same_v<target_scope_type, std::monostate>) {
					assert_throw(false);
					return nullptr;
				} else if constexpr (std::is_same_v<target_scope_type, country> || std::is_same_v<target_scope_type, province> || std::is_same_v<target_scope_type, site>) {
					return target_scope->get_game_data()->get_religion();
				} else {
					return target_scope->get_religion();
				}
			}, target_scope_variant);
		}

		assert_throw(false);
		return nullptr;
	}

private:
	target_variant<const religion> religion_target;
};

}
