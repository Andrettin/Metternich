#pragma once

#include "map/site.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class source_site_condition final : public condition<scope_type>
{
public:
	explicit source_site_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->site = site::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "source_site";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);

		return std::holds_alternative<const metternich::site *>(ctx.source_scope) && std::get<const metternich::site *>(ctx.source_scope) == this->site;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->site->get_name() + " is the source scope";
	}

private:
	const metternich::site *site = nullptr;
};

}
