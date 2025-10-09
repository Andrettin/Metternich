#pragma once

#include "domain/domain.h"
#include "item/item_type.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class item_condition final : public condition<scope_type>
{
public:
	explicit item_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->item_type = item_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "item";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const character *character = nullptr;

		if constexpr (std::is_same_v<scope_type, metternich::character>) {
			character = scope;
		} else if constexpr (std::is_same_v<scope_type, domain>) {
			character = scope->get_government()->get_ruler();
		} else {
			static_assert(false);
		}

		if (character == nullptr) {
			return false;
		}

		return character->get_game_data()->has_item(this->item_type);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} item", this->item_type->get_name());
	}

private:
	const metternich::item_type *item_type = nullptr;
};

}
