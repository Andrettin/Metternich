#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("map/terrain_type.h")

namespace metternich {

class terrain_type;

class terrain_feature final : public named_data_entry, public data_type<terrain_feature>
{
	Q_OBJECT

	Q_PROPERTY(metternich::terrain_type* terrain_type MEMBER terrain_type)
	Q_PROPERTY(bool river MEMBER river READ is_river)
	Q_PROPERTY(bool border_river MEMBER border_river READ is_border_river)
	Q_PROPERTY(QColor color MEMBER color READ get_color)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden)

public:
	static constexpr const char class_identifier[] = "terrain_feature";
	static constexpr const char property_class_identifier[] = "metternich::terrain_feature*";
	static constexpr const char database_folder[] = "terrain_features";

public:
	explicit terrain_feature(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const metternich::terrain_type *get_terrain_type() const
	{
		return this->terrain_type;
	}

	bool is_river() const
	{
		return this->river;
	}

	bool is_border_river() const
	{
		return this->border_river;
	}

	const QColor &get_color() const
	{
		return this->color;
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

private:
	metternich::terrain_type *terrain_type = nullptr;
	bool river = false;
	bool border_river = false;
	QColor color;
	bool hidden = false;
};

}
