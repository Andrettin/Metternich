#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("map/map_template.h")
Q_MOC_INCLUDE("time/calendar.h")
Q_MOC_INCLUDE("time/timeline.h")

namespace archimedes {
	class calendar;
}

namespace metternich {

class country;
class map_template;

class scenario final : public named_data_entry, public data_type<scenario>
{
	Q_OBJECT

	Q_PROPERTY(QDate start_date MEMBER start_date READ get_start_date NOTIFY changed)
	Q_PROPERTY(int start_year READ get_start_year NOTIFY changed)
	Q_PROPERTY(archimedes::calendar* start_date_calendar MEMBER start_date_calendar)
	Q_PROPERTY(archimedes::timeline* timeline MEMBER timeline NOTIFY changed)
	Q_PROPERTY(metternich::map_template* map_template MEMBER map_template NOTIFY changed)
	Q_PROPERTY(bool major MEMBER major READ is_major NOTIFY changed)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden NOTIFY changed)
	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(QVariantList default_countries READ get_default_countries_qvariant_list NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "scenario";
	static constexpr const char property_class_identifier[] = "metternich::scenario*";
	static constexpr const char database_folder[] = "scenarios";

	static void initialize_all();

	explicit scenario(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const QDate &get_start_date() const
	{
		return this->start_date;
	}

	int get_start_year() const
	{
		return this->get_start_date().year();
	}

	const archimedes::timeline *get_timeline() const
	{
		return this->timeline;
	}

	const metternich::map_template *get_map_template() const
	{
		return this->map_template;
	}

	bool is_major() const
	{
		return this->major;
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

	const std::string &get_description() const
	{
		return this->description;
	}

	Q_INVOKABLE void set_description(const std::string &description)
	{
		this->description = description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	QVariantList get_default_countries_qvariant_list() const;

signals:
	void changed();

private:
	QDate start_date;
	calendar *start_date_calendar = nullptr; //the calendar for the start date
	archimedes::timeline *timeline = nullptr; //the timeline in which the scenario is set
	metternich::map_template *map_template = nullptr;
	metternich::country *default_country = nullptr;
	bool major = false;
	bool hidden = false;
	std::string description;
	std::vector<const country *> default_countries;
};

}
