#pragma once

#include "infrastructure/building_slot.h"
#include "util/fractional_int.h"

namespace metternich {

class education_type;
class production_type;

class country_building_slot final : public building_slot
{
	Q_OBJECT

	Q_PROPERTY(bool expanding READ is_expanding WRITE set_expanding NOTIFY expanding_changed)
	Q_PROPERTY(int capacity READ get_capacity NOTIFY capacity_changed)
	Q_PROPERTY(int employed_capacity READ get_employed_capacity NOTIFY employed_capacity_changed)
	Q_PROPERTY(QVariantList available_production_types READ get_available_production_types_qvariant_list NOTIFY available_production_types_changed)
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

	int get_level() const
	{
		return (this->get_building() ? 1 : 0) + this->get_expansion_count();
	}

	void set_level(const int level)
	{
		this->set_expansion_count(std::max(0, level - 1));
	}

	int get_expansion_count() const
	{
		return this->expansion_count;
	}

	void change_expansion_count(const int change);

	void set_expansion_count(const int expansion_count)
	{
		this->change_expansion_count(expansion_count - this->get_expansion_count());
	}

	bool is_expanding() const
	{
		return this->expanding;
	}

	void set_expanding(const bool expanding)
	{
		if (expanding == this->is_expanding()) {
			return;
		}

		this->expanding = expanding;
		emit expanding_changed();
	}

	Q_INVOKABLE bool can_expand() const;
	void expand();

	int get_capacity() const
	{
		return this->capacity;
	}

	void change_capacity(const int change);

	int get_employed_capacity() const
	{
		return this->employed_capacity;
	}

	std::vector<const production_type *> get_available_production_types() const;
	QVariantList get_available_production_types_qvariant_list() const;

	Q_INVOKABLE int get_production_type_employed_capacity(const metternich::production_type *production_type) const
	{
		const auto find_iterator = this->production_type_employed_capacities.find(production_type);
		if (find_iterator != this->production_type_employed_capacities.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	commodity_map<int> get_production_type_inputs(const production_type *production_type) const;
	Q_INVOKABLE QVariantList get_production_type_inputs(metternich::production_type *production_type) const;
	Q_INVOKABLE int get_production_type_input_wealth(const metternich::production_type *production_type) const;

	centesimal_int get_production_type_output(const production_type *production_type) const;

	Q_INVOKABLE int get_production_type_output(metternich::production_type *production_type) const
	{
		const metternich::production_type *const_production_type = production_type;
		return this->get_production_type_output(const_production_type).to_int();
	}

	void change_production(const production_type *production_type, const int change, const bool change_input_storage = true);
	Q_INVOKABLE bool can_increase_production(const metternich::production_type *production_type) const;
	Q_INVOKABLE void increase_production(const metternich::production_type *production_type);
	Q_INVOKABLE bool can_decrease_production(const metternich::production_type *production_type) const;
	Q_INVOKABLE void decrease_production(const metternich::production_type *production_type, const bool restore_inputs);

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
	void expanding_changed();
	void capacity_changed();
	void employed_capacity_changed();
	void available_production_types_changed();
	void available_education_types_changed();
	void country_modifier_changed();

private:
	const metternich::country *country = nullptr;
	int expansion_count = 0;
	int capacity = 0;
	int employed_capacity = 0;
	std::map<const production_type *, int> production_type_employed_capacities;
	std::map<const education_type *, int> education_type_employed_capacities;
	bool expanding = false;
};

}
