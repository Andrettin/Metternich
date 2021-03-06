#pragma once

#include "culture/culture.h"
#include "script/condition/condition.h"
#include "script/condition/condition_check_base.h"

namespace metternich {

class culture;

template <typename T>
class culture_condition final : public condition<T>
{
public:
	culture_condition(const std::string &culture_identifier, const gsml_operator effect_operator)
		: condition<T>(effect_operator)
	{
		this->culture = culture::get(culture_identifier);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "culture";
		return identifier;
	}

	virtual bool check_assignment(const T *scope) const override
	{
		return this->check_equality(scope);
	}

	virtual bool check_equality(const T *scope) const override
	{
		return scope->get_culture() == this->culture;
	}

	virtual void bind_condition_check(condition_check_base &check, const T *scope) const override
	{
		scope->connect(scope, &T::culture_changed, scope, [&check](){ check.set_result_recalculation_needed(); }, Qt::ConnectionType::DirectConnection);
	}

	virtual std::string get_assignment_string() const override
	{
		return this->get_equality_string();
	}

	virtual std::string get_equality_string() const override
	{
		return "Culture is " + this->culture->get_name();
	}

	virtual std::string get_inequality_string() const override
	{
		return "Culture is not " + this->culture->get_name();
	}

private:
	const culture *culture = nullptr;
};

}
