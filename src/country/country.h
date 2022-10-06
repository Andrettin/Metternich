#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

enum class country_type;

class country final : public named_data_entry, public data_type<country>
{
	Q_OBJECT

	Q_PROPERTY(metternich::country_type type MEMBER type READ get_type)
	Q_PROPERTY(QColor color MEMBER color READ get_color)

public:
	static constexpr const char class_identifier[] = "country";
	static constexpr const char property_class_identifier[] = "metternich::country*";
	static constexpr const char database_folder[] = "countries";

	explicit country(const std::string &identifier);

	virtual void check() const override;

	country_type get_type() const
	{
		return this->type;
	}

	const QColor &get_color() const
	{
		return this->color;
	}

private:
	country_type type;
	QColor color;
};

}
