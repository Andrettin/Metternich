#pragma once

#include "map/region.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class region_condition final : public condition<scope_type>
{
public:
	explicit region_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->region = region::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "region";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const province *province = condition<scope_type>::get_scope_province(scope);
		if (province == nullptr) {
			return false;
		}

		if (!province->get_game_data()->is_on_map()) {
			return false;
		}

		return vector::contains(province->get_regions(), this->region);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} region", string::highlight(this->region->get_name()));
	}

private:
	const metternich::region *region = nullptr;
};

}
