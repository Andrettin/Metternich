#pragma once

#include "domain/domain.h"
#include "script/condition/condition.h"
#include "script/target_variant.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class country_condition final : public condition<scope_type>
{
public:
	explicit country_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->domain_target = string_to_target_variant<const domain>(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "country";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		const domain *domain = condition<scope_type>::get_scope_country(scope);

		return domain == this->get_domain(ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (std::holds_alternative<const domain *>(this->domain_target)) {
			const std::string domain_name = std::get<const metternich::domain *>(this->domain_target)->get_name();

			if constexpr (std::is_same_v<scope_type, metternich::domain>) {
				return "Is " + domain_name;
			} else {
				return domain_name + " domain";
			}
		} else if (std::holds_alternative<special_target_type>(this->domain_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->domain_target);
			return string::capitalized(std::string(magic_enum::enum_name(target_type))) + " scope domain";
		} else {
			assert_throw(false);
			return std::string();
		}
	}

	const domain *get_domain(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const domain *>(this->domain_target)) {
			return std::get<const domain *>(this->domain_target);
		} else if (std::holds_alternative<std::string>(this->domain_target)) {
			return ctx.get_saved_scope<const domain>(std::get<std::string>(this->domain_target));
		} else if (std::holds_alternative<special_target_type>(this->domain_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->domain_target);
			const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

			return std::visit([](auto &&target_scope) -> const domain * {
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
	target_variant<const domain> domain_target;
};

}
