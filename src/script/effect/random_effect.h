#pragma once

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class random_effect final : public effect<scope_type>
{
public:
	explicit random_effect(const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "random";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "chance") {
			this->chance = decimillesimal_int(value);
		} else {
			this->effects.process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		this->effects.process_gsml_scope(scope);
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		const int64_t random_number = random::get()->generate(decimillesimal_int::divisor * 100);
		if (this->chance.get_value() <= random_number) {
			return;
		}

		this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		std::string str = this->chance.to_string() + "% chance:\n";
		return str + this->effects.get_effects_string(scope, ctx, indent + 1, prefix);
	}

private:
	decimillesimal_int chance;
	effect_list<scope_type> effects;
};

}
