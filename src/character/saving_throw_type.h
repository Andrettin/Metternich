#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class saving_throw_type final : public named_data_entry, public data_type<saving_throw_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::saving_throw_type *base_saving_throw_type MEMBER base_saving_throw_type NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "saving_throw_type";
	static constexpr const char property_class_identifier[] = "metternich::saving_throw_type*";
	static constexpr const char database_folder[] = "saving_throw_types";

	explicit saving_throw_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const saving_throw_type *get_base_saving_throw_type() const
	{
		return this->base_saving_throw_type;
	}

	const std::vector<const saving_throw_type *> &get_derived_saving_throw_types() const
	{
		return this->derived_saving_throw_types;
	}

signals:
	void changed();

private:
	saving_throw_type *base_saving_throw_type = nullptr;
	std::vector<const saving_throw_type *> derived_saving_throw_types;
};

}
