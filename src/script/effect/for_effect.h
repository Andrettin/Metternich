#pragma once

#include "script/condition/and_condition.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class for_effect final : public effect<scope_type>
{
public:
	explicit for_effect(const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "for";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "count") {
			this->count = std::stoi(value);
		} else {
			this->effects.process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		this->effects.process_gsml_scope(scope);
	}

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(scope_type *scope, context &ctx) const override
	{
		for (int i = 0; i < this->count; ++i) {
			co_await this->effects.do_effects(scope, ctx);
		}
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		std::string str = "This will occur " + std::to_string(this->count) + " times:\n";
		str += this->effects.get_effects_string(scope, ctx, indent + 1, prefix, false);
		return str;
	}

private:
	int count = 0;
	effect_list<scope_type> effects;
};

}
