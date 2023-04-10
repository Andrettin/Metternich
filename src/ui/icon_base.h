#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

namespace metternich {

class icon_base : public data_entry
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path filepath MEMBER filepath WRITE set_filepath)

public:
	explicit icon_base(const std::string &identifier) : data_entry(identifier)
	{
	}

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
