#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class character;
class country;
class icon;
class military_unit;
class province;
enum class character_attribute;
enum class trait_type;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class trait final : public named_data_entry, public data_type<trait>
{
	Q_OBJECT

	Q_PROPERTY(metternich::trait_type type MEMBER type NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)
	Q_PROPERTY(metternich::character_attribute attribute MEMBER attribute READ get_attribute NOTIFY changed)
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

	trait_type get_type() const
	{
		return this->type;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	int get_level() const
	{
		return this->level;
	}

	character_attribute get_attribute() const
	{
		return this->attribute;
	}

	int get_max_scaling() const
	{
		return this->max_scaling;
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

	const condition<character> *get_conditions() const
	{
		return this->conditions.get();
	}

	const condition<character> *get_generation_conditions() const
	{
		return this->generation_conditions.get();
	}

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

	QString get_modifier_string() const;

	const metternich::modifier<const country> *get_ruler_modifier() const
	{
		return this->ruler_modifier.get();
	}

	const metternich::modifier<const country> *get_scaled_ruler_modifier() const
	{
		return this->scaled_ruler_modifier.get();
	}

	const metternich::modifier<const country> *get_advisor_modifier() const
	{
		return this->advisor_modifier.get();
	}

	const metternich::modifier<const country> *get_scaled_advisor_modifier() const
	{
		return this->scaled_advisor_modifier.get();
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
	trait_type type;
	metternich::icon *icon = nullptr;
	int level = 1;
	character_attribute attribute{};
	int max_scaling = std::numeric_limits<int>::max();
	std::map<character_attribute, int> attribute_bonuses;
	std::unique_ptr<const condition<character>> conditions;
	std::unique_ptr<const condition<character>> generation_conditions;
	std::unique_ptr<const metternich::modifier<const character>> modifier;
	std::unique_ptr<const metternich::modifier<const country>> ruler_modifier;
	std::unique_ptr<const metternich::modifier<const country>> scaled_ruler_modifier;
	std::unique_ptr<const metternich::modifier<const country>> advisor_modifier;
	std::unique_ptr<const metternich::modifier<const country>> scaled_advisor_modifier;
	std::unique_ptr<const metternich::modifier<const province>> governor_modifier;
	std::unique_ptr<const metternich::modifier<const province>> scaled_governor_modifier;
	std::unique_ptr<const metternich::modifier<const character>> leader_modifier;
	std::unique_ptr<const metternich::modifier<const character>> scaled_leader_modifier;
	std::unique_ptr<const metternich::modifier<military_unit>> military_unit_modifier;
};

}
