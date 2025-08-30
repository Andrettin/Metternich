#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("unit/civilian_unit_class.h")

namespace metternich {

class civilian_unit_class;
class country;
class technology;
enum class advisor_category;
enum class character_attribute;
enum class military_unit_category;
enum class starting_age_category;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class character_class final : public named_data_entry, public data_type<character_class>
{
	Q_OBJECT

	Q_PROPERTY(metternich::advisor_category advisor_category MEMBER advisor_category NOTIFY changed)
	Q_PROPERTY(metternich::character_attribute attribute MEMBER attribute NOTIFY changed)
	Q_PROPERTY(metternich::military_unit_category military_unit_category MEMBER military_unit_category READ get_military_unit_category NOTIFY changed)
	Q_PROPERTY(const metternich::civilian_unit_class* civilian_unit_class MEMBER civilian_unit_class READ get_civilian_unit_class NOTIFY changed)
	Q_PROPERTY(metternich::starting_age_category starting_age_category MEMBER starting_age_category READ get_starting_age_category NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* obsolescence_technology MEMBER obsolescence_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "character_class";
	static constexpr const char property_class_identifier[] = "metternich::character_class*";
	static constexpr const char database_folder[] = "character_classes";

	explicit character_class(const std::string &identifier);
	~character_class();

	virtual void check() const override;

	metternich::advisor_category get_advisor_category() const
	{
		return this->advisor_category;
	}

	character_attribute get_attribute() const
	{
		return this->attribute;
	}

	metternich::military_unit_category get_military_unit_category() const
	{
		return this->military_unit_category;
	}

	const metternich::civilian_unit_class *get_civilian_unit_class() const
	{
		return this->civilian_unit_class;
	}

	metternich::starting_age_category get_starting_age_category() const
	{
		return this->starting_age_category;
	}

	technology *get_required_technology() const
	{
		return this->required_technology;
	}

	technology *get_obsolescence_technology() const
	{
		return this->obsolescence_technology;
	}

signals:
	void changed();

private:
	metternich::advisor_category advisor_category;
	character_attribute attribute;
	metternich::military_unit_category military_unit_category;
	const metternich::civilian_unit_class *civilian_unit_class = nullptr;
	metternich::starting_age_category starting_age_category{};
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
};

}
