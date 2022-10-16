#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class technology final : public named_data_entry, public data_type<technology>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path icon_filepath MEMBER icon_filepath WRITE set_icon_filepath)

public:
	static constexpr const char class_identifier[] = "technology";
	static constexpr const char property_class_identifier[] = "metternich::technology*";
	static constexpr const char database_folder[] = "technologies";

	explicit technology(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const std::filesystem::path &get_icon_filepath() const
	{
		return this->icon_filepath;
	}

	void set_icon_filepath(const std::filesystem::path &filepath);

private:
	std::filesystem::path icon_filepath;
};

}
