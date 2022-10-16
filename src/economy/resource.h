#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class commodity;

//resources are present on tiles, allowing the tile to produce a given commodity
//multiple resources can produce the same commodity
class resource final : public named_data_entry, public data_type<resource>
{
	Q_OBJECT

	Q_PROPERTY(metternich::commodity* commodity MEMBER commodity)
	Q_PROPERTY(std::filesystem::path icon_filepath MEMBER icon_filepath WRITE set_icon_filepath)

public:
	static constexpr const char class_identifier[] = "resource";
	static constexpr const char property_class_identifier[] = "metternich::resource*";
	static constexpr const char database_folder[] = "resources";

	explicit resource(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const metternich::commodity *get_commodity() const
	{
		return this->commodity;
	}

	const std::filesystem::path &get_icon_filepath() const
	{
		return this->icon_filepath;
	}

	void set_icon_filepath(const std::filesystem::path &filepath);

private:
	metternich::commodity *commodity = nullptr;
	std::filesystem::path icon_filepath;
};

}
