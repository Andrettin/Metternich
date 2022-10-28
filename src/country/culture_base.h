#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "unit/civilian_unit_class_container.h"

namespace metternich {

class civilian_unit_type;

class culture_base : public named_data_entry
{
	Q_OBJECT

public:
	explicit culture_base(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const civilian_unit_type *get_civilian_class_unit_type(const civilian_unit_class *unit_class) const
	{
		const auto find_iterator = this->civilian_class_unit_types.find(unit_class);
		if (find_iterator != this->civilian_class_unit_types.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_civilian_class_unit_type(const civilian_unit_class *unit_class, const civilian_unit_type *unit_type)
	{
		if (unit_type == nullptr) {
			this->civilian_class_unit_types.erase(unit_class);
			return;
		}

		this->civilian_class_unit_types[unit_class] = unit_type;
	}

private:
	civilian_unit_class_map<const civilian_unit_type *> civilian_class_unit_types;
};

}
