#pragma once

#include "infrastructure/improvement.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class add_improvement_effect final : public effect<const site>
{
public:
	explicit add_improvement_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<const site>(effect_operator)
	{
		this->improvement = improvement::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "add_improvement";
		return class_identifier;
	}

	virtual void do_assignment_effect(const site *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (!this->improvement->is_buildable_on_site(scope)) {
			return;
		}

		scope->get_game_data()->set_improvement(this->improvement->get_slot(), this->improvement);
	}

	virtual std::string get_assignment_string(const site *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		return std::format("Gain {} improvement", string::highlight(this->improvement->get_name()));
	}

private:
	const metternich::improvement *improvement = nullptr;
};

}
