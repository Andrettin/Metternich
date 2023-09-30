#pragma once

#include "country/country.h"
#include "script/condition/condition.h"
#include "script/target_variant.h"
#include "util/assert_util.h"
#include "util/enum_converter.h"

namespace metternich {

template <typename scope_type>
class country_condition final : public condition<scope_type>
{
public:
	explicit country_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->country_target = string_to_target_variant<const country>(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "country";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const country *country = condition<scope_type>::get_scope_country(scope);

		return country == this->get_country(ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (std::holds_alternative<const country *>(this->country_target)) {
			const std::string country_name = std::get<const metternich::country *>(this->country_target)->get_name();

			if constexpr (std::is_same_v<scope_type, metternich::country>) {
				return "Is " + country_name;
			} else {
				return country_name + " country";
			}
		} else if (std::holds_alternative<special_target_type>(this->country_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->country_target);
			return string::capitalized(enum_converter<special_target_type>::to_string(target_type)) + " scope country";
		} else {
			assert_throw(false);
			return std::string();
		}
	}

	const country *get_country(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const country *>(this->country_target)) {
			return std::get<const country *>(this->country_target);
		} else if (std::holds_alternative<std::string>(this->country_target)) {
			return ctx.get_saved_scope<const country>(std::get<std::string>(this->country_target));
		} else if (std::holds_alternative<special_target_type>(this->country_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->country_target);
			const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

			return std::visit([](auto &&target_scope) -> const country * {
				using target_scope_type = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(target_scope)>>>;

				if constexpr (std::is_same_v<target_scope_type, std::monostate>) {
					assert_throw(false);
					return nullptr;
				} else {
					return condition<target_scope_type>::get_scope_country(target_scope);
				}
			}, target_scope_variant);
		}

		assert_throw(false);
		return nullptr;
	}

private:
	target_variant<const country> country_target;
};

}
