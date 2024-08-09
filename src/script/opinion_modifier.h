#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class opinion_modifier final : public named_data_entry, public data_type<opinion_modifier>
{
	Q_OBJECT

	Q_PROPERTY(int value MEMBER value READ get_value)
	Q_PROPERTY(int duration MEMBER duration READ get_duration)
	Q_PROPERTY(int duration_days MEMBER duration_days READ get_duration_days)

public:
	static constexpr const char class_identifier[] = "opinion_modifier";
	static constexpr const char property_class_identifier[] = "metternich::opinion_modifier*";
	static constexpr const char database_folder[] = "opinion_modifiers";

	explicit opinion_modifier(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	void process_gsml_property(const gsml_property &property);

	int get_value() const
	{
		return this->value;
	}

	int get_duration() const
	{
		return this->duration;
	}

	int get_duration_days() const
	{
		return this->duration_days;
	}

private:
	int value = 0;
	int duration = 0;
	int duration_days = 0;
};

}
