#pragma once

#include "database/data_type.h"
#include "ui/icon_base.h"
#include "util/centesimal_int.h"

namespace metternich {

class icon final : public icon_base, public data_type<icon>
{
	Q_OBJECT

	Q_PROPERTY(archimedes::centesimal_int scale_factor MEMBER scale_factor READ get_scale_factor NOTIFY changed)
	Q_PROPERTY(bool left_facing MEMBER left_facing READ is_left_facing NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "icon";
	static constexpr const char property_class_identifier[] = "metternich::icon*";
	static constexpr const char database_folder[] = "icons";

	explicit icon(const std::string &identifier);
	~icon();

	virtual void initialize() override;

	const centesimal_int &get_scale_factor() const
	{
		return this->scale_factor;
	}

	bool is_left_facing() const
	{
		return this->left_facing;
	}

private:
	centesimal_int scale_factor = centesimal_int(1);
	bool left_facing = false;
};

}
