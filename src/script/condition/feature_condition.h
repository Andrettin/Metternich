#pragma once

#include "map/province_feature.h"
#include "map/site_feature.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class feature_condition final : public condition<scope_type>
{
public:
	using feature_type = std::conditional_t<std::is_same_v<scope_type, province>, province_feature, site_feature>;

	explicit feature_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->feature = feature_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "feature";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->has_feature(this->feature);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} feature", this->feature->get_name());
	}

private:
	const feature_type *feature = nullptr;
};

}
