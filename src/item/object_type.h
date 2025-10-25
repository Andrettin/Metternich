#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;

class object_type final : public named_data_entry, public data_type<object_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(QString usage_adjective READ get_usage_adjective_qstring NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "object_type";
	static constexpr const char property_class_identifier[] = "metternich::object_type*";
	static constexpr const char database_folder[] = "object_types";

	explicit object_type(const std::string &identifier);
	~object_type();

	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const std::string &get_usage_adjective() const
	{
		return this->usage_adjective;
	}

	Q_INVOKABLE void set_usage_adjective(const std::string &adjective)
	{
		this->usage_adjective = adjective;
	}

	QString get_usage_adjective_qstring() const
	{
		return QString::fromStdString(this->get_usage_adjective());
	}

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	std::string usage_adjective = "Used";
};

}
