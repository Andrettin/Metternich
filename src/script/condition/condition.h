#pragma once

#include "script/condition_base.h"
#include "script/context.h"

namespace metternich {

class country;
class province;
struct read_only_context;

template <typename scope_type>
class condition : public condition_base<scope_type, read_only_context>
{
public:
	static std::unique_ptr<const condition> from_gsml_property(const gsml_property &property);
	static std::unique_ptr<const condition> from_gsml_scope(const gsml_data &scope);

	static std::string get_conditions_string(const std::vector<std::unique_ptr<const condition<scope_type>>> &conditions, const size_t indent)
	{
		std::string conditions_string;
		bool first = true;
		for (const std::unique_ptr<const condition<scope_type>> &condition : conditions) {
			if (condition->is_hidden()) {
				continue;
			}

			const std::string condition_string = condition->get_string(indent);
			if (condition_string.empty()) {
				continue;
			}

			if (first) {
				first = false;
			} else {
				conditions_string += "\n";
			}

			if (indent > 0) {
				conditions_string += std::string(indent, '\t');
			}

			conditions_string += condition_string;
		}
		return conditions_string;
	}

	static const country *get_scope_country(const scope_type *scope);
	static const province *get_scope_province(const scope_type *scope);

	explicit condition(const gsml_operator condition_operator) : condition_base<scope_type, read_only_context>(condition_operator)
	{
	}

	virtual ~condition()
	{
	}
};

}
