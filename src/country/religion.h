#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class religion final : public named_data_entry, public data_type<religion>
{
	Q_OBJECT

	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "religion";
	static constexpr const char property_class_identifier[] = "metternich::religion*";
	static constexpr const char database_folder[] = "religions";

	explicit religion(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const QColor &get_color() const
	{
		return this->color;
	}

signals:
	void changed();

private:
	QColor color;
};

}