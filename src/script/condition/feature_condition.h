#pragma once

#include "map/site.h"
#include "map/site_feature.h"
#include "map/site_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

class feature_condition final : public condition<site>
{
public:
	explicit feature_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->feature = site_feature::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "feature";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
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
	const site_feature *feature = nullptr;
};

}
