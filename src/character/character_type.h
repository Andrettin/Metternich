#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class icon;

class character_type final : public named_data_entry, public data_type<character_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* portrait MEMBER portrait NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "character_type";
	static constexpr const char property_class_identifier[] = "metternich::character_type*";
	static constexpr const char database_folder[] = "character_types";

	explicit character_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const icon *get_portrait() const
	{
		return this->portrait;
	}

signals:
	void changed();

private:
	icon *portrait = nullptr;
};

}
