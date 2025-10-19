#pragma once

#include "infrastructure/dungeon_area.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class dungeon_area_condition final : public condition<scope_type>
{
public:
	explicit dungeon_area_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->dungeon_area = dungeon_area::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "dungeon_area";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);

		return ctx.dungeon_area == this->dungeon_area;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} dungeon area", this->dungeon_area->get_name());
	}

private:
	const metternich::dungeon_area *dungeon_area = nullptr;
};

}
