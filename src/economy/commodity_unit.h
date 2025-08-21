#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class icon;

class commodity_unit final : public named_data_entry, public data_type<commodity_unit>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(QString suffix READ get_suffix_qstring NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "commodity_unit";
	static constexpr const char property_class_identifier[] = "metternich::commodity_unit*";
	static constexpr const char database_folder[] = "commodity_units";

	explicit commodity_unit(const std::string &identifier);

	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const std::string &get_suffix() const
	{
		return this->suffix;
	}

	QString get_suffix_qstring() const
	{
		return QString::fromStdString(this->get_suffix());
	}

	Q_INVOKABLE void set_suffix(const std::string &suffix)
	{
		this->suffix = suffix;
	}

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	std::string suffix;
};

}
