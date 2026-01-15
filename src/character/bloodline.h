#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("character/character.h")

namespace metternich {

class character;

class bloodline final : public named_data_entry, public data_type<bloodline>
{
	Q_OBJECT

	Q_PROPERTY(metternich::character* founder MEMBER founder WRITE set_founder NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "bloodline";
	static constexpr const char property_class_identifier[] = "metternich::bloodline*";
	static constexpr const char database_folder[] = "bloodlines";

	static constexpr int max_bloodline_strength = 100;

	explicit bloodline(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const character *get_founder() const
	{
		return this->founder;
	}

	void set_founder(character *founder);

signals:
	void changed();

private:
	character *founder = nullptr;
};

}
