#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace archimedes {
	class calendar;
}

namespace metternich {

class map_template;

class scenario final : public named_data_entry, public data_type<scenario>
{
	Q_OBJECT

	Q_PROPERTY(QDateTime start_date MEMBER start_date READ get_start_date NOTIFY changed)
	Q_PROPERTY(archimedes::calendar* start_date_calendar MEMBER start_date_calendar)
	Q_PROPERTY(archimedes::timeline* timeline MEMBER timeline NOTIFY changed)
	Q_PROPERTY(metternich::map_template* map_template MEMBER map_template NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "scenario";
	static constexpr const char property_class_identifier[] = "metternich::scenario*";
	static constexpr const char database_folder[] = "scenarios";

	static void initialize_all();

	explicit scenario(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;

	const QDateTime &get_start_date() const
	{
		return this->start_date;
	}

	const archimedes::timeline *get_timeline() const
	{
		return this->timeline;
	}

	const metternich::map_template *get_map_template() const
	{
		return this->map_template;
	}

signals:
	void changed();

private:
	QDateTime start_date;
	calendar *start_date_calendar = nullptr; //the calendar for the start date
	archimedes::timeline *timeline = nullptr; //the timeline in which the scenario is set
	metternich::map_template *map_template = nullptr;
};

}
