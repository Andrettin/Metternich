#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class save_type final : public named_data_entry, public data_type<save_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::save_type *base_save_type MEMBER base_save_type NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "save_type";
	static constexpr const char property_class_identifier[] = "metternich::save_type*";
	static constexpr const char database_folder[] = "save_types";

	explicit save_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const save_type *get_base_save_type() const
	{
		return this->base_save_type;
	}

	const std::vector<const save_type *> &get_derived_save_types() const
	{
		return this->derived_save_types;
	}

signals:
	void changed();

private:
	save_type *base_save_type = nullptr;
	std::vector<const save_type *> derived_save_types;
};

}
