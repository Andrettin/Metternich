#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

namespace metternich {

class technological_period final : public data_entry, public data_type<technological_period>
{
	Q_OBJECT

	Q_PROPERTY(int start_year MEMBER start_year READ get_start_year NOTIFY changed)
	Q_PROPERTY(int end_year MEMBER end_year READ get_end_year NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "technological_period";
	static constexpr const char property_class_identifier[] = "metternich::technological_period*";
	static constexpr const char database_folder[] = "technological_periods";

	static void initialize_all();

	static const technological_period *get_by_year(const int year)
	{
		auto find_iterator = technological_period::periods_by_year.upper_bound(year);
		if (find_iterator != technological_period::periods_by_year.begin() && find_iterator != technological_period::periods_by_year.end()) {
			--find_iterator;
			return find_iterator->second;
		}

		throw std::runtime_error(std::format("No technological period found for year {}.", year));
	}

private:
	static inline std::map<int, const technological_period *> periods_by_year;

public:
	explicit technological_period(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	int get_index() const
	{
		return this->index;
	}

	int get_start_year() const
	{
		return this->start_year;
	}

	int get_end_year() const
	{
		return this->end_year;
	}

signals:
	void changed();

private:
	int index = -1;
	int start_year = 0;
	int end_year = 0;
};

}
