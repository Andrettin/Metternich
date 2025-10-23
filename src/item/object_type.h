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

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
};

}
