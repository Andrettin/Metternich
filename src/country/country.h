#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

namespace metternich {

class country_game_data;
class country_history;
class country_palette;
class culture;
class era;
class landed_title;
class province;
enum class country_type;

class country final : public named_data_entry, public data_type<country>
{
	Q_OBJECT

	Q_PROPERTY(metternich::country_type type MEMBER type READ get_type)
	Q_PROPERTY(bool great_power READ is_great_power NOTIFY changed)
	Q_PROPERTY(bool tribe READ is_tribe NOTIFY changed)
	Q_PROPERTY(QColor color MEMBER color READ get_color)
	Q_PROPERTY(metternich::country_palette* palette MEMBER palette)
	Q_PROPERTY(metternich::culture* culture MEMBER culture)
	Q_PROPERTY(metternich::province* capital_province MEMBER capital_province)
	Q_PROPERTY(metternich::country_game_data* game_data READ get_game_data NOTIFY game_data_changed)

public:
	static constexpr const char class_identifier[] = "country";
	static constexpr const char property_class_identifier[] = "metternich::country*";
	static constexpr const char database_folder[] = "countries";
	static constexpr bool history_enabled = true;

	static constexpr size_t max_great_powers = 7; //maximum true great powers, all others are called secondary powers instead
	static constexpr int score_per_province = 100;
	static constexpr int vassal_province_score_percent = 50;

	explicit country(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
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
	const country_palette *get_palette() const;

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const province *get_capital_province() const
	{
		return this->capital_province;
	}

	const landed_title *get_title() const
	{
		return this->title;
	}

	void create_title();

	const std::vector<const province *> &get_provinces() const
	{
		return this->provinces;
	}

signals:
	void changed();
	void game_data_changed() const;

private:
	country_type type;
	QColor color;
	country_palette *palette = nullptr;
	metternich::culture *culture = nullptr;
	province *capital_province = nullptr;
	landed_title *title = nullptr;
	std::vector<const era *> eras; //eras this country appears in at start, for random maps
	std::vector<const province *> provinces; //provinces for this country when it is generated in random maps
	qunique_ptr<country_history> history;
	qunique_ptr<country_game_data> game_data;
};

}
