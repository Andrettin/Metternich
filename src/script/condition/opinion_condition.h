#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/condition/numerical_condition.h"
#include "script/domain_target_variant.h"

namespace metternich {

class opinion_condition final : public numerical_condition<domain, read_only_context>
{
public:
	explicit opinion_condition(const gsml_operator condition_operator)
		: numerical_condition<domain, read_only_context>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "opinion";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "domain") {
			this->domain_target = string_to_target_variant<const domain>(value);
		} else if (key == "value") {
			this->set_base_value(std::stoi(value));
			this->set_condition_operator(property.get_operator());
		} else {
			numerical_condition<domain, read_only_context>::process_gsml_property(property);
		}
	}

	virtual int get_scope_value(const domain *scope, const read_only_context &ctx) const override
	{
		return scope->get_game_data()->get_opinion_of(get_domain_from_target(this->domain_target, ctx));
	}

	virtual std::string get_value_name() const override
	{
		std::string domain_name;

		if (std::holds_alternative<const domain *>(this->domain_target)) {
			domain_name = std::get<const metternich::domain *>(this->domain_target)->get_name();
		} else if (std::holds_alternative<special_target_type>(this->domain_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->domain_target);
			domain_name = std::format("{} scope domain", magic_enum::enum_name(target_type));
		} else {
			assert_throw(false);
			return std::string();
		}

		return std::format("Opinion of {}", domain_name);
	}

private:
	target_variant<const domain> domain_target;
};

}
