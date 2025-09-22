#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("character/character_class.h")
Q_MOC_INCLUDE("species/species.h")

namespace metternich {

class character_class;
class species;

class monster_type final : public named_data_entry, public data_type<monster_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::species* species MEMBER species READ get_species NOTIFY changed)
	Q_PROPERTY(const metternich::character_class* character_class MEMBER character_class READ get_character_class NOTIFY changed)
	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)
	Q_PROPERTY(int64_t experience_award MEMBER experience_award READ get_experience_award NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "monster_type";
	static constexpr const char property_class_identifier[] = "metternich::monster_type*";
	static constexpr const char database_folder[] = "monster_types";

	explicit monster_type(const std::string &identifier);
	~monster_type();

	virtual void check() const override;

	const metternich::species *get_species() const
	{
		return this->species;
	}

	const character_class *get_character_class() const
	{
		return this->character_class;
	}

	int get_level() const
	{
		return this->level;
	}

	int64_t get_experience_award() const
	{
		return this->experience_award;
	}

signals:
	void changed();

private:
	const metternich::species *species = nullptr;
	const metternich::character_class *character_class = nullptr;
	int level = 0;
	int64_t experience_award = 0; //the experience award for defeating the monster
};

}
