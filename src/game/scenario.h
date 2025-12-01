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

class domain;
class map_template;

class scenario final : public named_data_entry, public data_type<scenario>
{
	Q_OBJECT

	Q_PROPERTY(metternich::scenario* parent_scenario MEMBER parent_scenario NOTIFY changed)
	Q_PROPERTY(QDate start_date MEMBER start_date READ get_start_date NOTIFY changed)
	Q_PROPERTY(int start_year READ get_start_year NOTIFY changed)
	Q_PROPERTY(const archimedes::calendar* start_date_calendar MEMBER start_date_calendar READ get_start_date_calendar)
	Q_PROPERTY(const archimedes::timeline* timeline MEMBER timeline READ get_timeline NOTIFY changed)
	Q_PROPERTY(const metternich::map_template* map_template MEMBER map_template READ get_map_template NOTIFY changed)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden NOTIFY changed)
	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(QVariantList default_countries READ get_default_countries_qvariant_list NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "scenario";
	static constexpr const char property_class_identifier[] = "metternich::scenario*";
	static constexpr const char database_folder[] = "scenarios";

	static void initialize_all();

	static const std::vector<const scenario *> &get_top_level_scenarios()
	{
		return scenario::top_level_scenarios;
	}

private:
	static inline std::vector<const scenario *> top_level_scenarios;

public:
	explicit scenario(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const scenario *get_parent_scenario() const
	{
		return this->parent_scenario;
	}

	const std::vector<const scenario *> &get_child_scenarios() const
	{
		return this->child_scenarios;
	}

	const QDate &get_start_date() const
	{
		return this->start_date;
	}

	int get_start_year() const
	{
		return this->get_start_date().year();
	}

	const calendar *get_start_date_calendar() const
	{
		return this->start_date_calendar;
	}

	const archimedes::timeline *get_timeline() const
	{
		return this->timeline;
	}

	const metternich::map_template *get_map_template() const
	{
		return this->map_template;
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
	scenario *parent_scenario = nullptr;
	std::vector<const scenario *> child_scenarios;
	QDate start_date;
	const calendar *start_date_calendar = nullptr; //the calendar for the start date
	const archimedes::timeline *timeline = nullptr; //the timeline in which the scenario is set
	const metternich::map_template *map_template = nullptr;
	bool hidden = false;
	std::string description;
	std::vector<const domain *> default_countries;
};

}
