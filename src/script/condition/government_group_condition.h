#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/government_group.h"
#include "domain/government_type.h"
#include "script/condition/condition.h"

namespace metternich {

class government_group_condition final : public condition<domain>
{
public:
	explicit government_group_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<domain>(condition_operator)
	{
		this->government_group = government_group::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "government_group";
		return class_identifier;
	}

	virtual bool check_assignment(const domain *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const metternich::government_group *domain_government_group = nullptr;
		if (scope->get_game_data()->get_government_type() != nullptr) {
			domain_government_group = scope->get_game_data()->get_government_type()->get_group();
		}

		return domain_government_group == this->government_group;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} government group", string::highlight(this->government_group->get_name()));
	}

private:
	const metternich::government_group *government_group = nullptr;
};

}
