#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class route final : public named_data_entry, public data_type<route>
{
	Q_OBJECT

	Q_PROPERTY(QColor color MEMBER color READ get_color)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden)

public:
	static constexpr const char class_identifier[] = "route";
	static constexpr const char property_class_identifier[] = "metternich::route*";
	static constexpr const char database_folder[] = "routes";

public:
	explicit route(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const QColor &get_color() const
	{
		return this->color;
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

private:
	QColor color;
	bool hidden = false;
};

}
