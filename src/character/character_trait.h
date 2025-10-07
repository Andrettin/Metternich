#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "trait_base.h"

namespace metternich {

class character;
class character_attribute;
class domain;
class military_unit;
class office;
class province;
class trait_type;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class character_trait final : public trait_base, public data_type<character_trait>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::character_attribute* attribute MEMBER attribute READ get_attribute NOTIFY changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string CONSTANT)
	Q_PROPERTY(QString military_unit_modifier_string READ get_military_unit_modifier_string CONSTANT)

public:
	static constexpr const char class_identifier[] = "character_trait";
	static constexpr const char property_class_identifier[] = "metternich::character_trait*";
	static constexpr const char database_folder[] = "traits/character";

	explicit character_trait(const std::string &identifier);
	~character_trait();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const std::vector<const trait_type *> &get_types() const
	{
		return this->types;
	}

	const character_attribute *get_attribute() const
	{
		return this->attribute;
	}

	const data_entry_map<character_attribute, int> &get_attribute_bonuses() const
	{
		return this->attribute_bonuses;
	}

	int get_attribute_bonus(const character_attribute *attribute) const
	{
		const auto find_iterator = this->get_attribute_bonuses().find(attribute);
		if (find_iterator != this->get_attribute_bonuses().end()) {
			return find_iterator->second;
		}

		return 0;
	}

	const and_condition<character> *get_conditions() const
	{
		return this->conditions.get();
	}

	const and_condition<character> *get_generation_conditions() const
	{
		return this->generation_conditions.get();
	}

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

	QString get_modifier_string() const;

	const metternich::modifier<const domain> *get_office_modifier(const office *office) const
	{
		const auto find_iterator = this->office_modifiers.find(office);
		if (find_iterator != this->office_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	const metternich::modifier<const domain> *get_scaled_office_modifier(const office *office) const
	{
		const auto find_iterator = this->scaled_office_modifiers.find(office);
		if (find_iterator != this->scaled_office_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	const metternich::modifier<military_unit> *get_military_unit_modifier() const
	{
		return this->military_unit_modifier.get();
	}

	QString get_military_unit_modifier_string() const;

signals:
	void changed();

private:
	std::vector<const trait_type *> types;
	const character_attribute *attribute = nullptr;
	data_entry_map<character_attribute, int> attribute_bonuses;
	std::unique_ptr<const and_condition<character>> conditions;
	std::unique_ptr<const and_condition<character>> generation_conditions;
	std::unique_ptr<const metternich::modifier<const character>> modifier;
	data_entry_map<office, std::unique_ptr<const metternich::modifier<const domain>>> office_modifiers;
	data_entry_map<office, std::unique_ptr<const metternich::modifier<const domain>>> scaled_office_modifiers;
	std::unique_ptr<const metternich::modifier<military_unit>> military_unit_modifier;
};

}
