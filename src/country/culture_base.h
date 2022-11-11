#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "population/population_class_container.h"
#include "unit/civilian_unit_class_container.h"

namespace metternich {

class civilian_unit_type;
class phenotype;
class population_type;

class culture_base : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::phenotype* default_phenotype MEMBER default_phenotype)

public:
	explicit culture_base(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const phenotype *get_default_phenotype() const
	{
		return this->default_phenotype;
	}

	const population_type *get_population_class_type(const population_class *population_class) const
	{
		const auto find_iterator = this->population_class_types.find(population_class);
		if (find_iterator != this->population_class_types.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_population_class_type(const population_class *population_class, const population_type *population_type)
	{
		if (population_type == nullptr) {
			this->population_class_types.erase(population_class);
			return;
		}

		this->population_class_types[population_class] = population_type;
	}

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
	phenotype *default_phenotype = nullptr;
	population_class_map<const population_type *> population_class_types;
	civilian_unit_class_map<const civilian_unit_type *> civilian_class_unit_types;
};

}
