#pragma once

#include "country/country_container.h"
#include "economy/resource_container.h"
#include "technology/technology_container.h"
#include "util/qunique_ptr.h"

namespace metternich {

class civilian_unit;
class country;
class country_palette;
class province;
enum class diplomacy_state;

class country_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* overlord READ get_overlord_unconst NOTIFY overlord_changed)
	Q_PROPERTY(bool true_great_power READ is_true_great_power NOTIFY rank_changed)
	Q_PROPERTY(bool secondary_power READ is_secondary_power NOTIFY rank_changed)
	Q_PROPERTY(QString type_name READ get_type_name_qstring NOTIFY type_name_changed)
	Q_PROPERTY(QString vassalage_type_name READ get_vassalage_type_name_qstring NOTIFY vassalage_type_name_changed)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList contiguous_territory_rects READ get_contiguous_territory_rects_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QRect main_contiguous_territory_rect READ get_main_contiguous_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList resource_counts READ get_resource_counts_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList vassal_resource_counts READ get_vassal_resource_counts_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QVariantList vassals READ get_vassals_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QVariantList colonies READ get_colonies_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QRect diplomatic_map_image_rect READ get_diplomatic_map_image_rect NOTIFY diplomatic_map_image_changed)
	Q_PROPERTY(int rank READ get_rank NOTIFY rank_changed)
	Q_PROPERTY(int score READ get_score NOTIFY score_changed)
	Q_PROPERTY(int population READ get_population NOTIFY population_changed)
	Q_PROPERTY(QColor diplomatic_map_color READ get_diplomatic_map_color NOTIFY overlord_changed)

public:
	explicit country_game_data(const metternich::country *country);
	~country_game_data();

	const metternich::country *get_overlord() const
	{
		return this->overlord;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_overlord_unconst() const
	{
		return const_cast<metternich::country *>(this->get_overlord());
	}

public:
	void set_overlord(const metternich::country *overlord);

	bool is_vassal_of(const metternich::country *country) const;
	bool is_any_vassal_of(const metternich::country *country) const;

	bool is_true_great_power() const;
	bool is_secondary_power() const;

	std::string get_type_name() const;

	QString get_type_name_qstring() const
	{
		return QString::fromStdString(this->get_type_name());
	}

	std::string get_vassalage_type_name() const;

	QString get_vassalage_type_name_qstring() const
	{
		return QString::fromStdString(this->get_vassalage_type_name());
	}

	const std::vector<const province *> &get_provinces() const
	{
		return this->provinces;
	}

	QVariantList get_provinces_qvariant_list() const;
	void add_province(const province *province);
	void remove_province(const province *province);

	int get_province_count() const
	{
		return static_cast<int>(this->get_provinces().size());
	}

	bool is_alive() const
	{
		return !this->get_provinces().empty();
	}

	const QRect &get_territory_rect() const
	{
		return this->territory_rect;
	}

	void calculate_territory_rect();

	const std::vector<QRect> &get_contiguous_territory_rects() const
	{
		return this->contiguous_territory_rects;
	}

	QVariantList get_contiguous_territory_rects_qvariant_list() const;

	const QRect &get_main_contiguous_territory_rect() const
	{
		return this->main_contiguous_territory_rect;
	}

	const std::vector<QPoint> &get_border_tiles() const
	{
		return this->border_tiles;
	}

	const resource_map<int> &get_resource_counts() const
	{
		return this->resource_counts;
	}

	QVariantList get_resource_counts_qvariant_list() const;

	void change_resource_count(const resource *resource, const int change)
	{
		const int final_count = (this->resource_counts[resource] += change);

		if (final_count == 0) {
			this->resource_counts.erase(resource);
		}
	}

	const resource_map<int> &get_vassal_resource_counts() const
	{
		return this->vassal_resource_counts;
	}

	QVariantList get_vassal_resource_counts_qvariant_list() const;

	void change_vassal_resource_count(const resource *resource, const int change)
	{
		const int final_count = (this->vassal_resource_counts[resource] += change);

		if (final_count == 0) {
			this->vassal_resource_counts.erase(resource);
		}
	}

	diplomacy_state get_diplomacy_state(const metternich::country *other_country) const;
	void set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state);

	std::vector<const metternich::country *> get_vassals() const;
	QVariantList get_vassals_qvariant_list() const;
	QVariantList get_colonies_qvariant_list() const;

	const QColor &get_diplomatic_map_color() const;
	const country_palette *get_palette() const;

	const QImage &get_diplomatic_map_image() const
	{
		return this->diplomatic_map_image;
	}

	[[nodiscard]]
	boost::asio::awaitable<void> create_diplomatic_map_image();

	const QRect &get_diplomatic_map_image_rect() const
	{
		return this->diplomatic_map_image_rect;
	}

	const QImage &get_selected_diplomatic_map_image() const
	{
		return this->selected_diplomatic_map_image;
	}

	int get_rank() const
	{
		return this->rank;
	}

	void set_rank(const int rank)
	{
		if (rank == this->get_rank()) {
			return;
		}

		this->rank = rank;
		emit rank_changed();
	}

	int get_score() const
	{
		return this->score;
	}

	void change_score(const int change)
	{
		if (change == 0) {
			return;
		}

		this->score += change;

		emit score_changed();
	}

	int get_province_score() const;
	void change_province_score(const int change);

	int get_population() const
	{
		return this->population;
	}

	void change_population(const int change);

	bool can_declare_war_on(const metternich::country *other_country) const;

	bool has_technology(const technology *technology) const
	{
		return this->technologies.contains(technology);
	}

	void add_technology(const technology *technology)
	{
		this->technologies.insert(technology);
	}

	void add_civilian_unit(qunique_ptr<metternich::civilian_unit> &&civilian_unit);
	void remove_civilian_unit(metternich::civilian_unit *civilian_unit);

	void do_turn();

signals:
	void overlord_changed();
	void type_name_changed();
	void vassalage_type_name_changed();
	void diplomacy_states_changed();
	void provinces_changed();
	void diplomatic_map_image_changed();
	void rank_changed();
	void score_changed();
	void population_changed();

private:
	const metternich::country *country = nullptr;
	const metternich::country *overlord = nullptr;
	std::vector<const province *> provinces;
	QRect territory_rect;
	std::vector<QRect> contiguous_territory_rects;
	QRect main_contiguous_territory_rect;
	std::vector<QPoint> border_tiles;
	resource_map<int> resource_counts;
	resource_map<int> vassal_resource_counts;
	country_map<diplomacy_state> diplomacy_states;
	QImage diplomatic_map_image;
	QImage selected_diplomatic_map_image;
	QRect diplomatic_map_image_rect;
	int rank = 0;
	int score = 0;
	int population = 0;
	technology_set technologies;
	std::vector<qunique_ptr<civilian_unit>> civilian_units;
};

}
