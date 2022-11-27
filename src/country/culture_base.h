#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "infrastructure/building_class_container.h"
#include "population/population_class_container.h"
#include "unit/civilian_unit_class_container.h"

namespace metternich {

class building_type;
class civilian_unit_type;
class cultural_group;
class phenotype;
class population_type;

class culture_base : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::cultural_group* group MEMBER group NOTIFY changed)
	Q_PROPERTY(metternich::phenotype* default_phenotype MEMBER default_phenotype)

public:
	explicit culture_base(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

protected:
	const cultural_group *get_group() const
	{
		return this->group;
	}

public:
	const phenotype *get_default_phenotype() const;

	const building_type *get_building_class_type(const building_class *building_class) const;

	void set_building_class_type(const building_class *building_class, const building_type *building_type)
	{
		if (building_type == nullptr) {
			this->building_class_types.erase(building_class);
			return;
		}

		this->building_class_types[building_class] = building_type;
	}

	const population_type *get_population_class_type(const population_class *population_class) const;

	void set_population_class_type(const population_class *population_class, const population_type *population_type)
	{
		if (population_type == nullptr) {
			this->population_class_types.erase(population_class);
			return;
		}

		this->population_class_types[population_class] = population_type;
	}

	const civilian_unit_type *get_civilian_class_unit_type(const civilian_unit_class *unit_class) const;

	void set_civilian_class_unit_type(const civilian_unit_class *unit_class, const civilian_unit_type *unit_type)
	{
		if (unit_type == nullptr) {
			this->civilian_class_unit_types.erase(unit_class);
			return;
		}

		this->civilian_class_unit_types[unit_class] = unit_type;
	}

signals:
	void changed();

private:
	cultural_group *group = nullptr;
	phenotype *default_phenotype = nullptr;
	building_class_map<const building_type *> building_class_types;
	population_class_map<const population_type *> population_class_types;
	civilian_unit_class_map<const civilian_unit_type *> civilian_class_unit_types;
};

}
