#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class commodity final : public named_data_entry, public data_type<commodity>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path icon_filepath MEMBER icon_filepath WRITE set_icon_filepath)

public:
	static constexpr const char class_identifier[] = "commodity";
	static constexpr const char property_class_identifier[] = "metternich::commodity*";
	static constexpr const char database_folder[] = "commodities";

	explicit commodity(const std::string &identifier) : named_data_entry(identifier)
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
