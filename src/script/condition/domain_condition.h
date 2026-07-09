#pragma once

#include "domain/domain.h"
#include "script/condition/condition.h"
#include "script/domain_target_variant.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class domain_condition final : public condition<scope_type>
{
public:
	explicit domain_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->domain_target = string_to_target_variant<const domain>(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "domain";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		const domain *domain = condition<scope_type>::get_scope_domain(scope);

		return domain == get_domain_from_target(this->domain_target, ctx);
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

private:
	domain_target_variant domain_target;
};

}
