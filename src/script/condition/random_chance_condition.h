#pragma once

#include "script/condition/condition.h"
#include "util/decimillesimal_int.h"
#include "util/random.h"

namespace metternich {

template <typename scope_type>
class random_chance_condition final : public condition<scope_type>
{
public:
	explicit random_chance_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->chance = decimillesimal_int(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "random_chance";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);

		const int64_t random_number = random::get()->generate(decimillesimal_int::divisor * 100);
		return this->chance.get_value() > random_number;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{}% chance", this->chance.to_string());
	}

private:
	decimillesimal_int chance;
};

}
