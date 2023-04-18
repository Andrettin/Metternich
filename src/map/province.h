#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/color_container.h"
#include "util/qunique_ptr.h"

namespace metternich {

class character;
class country;
class cultural_group;
class culture;
class province_game_data;
class province_history;
class region;
class site;

class province final : public named_data_entry, public data_type<province>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color NOTIFY changed)
	Q_PROPERTY(bool sea MEMBER sea READ is_sea NOTIFY changed)
	Q_PROPERTY(bool bay MEMBER bay READ is_bay NOTIFY changed)
	Q_PROPERTY(bool lake MEMBER lake READ is_lake NOTIFY changed)
	Q_PROPERTY(bool water_zone READ is_water_zone NOTIFY changed)
	Q_PROPERTY(metternich::site* capital_settlement MEMBER capital_settlement NOTIFY changed)
	Q_PROPERTY(std::vector<metternich::region *> regions READ get_regions NOTIFY changed)
	Q_PROPERTY(metternich::province_game_data* game_data READ get_game_data NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "province";
	static constexpr const char property_class_identifier[] = "metternich::province*";
	static constexpr const char database_folder[] = "provinces";
	static constexpr bool history_enabled = true;

	static constexpr int base_score = 100;

	static province *get_by_color(const QColor &color)
	{
		province *province = province::try_get_by_color(color);

		if (province == nullptr) {
			throw std::runtime_error("No province found for color: (" + std::to_string(color.red()) + ", " + std::to_string(color.green()) + ", " + std::to_string(color.blue()) + ").");
		}

		return province;
	}

	static province *try_get_by_color(const QColor &color)
	{
		const auto find_iterator = province::provinces_by_color.find(color);
		if (find_iterator != province::provinces_by_color.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static void clear()
	{
		data_type::clear();
		province::provinces_by_color.clear();
	}

private:
	static inline color_map<province *> provinces_by_color;

public:
	explicit province(const std::string &identifier);
	~province();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	province_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	void reset_game_data();

	province_game_data *get_game_data() const
	{
		return this->game_data.get();
	}

	const QColor &get_color() const
	{
		return this->color;
	}

	void set_color(const QColor &color)
	{
		if (color == this->get_color()) {
			return;
		}

		if (province::try_get_by_color(color) != nullptr) {
			throw std::runtime_error("Color is already used by another province.");
		}

		this->color = color;
		province::provinces_by_color[color] = this;
	}

	bool is_sea() const
	{
		return this->sea;
	}

	bool is_bay() const
	{
		return this->bay;
	}

	bool is_lake() const
	{
		return this->lake;
	}

	bool is_water_zone() const
	{
		return this->is_sea() || this->is_bay() || this->is_lake();
	}

	const site *get_capital_settlement() const
	{
		return this->capital_settlement;
	}

	const std::string &get_cultural_name(const culture *culture) const;

	const std::vector<region *> &get_regions() const
	{
		return this->regions;
	}

	Q_INVOKABLE void add_region(region *region);
	Q_INVOKABLE void remove_region(region *region);

	const std::vector<const country *> &get_core_countries() const
	{
		return this->core_countries;
	}

	void add_core_country(const country *country)
	{
		this->core_countries.push_back(country);
	}

	bool has_core_country_of_culture(const culture *culture) const;

	const std::vector<const site *> &get_sites() const
	{
		return this->sites;
	}

	void add_site(const site *site)
	{
		this->sites.push_back(site);
	}

	const std::vector<const character *> &get_characters() const
	{
		return this->characters;
	}

	void add_character(const character *character)
	{
		this->characters.push_back(character);
	}

signals:
	void changed();

private:
	QColor color;
	bool sea = false;
	bool bay = false;
	bool lake = false;
	site *capital_settlement = nullptr;
	std::map<const culture *, std::string> cultural_names;
	std::map<const cultural_group *, std::string> cultural_group_names;
	std::vector<region *> regions; //regions where this province is located
	std::vector<const country *> core_countries;
	std::vector<const site *> sites; //sites located in this province, used for map generation
	std::vector<const character *> characters; //characters who have this province as their home
	qunique_ptr<province_history> history;
	qunique_ptr<province_game_data> game_data;
};

}
