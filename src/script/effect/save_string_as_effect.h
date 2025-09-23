#pragma once

#include "script/context.h"
#include "script/effect/effect.h"

namespace metternich {

template <typename scope_type>
class save_string_as_effect final : public effect<scope_type>
{
public:
	explicit save_string_as_effect(const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "save_string_as";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "name") {
			this->name = value;
		} else if (key == "string") {
			this->string = value;
		} else {
			effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		Q_UNUSED(scope);

		ctx.saved_strings[this->name] = this->string;
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	std::string name;
	std::string string;
};

}
