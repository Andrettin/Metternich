#pragma once

#include "map/site.h"
#include "script/condition/condition.h"
#include "script/target_variant.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class site_condition final : public condition<site>
{
public:
	explicit site_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->site_target = string_to_target_variant<const site>(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "site";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		return scope == this->get_site(ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (std::holds_alternative<const site *>(this->site_target)) {
			const std::string site_name = std::get<const site *>(this->site_target)->get_name();

			return std::format("Is {}", site_name);
		} else if (std::holds_alternative<special_target_type>(this->site_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->site_target);
			return string::capitalized(std::string(magic_enum::enum_name(target_type))) + " scope site";
		} else {
			assert_throw(false);
			return std::string();
		}
	}

	const site *get_site(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const site *>(this->site_target)) {
			return std::get<const site *>(this->site_target);
		} else if (std::holds_alternative<std::string>(this->site_target)) {
			return ctx.get_saved_scope<const site>(std::get<std::string>(this->site_target));
		} else if (std::holds_alternative<special_target_type>(this->site_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->site_target);
			const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

			return std::visit([](auto &&target_scope) -> const site * {
				using target_scope_type = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(target_scope)>>>;

				if constexpr (std::is_same_v<target_scope_type, std::monostate>) {
					assert_throw(false);
					return nullptr;
				} else if constexpr (std::is_same_v<target_scope_type, const site>) {
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
	target_variant<const site> site_target;
};

}
