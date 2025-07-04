#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "trait_base.h"

namespace metternich {

class character;
class country;
class military_unit;
class office;
class province;
enum class character_attribute;
enum class character_trait_type;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class character_trait final : public trait_base, public data_type<character_trait>
{
	Q_OBJECT

	Q_PROPERTY(metternich::character_attribute attribute MEMBER attribute READ get_attribute NOTIFY changed)
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

	const std::set<character_trait_type> &get_types() const
	{
		return this->types;
	}

	character_attribute get_attribute() const
	{
		return this->attribute;
	}

	const std::map<character_attribute, int> &get_attribute_bonuses() const
	{
		return this->attribute_bonuses;
	}

	int get_attribute_bonus(const character_attribute attribute) const
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

	const metternich::modifier<const country> *get_office_modifier(const office *office) const
	{
		const auto find_iterator = this->office_modifiers.find(office);
		if (find_iterator != this->office_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	const metternich::modifier<const country> *get_scaled_office_modifier(const office *office) const
	{
		const auto find_iterator = this->scaled_office_modifiers.find(office);
		if (find_iterator != this->scaled_office_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	const metternich::modifier<const country> *get_advisor_modifier() const
	{
		return this->advisor_modifier.get();
	}

	const metternich::modifier<const country> *get_scaled_advisor_modifier() const
	{
		return this->scaled_advisor_modifier.get();
	}

	const effect_list<const country> *get_advisor_effects() const
	{
		return this->advisor_effects.get();
	}

	const metternich::modifier<const province> *get_governor_modifier() const
	{
		return this->governor_modifier.get();
	}

	const metternich::modifier<const province> *get_scaled_governor_modifier() const
	{
		return this->scaled_governor_modifier.get();
	}

	const metternich::modifier<const character> *get_leader_modifier() const
	{
		return this->leader_modifier.get();
	}

	const metternich::modifier<const character> *get_scaled_leader_modifier() const
	{
		return this->scaled_leader_modifier.get();
	}

	const metternich::modifier<military_unit> *get_military_unit_modifier() const
	{
		return this->military_unit_modifier.get();
	}

	QString get_military_unit_modifier_string() const;

signals:
	void changed();

private:
	std::set<character_trait_type> types;
	character_attribute attribute{};
	std::map<character_attribute, int> attribute_bonuses;
	std::unique_ptr<const and_condition<character>> conditions;
	std::unique_ptr<const and_condition<character>> generation_conditions;
	std::unique_ptr<const metternich::modifier<const character>> modifier;
	data_entry_map<office, std::unique_ptr<const metternich::modifier<const country>>> office_modifiers;
	data_entry_map<office, std::unique_ptr<const metternich::modifier<const country>>> scaled_office_modifiers;
	std::unique_ptr<const metternich::modifier<const country>> advisor_modifier;
	std::unique_ptr<const metternich::modifier<const country>> scaled_advisor_modifier;
	std::unique_ptr<const effect_list<const country>> advisor_effects;
	std::unique_ptr<const metternich::modifier<const province>> governor_modifier;
	std::unique_ptr<const metternich::modifier<const province>> scaled_governor_modifier;
	std::unique_ptr<const metternich::modifier<const character>> leader_modifier;
	std::unique_ptr<const metternich::modifier<const character>> scaled_leader_modifier;
	std::unique_ptr<const metternich::modifier<military_unit>> military_unit_modifier;
};

}
