#pragma once

#include "country/country_container.h"
#include "economy/resource_container.h"

namespace metternich {

class country;
class country_palette;
class province;
enum class diplomacy_state;

class country_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* overlord READ get_overlord_unconst NOTIFY overlord_changed)
	Q_PROPERTY(bool secondary_power READ is_secondary_power NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QRect territory_rect READ get_territory_rect NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList resource_counts READ get_resource_counts_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList colonial_resource_counts READ get_colonial_resource_counts_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QVariantList vassals READ get_vassals_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QRect diplomatic_map_image_rect READ get_diplomatic_map_image_rect NOTIFY diplomatic_map_image_changed)

public:
	explicit country_game_data(const metternich::country *country) : country(country)
	{
	}

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

	bool is_secondary_power() const;

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

	const resource_map<int> &get_colonial_resource_counts() const
	{
		return this->colonial_resource_counts;
	}

	QVariantList get_colonial_resource_counts_qvariant_list() const;

	void change_colonial_resource_count(const resource *resource, const int change)
	{
		const int final_count = (this->colonial_resource_counts[resource] += change);

		if (final_count == 0) {
			this->colonial_resource_counts.erase(resource);
		}
	}

	diplomacy_state get_diplomacy_state(const metternich::country *other_country) const;
	void set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state);

	QVariantList get_vassals_qvariant_list() const;

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
		this->rank = rank;
	}

	int get_score() const
	{
		return this->score;
	}

	void change_score(const int change)
	{
		this->score += change;
	}

signals:
	void overlord_changed();
	void diplomacy_states_changed();
	void provinces_changed();
	void diplomatic_map_image_changed();

private:
	const metternich::country *country = nullptr;
	const metternich::country *overlord = nullptr;
	std::vector<const province *> provinces;
	QRect territory_rect;
	std::vector<QPoint> border_tiles;
	resource_map<int> resource_counts;
	resource_map<int> colonial_resource_counts;
	country_map<diplomacy_state> diplomacy_states;
	QImage diplomatic_map_image;
	QImage selected_diplomatic_map_image;
	QRect diplomatic_map_image_rect;
	int rank = 0;
	int score = 0;
};

}
