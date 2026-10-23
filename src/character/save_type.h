#pragma once

#include "character/character_stat.h"
#include "database/data_type.h"

namespace metternich {

class save_type final : public character_stat, public data_type<save_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::save_type *base_save_type MEMBER base_save_type NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "save_type";
	static constexpr const char property_class_identifier[] = "metternich::save_type*";
	static constexpr const char database_folder[] = "save_types";

	explicit save_type(const std::string &identifier) : character_stat(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const save_type *get_base_save_type() const
	{
		return this->base_save_type;
	}

signals:
	void changed();

private:
	save_type *base_save_type = nullptr;
};

}
