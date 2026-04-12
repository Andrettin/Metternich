#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/condition/condition.h"
#include "script/target_variant.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class realm_condition final : public condition<scope_type>
{
public:
	explicit realm_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->realm_target = string_to_target_variant<const domain>(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "realm";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		const domain *domain = condition<scope_type>::get_scope_domain(scope);
		const metternich::domain *realm = domain ? domain->get_game_data()->get_realm() : nullptr;

		return realm == this->get_realm(ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (std::holds_alternative<const domain *>(this->realm_target)) {
			const std::string realm_name = std::get<const metternich::domain *>(this->realm_target)->get_name();

			return realm_name + " realm";
		} else if (std::holds_alternative<special_target_type>(this->realm_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->realm_target);
			return string::capitalized(std::string(magic_enum::enum_name(target_type))) + " scope realm";
		} else {
			assert_throw(false);
			return std::string();
		}
	}

	const domain *get_realm(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const domain *>(this->realm_target)) {
			return std::get<const domain *>(this->realm_target);
		} else if (std::holds_alternative<std::string>(this->realm_target)) {
			return ctx.get_saved_scope<const domain>(std::get<std::string>(this->realm_target));
		} else if (std::holds_alternative<special_target_type>(this->realm_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->realm_target);
			const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

			return std::visit([](auto &&target_scope) -> const domain * {
				using target_scope_type = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(target_scope)>>>;

				if constexpr (std::is_same_v<target_scope_type, std::monostate>) {
					assert_throw(false);
					return nullptr;
				} else {
					const domain *domain = condition<target_scope_type>::get_scope_domain(target_scope);
					return domain ? domain->get_game_data()->get_realm() : nullptr;
				}
			}, target_scope_variant);
		}

		assert_throw(false);
		return nullptr;
	}

private:
	target_variant<const domain> realm_target;
};

}
