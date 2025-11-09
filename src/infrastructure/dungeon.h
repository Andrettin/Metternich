#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("map/terrain_type.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class icon;
class portrait;
class site;
class terrain_type;

template <typename scope_type>
class and_condition;

class dungeon final : public named_data_entry, public data_type<dungeon>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)
	Q_PROPERTY(bool random MEMBER random READ is_random NOTIFY changed)
	Q_PROPERTY(int max_areas MEMBER max_areas READ get_max_areas NOTIFY changed)
	Q_PROPERTY(const metternich::terrain_type* terrain MEMBER terrain READ get_terrain NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "dungeon";
	static constexpr const char property_class_identifier[] = "metternich::dungeon*";
	static constexpr const char database_folder[] = "dungeons";

	explicit dungeon(const std::string &identifier);
	~dungeon();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	int get_level() const
	{
		return this->level;
	}

	bool is_random() const
	{
		return this->random;
	}

	int get_max_areas() const
	{
		return this->max_areas;
	}

	const terrain_type *get_terrain() const
	{
		return this->terrain;
	}

	const and_condition<site> *get_conditions() const
	{
		return this->conditions.get();
	}

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	const metternich::portrait *portrait = nullptr;
	int level = 0; //expected level of characters to take on this dungeon
	bool random = false; //whether this is a random dungeon
	int max_areas = 0; //how many areas the dungeon may have at maximum
	const terrain_type *terrain = nullptr;
	std::unique_ptr<const and_condition<site>> conditions;
};

}
