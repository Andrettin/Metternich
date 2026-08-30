#pragma once

#include "domain/domain.h"
#include "item/item.h"
#include "item/item_class.h"
#include "item/item_type.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class item_condition final : public condition<scope_type>
{
public:
	explicit item_condition(const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
	}

	explicit item_condition(const std::string &value, const gsml_operator condition_operator)
		: item_condition(condition_operator)
	{
		this->item_type = item_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "item";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "item_type") {
			this->item_type = item_type::get(value);
		} else if (key == "item_class") {
			this->item_class = item_class::get(value);
		} else {
			condition<scope_type>::process_gsml_property(property);
		}
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

		for (const qunique_ptr<item> &item : character->get_game_data()->get_items()) {
			if (this->item_type != nullptr && item->get_type() != this->item_type) {
				continue;
			}

			if (this->item_class != nullptr && item->get_type()->get_item_class() != this->item_class) {
				continue;
			}

			return true;
		}

		return false;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		assert_throw(this->item_type != nullptr || this->item_class != nullptr);

		return std::format("{} item", item::create_name(this->item_type != nullptr ? this->item_type->get_name() : this->item_class->get_name(), nullptr, nullptr, nullptr, nullptr));
	}

private:
	const metternich::item_type *item_type = nullptr;
	const metternich::item_class *item_class = nullptr;
};

}
