#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class icon;

class commodity final : public named_data_entry, public data_type<commodity>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "commodity";
	static constexpr const char property_class_identifier[] = "metternich::commodity*";
	static constexpr const char database_folder[] = "commodities";

	explicit commodity(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

signals:
	void changed();

private:
	metternich::icon *icon = nullptr;
};

}
