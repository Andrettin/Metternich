#pragma once

#include "country/culture_container.h"
#include "country/religion_container.h"
#include "economy/commodity_container.h"
#include "economy/resource_container.h"
#include "map/terrain_type_container.h"
#include "script/scripted_modifier_container.h"
#include "util/fractional_int.h"
#include "util/qunique_ptr.h"

namespace metternich {

class civilian_unit;
class commodity;
class country;
class culture;
class icon;
class improvement;
class military_unit;
class phenotype;
class province;
class religion;
class scripted_province_modifier;
class site;
class tile;
enum class military_unit_category;

template <typename scope_type>
class modifier;

class province_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* owner READ get_owner_unconst NOTIFY owner_changed)
	Q_PROPERTY(metternich::culture* culture READ get_culture_unconst NOTIFY culture_changed)
	Q_PROPERTY(metternich::religion* religion READ get_religion_unconst NOTIFY religion_changed)
	Q_PROPERTY(QString current_cultural_name READ get_current_cultural_name_qstring NOTIFY culture_changed)
	Q_PROPERTY(bool coastal READ is_coastal CONSTANT)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY territory_changed)
	Q_PROPERTY(QPoint center_tile_pos READ get_center_tile_pos NOTIFY territory_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(QVariantList military_unit_category_counts READ get_military_unit_category_counts_qvariant_list NOTIFY military_unit_category_counts_changed)

public:
	static constexpr int base_free_food_consumption = 1;

	explicit province_game_data(const metternich::province *province);
	province_game_data(const province_game_data &other) = delete;
	~province_game_data();

	void reset_non_map_data();

	void on_map_created();

	void do_turn();
	void do_events();
	void do_ai_turn();

	bool is_on_map() const
	{
		return !this->get_tiles().empty();
	}

	const country *get_owner() const
	{
		return this->owner;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_owner_unconst() const
	{
		return const_cast<metternich::country *>(this->get_owner());
	}

public:
	void set_owner(const country *country);

	bool is_capital() const;

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::culture *get_culture_unconst() const
	{
		return const_cast<metternich::culture *>(this->get_culture());
	}

public:
	void set_culture(const metternich::culture *culture);

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::religion *get_religion_unconst() const
	{
		return const_cast<metternich::religion *>(this->get_religion());
	}

public:
	void set_religion(const metternich::religion *religion);

	const std::string &get_current_cultural_name() const;

	QString get_current_cultural_name_qstring() const
	{
		return QString::fromStdString(this->get_current_cultural_name());
	}

	bool is_coastal() const
	{
		return this->coastal;
	}

	const QRect &get_territory_rect() const
	{
		return this->territory_rect;
	}

	const QPoint &get_territory_rect_center() const
	{
		return this->territory_rect_center;
	}

	void calculate_territory_rect_center();

	const std::vector<const metternich::province *> &get_neighbor_provinces() const
	{
		return this->neighbor_provinces;
	}

	void add_neighbor_province(const metternich::province *province);
	bool is_country_border_province() const;

	const QPoint &get_center_tile_pos() const
	{
		return this->center_tile_pos;
	}

	const std::vector<QPoint> &get_tiles() const
	{
		return this->tiles;
	}

	void add_tile(const QPoint &tile_pos);

	const std::vector<QPoint> &get_border_tiles() const
	{
		return this->border_tiles;
	}

	void add_border_tile(const QPoint &tile_pos);

	const std::vector<QPoint> &get_resource_tiles() const
	{
		return this->resource_tiles;
	}

	const resource_map<int> &get_resource_counts() const
	{
		return this->resource_counts;
	}

	const terrain_type_map<int> &get_tile_terrain_counts() const
	{
		return this->tile_terrain_counts;
	}

	bool produces_commodity(const commodity *commodity) const;

	void on_improvement_gained(const improvement *improvement, const int multiplier);

	const scripted_province_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_province_modifier *modifier) const;
	void add_scripted_modifier(const scripted_province_modifier *modifier, const int duration);
	void remove_scripted_modifier(const scripted_province_modifier *modifier);
	void decrement_scripted_modifiers();

	void apply_modifier(const modifier<const metternich::province> *modifier, const int multiplier = 1);

	void remove_modifier(const modifier<const metternich::province> *modifier)
	{
		this->apply_modifier(modifier, -1);
	}

	int get_free_food_consumption() const
	{
		return this->free_food_consumption;
	}

	int get_score() const
	{
		return this->score;
	}

	void change_score(const int change);

	const std::vector<military_unit *> &get_military_units() const
	{
		return this->military_units;
	}

	void add_military_unit(military_unit *military_unit);
	void remove_military_unit(military_unit *military_unit);
	void clear_military_units();

	QVariantList get_military_unit_category_counts_qvariant_list() const;

	Q_INVOKABLE int get_military_unit_category_count(const metternich::military_unit_category category) const
	{
		const auto find_iterator = this->military_unit_category_counts.find(category);

		if (find_iterator != this->military_unit_category_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_military_unit_category_count(const military_unit_category category, const int change);

	bool has_country_military_unit(const country *country) const;
	Q_INVOKABLE int get_country_military_unit_category_count(const metternich::military_unit_category category, metternich::country *country) const;

	Q_INVOKABLE QObject *get_military_unit_category_icon(const metternich::military_unit_category category) const;
	Q_INVOKABLE QString get_military_unit_category_name(const metternich::military_unit_category category) const;

	int get_output_modifier() const
	{
		return this->output_modifier;
	}

	void set_output_modifier(const int value)
	{
		if (value == this->get_output_modifier()) {
			return;
		}

		this->output_modifier = value;
	}

	void change_output_modifier(const int value)
	{
		this->set_output_modifier(this->get_output_modifier() + value);
	}

	int get_commodity_output_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_output_modifiers.find(commodity);

		if (find_iterator != this->commodity_output_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_commodity_output_modifier(const commodity *commodity, const int value)
	{
		if (value == this->get_commodity_output_modifier(commodity)) {
			return;
		}

		if (value == 0) {
			this->commodity_output_modifiers.erase(commodity);
		} else {
			this->commodity_output_modifiers[commodity] = value;
		}
	}

	void change_commodity_output_modifier(const commodity *commodity, const int value)
	{
		this->set_commodity_output_modifier(commodity, this->get_commodity_output_modifier(commodity) + value);
	}

	bool can_produce_commodity(const commodity *commodity) const;

	province_game_data &operator =(const province_game_data &other) = delete;

signals:
	void owner_changed();
	void culture_changed();
	void religion_changed();
	void territory_changed();
	void scripted_modifiers_changed();
	void military_units_changed();
	void military_unit_category_counts_changed();

private:
	const metternich::province *province = nullptr;
	const country *owner = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	bool coastal = false;
	QRect territory_rect;
	QPoint territory_rect_center = QPoint(-1, -1);
	std::vector<const metternich::province *> neighbor_provinces;
	QPoint center_tile_pos = QPoint(-1, -1);
	std::vector<QPoint> tiles;
	std::vector<QPoint> border_tiles;
	std::vector<QPoint> resource_tiles;
	std::vector<const site *> sites;
	resource_map<int> resource_counts;
	terrain_type_map<int> tile_terrain_counts;
	scripted_province_modifier_map<int> scripted_modifiers;
	int free_food_consumption = 0;
	int score = 0;
	std::vector<military_unit *> military_units;
	std::map<military_unit_category, int> military_unit_category_counts;
	int output_modifier = 0;
	commodity_map<int> commodity_output_modifiers;
};

}
