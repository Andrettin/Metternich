#pragma once

#include "domain/country_technology.h"
#include "domain/domain.h"
#include "script/condition/condition.h"
#include "technology/technology.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class technology_condition final : public condition<scope_type>
{
public:
	explicit technology_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->technology = technology::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "technology";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if constexpr (std::is_same_v<scope_type, province>) {
			return scope->get_game_data()->has_technology(this->technology);
		} else {
			const domain *domain = condition<scope_type>::get_scope_domain(scope);

			if (domain == nullptr) {
				return false;
			}

			return domain->get_technology()->has_technology(this->technology);
		}
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} technology", string::highlight(this->technology->get_name()));
	}

private:
	const metternich::technology *technology = nullptr;
};

}
