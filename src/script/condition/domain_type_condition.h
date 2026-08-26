#pragma once

#include "domain/domain.h"
#include "domain/domain_type.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

class domain_type_condition final : public condition<domain>
{
public:
	explicit domain_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<domain>(condition_operator)
	{
		this->domain_type = magic_enum::enum_cast<metternich::domain_type>(value).value();
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "domain_type";
		return class_identifier;
	}

	virtual bool check_assignment(const domain *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_type() == this->domain_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} domain type", string::highlight(get_domain_type_name(this->domain_type)));
	}

private:
	metternich::domain_type domain_type;
};

}
