#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class character;
class character_attribute;
class domain;
class icon;
class military_unit;
class office;
class trait_type;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class trait final : public named_data_entry, public data_type<trait>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(const metternich::character_attribute* attribute MEMBER attribute READ get_attribute NOTIFY changed)
	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)
	Q_PROPERTY(bool hidden_name MEMBER hidden_name READ has_hidden_name NOTIFY changed)
	Q_PROPERTY(int max_scaling MEMBER max_scaling READ get_max_scaling NOTIFY changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string CONSTANT)
	Q_PROPERTY(QString military_unit_modifier_string READ get_military_unit_modifier_string CONSTANT)

public:
	static constexpr const char class_identifier[] = "trait";
	static constexpr const char property_class_identifier[] = "metternich::trait*";
	static constexpr const char database_folder[] = "traits";

	explicit trait(const std::string &identifier);
	~trait();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const std::vector<const trait_type *> &get_types() const
	{
		return this->types;
	}

	const character_attribute *get_attribute() const
	{
		return this->attribute;
	}

	int get_level() const
	{
		return this->level;
	}

	bool has_hidden_name() const
	{
		return this->hidden_name;
	}

	int get_max_scaling() const
	{
		return this->max_scaling;
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
	const metternich::icon *icon = nullptr;
	std::vector<const trait_type *> types;
	const character_attribute *attribute = nullptr;
	int level = 1;
	bool hidden_name = false;
	int max_scaling = std::numeric_limits<int>::max();
	data_entry_map<character_attribute, int> attribute_bonuses;
	std::unique_ptr<const and_condition<character>> conditions;
	std::unique_ptr<const and_condition<character>> generation_conditions;
	std::unique_ptr<const metternich::modifier<const character>> modifier;
	data_entry_map<office, std::unique_ptr<const metternich::modifier<const domain>>> office_modifiers;
	data_entry_map<office, std::unique_ptr<const metternich::modifier<const domain>>> scaled_office_modifiers;
	std::unique_ptr<const metternich::modifier<military_unit>> military_unit_modifier;
};

}
