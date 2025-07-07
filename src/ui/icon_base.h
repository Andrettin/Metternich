#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "util/color_container.h"

namespace metternich {

class icon_base : public data_entry
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path filepath MEMBER filepath WRITE set_filepath)
	Q_PROPERTY(int hue_rotation MEMBER hue_rotation READ get_hue_rotation)

public:
	explicit icon_base(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const std::filesystem::path &get_filepath() const
	{
		return this->filepath;
	}

	void set_filepath(const std::filesystem::path &filepath);

	int get_hue_rotation() const
	{
		return this->hue_rotation;
	}

	const color_set &get_hue_ignored_colors() const
	{
		return this->hue_ignored_colors;
	}

private:
	std::filesystem::path filepath;
	int hue_rotation = 0;
	color_set hue_ignored_colors;
};

}
