#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class icon;
enum class attribute;

class character_type final : public named_data_entry, public data_type<character_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::attribute primary_attribute MEMBER primary_attribute NOTIFY changed)
	Q_PROPERTY(int primary_attribute_index READ get_primary_attribute_index CONSTANT)

public:
	static constexpr const char class_identifier[] = "character_type";
	static constexpr const char property_class_identifier[] = "metternich::character_type*";
	static constexpr const char database_folder[] = "character_types";

	explicit character_type(const std::string &identifier);

	virtual void check() const override;

	const icon *get_portrait() const
	{
		return this->portrait;
	}

	attribute get_primary_attribute() const
	{
		return this->primary_attribute;
	}

	int get_primary_attribute_index() const
	{
		return static_cast<int>(this->get_primary_attribute());
	}

signals:
	void changed();

private:
	icon *portrait = nullptr;
	attribute primary_attribute;
};

}
