#pragma once

#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class saved_building_condition final : public condition<scope_type>
{
public:
	explicit saved_building_condition(const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "saved_building";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "name") {
			this->name = value;
		} else if (key == "building_type") {
			this->building_type = building_type::get(value);
		} else if (key == "building_class") {
			this->building_class = building_class::get(value);
		} else {
			condition<scope_type>::process_gsml_property(property);
		}
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);

		const metternich::building_type *saved_building = ctx.get_saved_building(this->name);

		if (saved_building == nullptr) {
			return false;
		}

		if (this->building_class != nullptr && this->building_class == saved_building->get_building_class()) {
			return true;
		}

		return saved_building == this->building_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return "Saved Building";
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	std::string name;
	const metternich::building_type *building_type = nullptr;
	const metternich::building_class *building_class = nullptr;
};

}
