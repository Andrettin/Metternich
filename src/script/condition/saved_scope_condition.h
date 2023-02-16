#pragma once

#include "script/condition/scope_condition.h"
#include "util/assert_util.h"

namespace metternich {

template <typename upper_scope_type, typename scope_type>
class saved_scope_condition final : public scope_condition<upper_scope_type, scope_type>
{
public:
	explicit saved_scope_condition(const gsml_operator condition_operator)
		: scope_condition<upper_scope_type, scope_type>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "saved_scope";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		if (property.get_key() == "scope") {
			this->scope_name = property.get_value();
		} else {
			scope_condition<upper_scope_type, scope_type>::process_gsml_property(property);
		}
	}

	virtual const scope_type *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope);

		const std::map<std::string, const scope_type *> &saved_scopes = ctx.get_saved_scopes<const scope_type>();

		const auto find_iterator = saved_scopes.find(this->scope_name);
		assert_throw(find_iterator != saved_scopes.end());

		return find_iterator->second;
	}

	virtual std::string get_scope_name() const override
	{
		return "Saved Scope";
	}

private:
	std::string scope_name;
};

}
