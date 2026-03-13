#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

namespace metternich {

class music final : public data_entry, public data_type<music>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path filepath MEMBER filepath WRITE set_filepath)

public:
	static constexpr const char class_identifier[] = "music";
	static constexpr const char property_class_identifier[] = "metternich::music*";
	static constexpr const char database_folder[] = "music";

	explicit music(const std::string &identifier);
	~music();

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
