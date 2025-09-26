#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character_attribute;

class saving_throw_type final : public named_data_entry, public data_type<saving_throw_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::character_attribute* attribute MEMBER attribute READ get_attribute NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "saving_throw_type";
	static constexpr const char property_class_identifier[] = "metternich::saving_throw_type*";
	static constexpr const char database_folder[] = "saving_throw_types";

	explicit saving_throw_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const character_attribute *get_attribute() const
	{
		return this->attribute;
	}

signals:
	void changed();

private:
	const character_attribute *attribute = nullptr;
};

}
