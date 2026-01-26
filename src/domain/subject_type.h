#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class subject_type final : public named_data_entry, public data_type<subject_type>
{
	Q_OBJECT

	Q_PROPERTY(int wealth_tribute_rate MEMBER wealth_tribute_rate READ get_wealth_tribute_rate NOTIFY changed)
	Q_PROPERTY(int regency_tribute_rate MEMBER regency_tribute_rate READ get_regency_tribute_rate NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "subject_type";
	static constexpr const char property_class_identifier[] = "metternich::subject_type*";
	static constexpr const char database_folder[] = "subject_types";

	explicit subject_type(const std::string &identifier);
	~subject_type();

	int get_wealth_tribute_rate() const
	{
		return this->wealth_tribute_rate;
	}

	int get_regency_tribute_rate() const
	{
		return this->regency_tribute_rate;
	}

signals:
	void changed();

private:
	int wealth_tribute_rate = 0;
	int regency_tribute_rate = 0;
};

}
