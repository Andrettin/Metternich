#pragma once

#include "infrastructure/building_slot.h"
#include "util/fractional_int.h"

namespace metternich {

class education_type;

class country_building_slot final : public building_slot
{
	Q_OBJECT

	Q_PROPERTY(QVariantList available_education_types READ get_available_education_types_qvariant_list NOTIFY available_education_types_changed)
	Q_PROPERTY(QString country_modifier_string READ get_country_modifier_string NOTIFY country_modifier_changed)

public:
	explicit country_building_slot(const building_slot_type *type, const metternich::country *country);

	void set_building(const building_type *building);

	virtual bool can_have_building(const building_type *building) const override;
	virtual bool can_build_building(const building_type *building) const override;

	virtual const metternich::country *get_country() const override
	{
		return this->country;
	}

	std::vector<const education_type *> get_available_education_types() const;
	QVariantList get_available_education_types_qvariant_list() const;

	Q_INVOKABLE int get_education_type_employed_capacity(const metternich::education_type *education_type) const
	{
		const auto find_iterator = this->education_type_employed_capacities.find(education_type);
		if (find_iterator != this->education_type_employed_capacities.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	commodity_map<int> get_education_type_inputs(const education_type *education_type) const;
	Q_INVOKABLE QVariantList get_education_type_inputs(metternich::education_type *education_type) const;
	Q_INVOKABLE int get_education_type_input_wealth(const metternich::education_type *education_type) const;
	Q_INVOKABLE int get_education_type_output(const metternich::education_type *education_type) const;

	void change_education(const education_type *education_type, const int change, const bool change_input_storage = true);
	Q_INVOKABLE bool can_increase_education(const metternich::education_type *education_type) const;
	Q_INVOKABLE void increase_education(const metternich::education_type *education_type);
	Q_INVOKABLE bool can_decrease_education(const metternich::education_type *education_type) const;
	Q_INVOKABLE void decrease_education(const metternich::education_type *education_type, const bool restore_inputs);

	QString get_country_modifier_string() const;

signals:
	void available_education_types_changed();
	void country_modifier_changed();

private:
	const metternich::country *country = nullptr;
	std::map<const education_type *, int> education_type_employed_capacities;
};

}
