#pragma once

#include "country/country.h"
#include "map/province.h"
#include "script/condition/condition.h"
#include "util/vector_util.h"

namespace metternich {

class core_condition final : public condition<province>
{
public:
	explicit core_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<province>(condition_operator)
	{
		this->country = country::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "core";
		return class_identifier;
	}

	virtual bool check_assignment(const province *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return vector::contains(scope->get_core_countries(), this->country);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return condition<province>::get_object_string(this->country) + " core";
	}

private:
	const metternich::country *country = nullptr;
};

}
