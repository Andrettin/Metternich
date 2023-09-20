#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("country/country_game_data.h")
Q_MOC_INCLUDE("country/culture.h")
Q_MOC_INCLUDE("country/religion.h")
Q_MOC_INCLUDE("map/site.h")

namespace metternich {

class character;
class country_game_data;
class country_history;
class culture;
class era;
class province;
class religion;
class site;
enum class country_type;

class country final : public named_data_entry, public data_type<country>
{
	Q_OBJECT

	Q_PROPERTY(metternich::country_type type MEMBER type READ get_type NOTIFY changed)
	Q_PROPERTY(bool great_power READ is_great_power NOTIFY changed)
	Q_PROPERTY(bool tribe READ is_tribe NOTIFY changed)
	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::religion* default_religion MEMBER default_religion NOTIFY changed)
	Q_PROPERTY(metternich::site* default_capital MEMBER default_capital NOTIFY changed)
	Q_PROPERTY(metternich::country_game_data* game_data READ get_game_data NOTIFY game_data_changed)

public:
	static constexpr const char class_identifier[] = "country";
	static constexpr const char property_class_identifier[] = "metternich::country*";
	static constexpr const char database_folder[] = "countries";
	static constexpr bool history_enabled = true;

	static constexpr size_t max_great_powers = 7; //maximum true great powers, all others are called secondary powers instead
	static constexpr int vassal_score_percent = 50;
	static constexpr int min_opinion = -200;
	static constexpr int max_opinion = 200;

	explicit country(const std::string &identifier);
	~country();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	country_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	void reset_game_data();

	country_game_data *get_game_data() const
	{
		return this->game_data.get();
	}

	country_type get_type() const
	{
		return this->type;
	}

	bool is_great_power() const;
	bool is_tribe() const;

	const QColor &get_color() const;

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const religion *get_default_religion() const
	{
		return this->default_religion;
	}

	const site *get_default_capital() const
	{
		return this->default_capital;
	}

	const std::vector<const era *> &get_eras() const
	{
		return this->eras;
	}

	const std::vector<province *> &get_core_provinces() const
	{
		return this->core_provinces;
	}

	const std::vector<const character *> &get_rulers() const
	{
		return this->rulers;
	}

	void add_ruler(const character *character)
	{
		this->rulers.push_back(character);
	}

	bool can_declare_war() const;

signals:
	void changed();
	void game_data_changed() const;

private:
	country_type type;
	QColor color;
	metternich::culture *culture = nullptr;
	religion *default_religion = nullptr;
	site *default_capital = nullptr;
	std::vector<const era *> eras; //eras this country appears in at start, for random maps
	std::vector<province *> core_provinces;
	std::vector<const character *> rulers;
	qunique_ptr<country_history> history;
	qunique_ptr<country_game_data> game_data;
};

}
