#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class era final : public named_data_entry, public data_type<era>
{
	Q_OBJECT

	Q_PROPERTY(QDate start_date MEMBER start_date READ get_start_date)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "era";
	static constexpr const char property_class_identifier[] = "metternich::era*";
	static constexpr const char database_folder[] = "eras";

	static void initialize_all();

	explicit era(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const QDate &get_start_date() const
	{
		return this->start_date;
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

signals:
	void changed();

private:
	QDate start_date;
	bool hidden = false;
};

}
