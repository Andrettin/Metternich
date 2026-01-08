#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/dice.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class character_attribute;
class icon;

class skill final : public named_data_entry, public data_type<skill>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(archimedes::dice check_dice MEMBER check_dice READ get_check_dice NOTIFY changed)
	Q_PROPERTY(metternich::character_attribute* base_attribute MEMBER base_attribute NOTIFY changed)
	Q_PROPERTY(int base_value MEMBER base_value READ get_base_value NOTIFY changed)
	Q_PROPERTY(bool find_traps MEMBER find_traps NOTIFY changed)
	Q_PROPERTY(bool disarm_traps MEMBER disarm_traps NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "skill";
	static constexpr const char property_class_identifier[] = "metternich::skill*";
	static constexpr const char database_folder[] = "skills";

	static const skill *get_find_traps_skill()
	{
		return skill::find_traps_skill;
	}

	static const skill *get_disarm_traps_skill()
	{
		return skill::disarm_traps_skill;
	}

private:
	static inline const skill *find_traps_skill = nullptr;
	static inline const skill *disarm_traps_skill = nullptr;

public:
	explicit skill(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const metternich::icon *get_icon() const;
	const dice &get_check_dice() const;

	const character_attribute *get_base_attribute() const
	{
		return this->base_attribute;
	}

	int get_base_value() const
	{
		return this->base_value;
	}

	std::string_view get_value_suffix() const;

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	dice check_dice;
	character_attribute *base_attribute = nullptr;
	int base_value = 0;
	bool find_traps = false;
	bool disarm_traps = false;
};

}
