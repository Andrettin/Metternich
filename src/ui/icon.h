#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

namespace metternich {

class icon final : public data_entry, public data_type<icon>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path filepath MEMBER filepath WRITE set_filepath)

public:
	static constexpr const char class_identifier[] = "icon";
	static constexpr const char property_class_identifier[] = "metternich::icon*";
	static constexpr const char database_folder[] = "icons";

	explicit icon(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const std::filesystem::path &get_filepath() const
	{
		return this->filepath;
	}

	void set_filepath(const std::filesystem::path &filepath);

private:
	std::filesystem::path filepath;
};

}
