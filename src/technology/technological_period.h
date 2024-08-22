#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

namespace metternich {

class technological_period final : public data_entry, public data_type<technological_period>
{
	Q_OBJECT

	Q_PROPERTY(metternich::technological_period* parent_period MEMBER parent_period NOTIFY changed)
	Q_PROPERTY(int start_year MEMBER start_year READ get_start_year NOTIFY changed)
	Q_PROPERTY(int end_year MEMBER end_year READ get_end_year NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "technological_period";
	static constexpr const char property_class_identifier[] = "metternich::technological_period*";
	static constexpr const char database_folder[] = "technological_periods";

public:
	explicit technological_period(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const std::vector<const technological_period *> &get_child_periods() const
	{
		return this->child_periods;
	}

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
	technological_period *parent_period = nullptr;
	std::vector<const technological_period *> child_periods;
	int index = -1;
	int start_year = 0;
	int end_year = 0;
};

}
