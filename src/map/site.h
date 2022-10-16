#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/geocoordinate.h"
#include "util/qunique_ptr.h"

namespace metternich {

class cultural_group;
class culture;
class resource;
class site_game_data;
class site_history;
class terrain_type;
class world;
enum class site_type;

class site final : public named_data_entry, public data_type<site>
{
	Q_OBJECT

	Q_PROPERTY(metternich::world* world MEMBER world)
	Q_PROPERTY(archimedes::geocoordinate geocoordinate MEMBER geocoordinate READ get_geocoordinate)
	Q_PROPERTY(metternich::site_type type MEMBER type READ get_type)
	Q_PROPERTY(metternich::terrain_type* terrain_type MEMBER terrain_type)
	Q_PROPERTY(metternich::resource* resource MEMBER resource)
	Q_PROPERTY(metternich::site_game_data* game_data READ get_game_data NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "site";
	static constexpr const char property_class_identifier[] = "metternich::site*";
	static constexpr const char database_folder[] = "sites";
	static constexpr bool history_enabled = true;

public:
	explicit site(const std::string &identifier);
	~site();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	site_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	void reset_game_data();

	site_game_data *get_game_data() const
	{
		return this->game_data.get();
	}

	const metternich::world *get_world() const
	{
		return this->world;
	}

	const archimedes::geocoordinate &get_geocoordinate() const
	{
		return this->geocoordinate;
	}

	site_type get_type() const
	{
		return this->type;
	}

	const metternich::terrain_type *get_terrain_type() const
	{
		return this->terrain_type;
	}

	const metternich::resource *get_resource() const
	{
		return this->resource;
	}

	const std::string &get_cultural_name(const culture *culture) const;

signals:
	void changed();

private:
	metternich::world *world = nullptr;
	archimedes::geocoordinate geocoordinate;
	site_type type;
	metternich::terrain_type *terrain_type = nullptr;
	metternich::resource *resource = nullptr;
	std::map<const culture *, std::string> cultural_names;
	std::map<const cultural_group *, std::string> cultural_group_names;
	qunique_ptr<site_history> history;
	qunique_ptr<site_game_data> game_data;
};

}
