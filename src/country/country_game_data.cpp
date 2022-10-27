#include "metternich.h"

#include "country/country_game_data.h"

#include "country/country.h"
#include "country/country_type.h"
#include "country/diplomacy_state.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/resource.h"
#include "game/game.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/tile.h"
#include "util/container_util.h"
#include "util/image_util.h"
#include "util/map_util.h"
#include "util/point_util.h"
#include "util/size_util.h"
#include "util/thread_pool.h"
#include "util/vector_util.h"

#include "xbrz.h"

namespace metternich {

void country_game_data::set_overlord(const metternich::country *overlord)
{
	if (overlord == this->get_overlord()) {
		return;
	}

	if (this->overlord != nullptr) {
		this->overlord->get_game_data()->change_province_score(-this->get_province_count() * country::score_per_province * country::vassal_province_score_percent / 100);

		for (const auto &[resource, count] : this->get_resource_counts()) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, -count);
		}
	}

	this->overlord = overlord;

	if (this->overlord != nullptr) {
		this->overlord->get_game_data()->change_province_score(this->get_province_count() * country::score_per_province * country::vassal_province_score_percent / 100);

		for (const auto &[resource, count] : this->get_resource_counts()) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, count);
		}
	}

	if (game::get()->is_running()) {
		emit overlord_changed();
	}
}

bool country_game_data::is_secondary_power() const
{
	//a country is a secondary power if its type is great power, and it is not high-ranking enough to be considered a true great power
	if (this->country->get_type() != country_type::great_power) {
		return false;
	}

	return this->get_rank() >= country::max_great_powers;
}

QVariantList country_game_data::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}

void country_game_data::add_province(const province *province)
{
	this->provinces.push_back(province);

	map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

	this->change_province_score(country::score_per_province);

	for (const auto &[resource, count] : province_game_data->get_resource_counts()) {
		this->change_resource_count(resource, count);

		if (this->get_overlord() != nullptr) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, count);
		}
	}

	for (const metternich::province *border_province : province_game_data->get_border_provinces()) {
		const metternich::province_game_data *border_province_game_data = border_province->get_game_data();
		if (border_province_game_data->get_owner() != this->country) {
			continue;
		}

		for (const QPoint &tile_pos : border_province_game_data->get_border_tiles()) {
			if (!map->is_tile_on_country_border(tile_pos)) {
				std::erase(this->border_tiles, tile_pos);
			}

			if (game::get()->is_running()) {
				map->calculate_tile_country_border_directions(tile_pos);
			}
		}
	}

	for (const QPoint &tile_pos : province_game_data->get_border_tiles()) {
		if (map->is_tile_on_country_border(tile_pos)) {
			this->border_tiles.push_back(tile_pos);
		}
	}

	this->calculate_territory_rect();

	if (this->get_provinces().size() == 1) {
		game::get()->add_country(this->country);
	}

	if (game::get()->is_running()) {
		emit provinces_changed();
	}
}

void country_game_data::remove_province(const province *province)
{
	std::erase(this->provinces, province);

	map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

	this->change_province_score(-country::score_per_province);

	for (const auto &[resource, count] : province_game_data->get_resource_counts()) {
		this->change_resource_count(resource, -count);

		if (this->get_overlord() != nullptr) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, -count);
		}
	}

	for (const QPoint &tile_pos : province_game_data->get_border_tiles()) {
		std::erase(this->border_tiles, tile_pos);
	}

	for (const metternich::province *border_province : province_game_data->get_border_provinces()) {
		const metternich::province_game_data *border_province_game_data = border_province->get_game_data();
		if (border_province_game_data->get_owner() != this->country) {
			continue;
		}

		for (const QPoint &tile_pos : border_province_game_data->get_border_tiles()) {
			if (map->is_tile_on_country_border(tile_pos) && !vector::contains(this->get_border_tiles(), tile_pos)) {
				this->border_tiles.push_back(tile_pos);
			}

			if (game::get()->is_running()) {
				map->calculate_tile_country_border_directions(tile_pos);
			}
		}
	}

	this->calculate_territory_rect();

	if (this->get_provinces().empty()) {
		game::get()->remove_country(this->country);
	}

	if (game::get()->is_running()) {
		emit provinces_changed();
	}
}

void country_game_data::calculate_territory_rect()
{
	QRect territory_rect;

	for (const province *province : this->get_provinces()) {
		const province_game_data *province_game_data = province->get_game_data();

		if (territory_rect.isNull()) {
			territory_rect = province_game_data->get_territory_rect();
		} else {
			territory_rect = territory_rect.united(province_game_data->get_territory_rect());
		}
	}

	this->territory_rect = territory_rect;

	if (game::get()->is_running()) {
		thread_pool::get()->co_spawn_sync([this]() -> boost::asio::awaitable<void> {
			co_await this->create_diplomatic_map_image();
		});
	}
}

QVariantList country_game_data::get_resource_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_resource_counts());
}

QVariantList country_game_data::get_vassal_resource_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_vassal_resource_counts());
}

diplomacy_state country_game_data::get_diplomacy_state(const metternich::country *other_country) const
{
	const auto find_iterator = this->diplomacy_states.find(other_country);

	if (find_iterator != this->diplomacy_states.end()) {
		return find_iterator->second;
	}

	return diplomacy_state::peace;
}

void country_game_data::set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state)
{
	if (is_vassalage_diplomacy_state(state)) {
		this->set_overlord(other_country);
	} else {
		if (this->get_overlord() == other_country) {
			this->set_overlord(nullptr);
		}
	}

	if (state == diplomacy_state::peace) {
		this->diplomacy_states.erase(other_country);
	} else {
		this->diplomacy_states[other_country] = state;
	}

	if (game::get()->is_running()) {
		emit diplomacy_states_changed();
	}
}

QVariantList country_game_data::get_vassals_qvariant_list() const
{
	std::vector<const metternich::country *> vassals;

	for (const auto &[country, diplomacy_state] : this->diplomacy_states) {
		if (is_overlordship_diplomacy_state(diplomacy_state)) {
			vassals.push_back(country);
		}
	}

	return container::to_qvariant_list(vassals);
}

const QColor &country_game_data::get_diplomatic_map_color() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->get_diplomatic_map_color();
	}

	return this->country->get_color();
}

const country_palette *country_game_data::get_palette() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->get_palette();
	}

	return this->country->get_palette();
}

boost::asio::awaitable<void> country_game_data::create_diplomatic_map_image()
{
	const int tile_pixel_size = game::get()->get_diplomatic_map_tile_pixel_size();

	this->diplomatic_map_image = QImage(this->territory_rect.size() * tile_pixel_size, QImage::Format_RGBA8888);
	this->diplomatic_map_image.fill(Qt::transparent);

	this->selected_diplomatic_map_image = this->diplomatic_map_image;

	const map *map = map::get();

	const QColor &color = this->get_diplomatic_map_color();
	const QColor &selected_color = defines::get()->get_selected_country_color();

	std::vector<QPoint> country_pixels;

	for (int x = 0; x < this->territory_rect.width(); ++x) {
		for (int y = 0; y < this->territory_rect.height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->territory_rect.topLeft() + relative_tile_pos);

			if (tile->get_owner() != this->country) {
				continue;
			}

			const QPoint top_left_pixel_pos = relative_tile_pos * tile_pixel_size;

			for (int pixel_x_offset = 0; pixel_x_offset < tile_pixel_size; ++pixel_x_offset) {
				for (int pixel_y_offset = 0; pixel_y_offset < tile_pixel_size; ++pixel_y_offset) {
					const QPoint pixel_pos = top_left_pixel_pos + QPoint(pixel_x_offset, pixel_y_offset);
					this->diplomatic_map_image.setPixelColor(pixel_pos, color);
					this->selected_diplomatic_map_image.setPixelColor(pixel_pos, selected_color);

					country_pixels.push_back(pixel_pos);
				}
			}
		}
	}

	std::vector<QPoint> border_pixels;

	for (const QPoint &pixel_pos : country_pixels) {
		if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (this->diplomatic_map_image.width() - 1) || pixel_pos.y() == (this->diplomatic_map_image.height() - 1)) {
			border_pixels.push_back(pixel_pos);
			continue;
		}

		bool is_border_pixel = false;
		point::for_each_cardinally_adjacent_until(pixel_pos, [this, &color, &is_border_pixel](const QPoint &adjacent_pos) {
			if (this->diplomatic_map_image.pixelColor(adjacent_pos) == color) {
				return false;
			}

			is_border_pixel = true;
			return true;
		});

		if (is_border_pixel) {
			border_pixels.push_back(pixel_pos);
		}
	}

	const QColor &border_pixel_color = defines::get()->get_country_border_color();

	for (const QPoint &border_pixel_pos : border_pixels) {
		this->diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
		this->selected_diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	QImage scaled_diplomatic_map_image;
	QImage scaled_selected_diplomatic_map_image;

	co_await thread_pool::get()->co_spawn_awaitable([this, &scale_factor, &scaled_diplomatic_map_image, &scaled_selected_diplomatic_map_image]() -> boost::asio::awaitable<void> {
		scaled_diplomatic_map_image = co_await image::scale<QImage::Format_ARGB32>(this->diplomatic_map_image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});

		scaled_selected_diplomatic_map_image = co_await image::scale<QImage::Format_ARGB32>(this->selected_diplomatic_map_image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->diplomatic_map_image = std::move(scaled_diplomatic_map_image);
	this->selected_diplomatic_map_image = std::move(scaled_selected_diplomatic_map_image);

	this->diplomatic_map_image_rect = QRect(this->territory_rect.topLeft() * tile_pixel_size * scale_factor, this->diplomatic_map_image.size());

	emit diplomatic_map_image_changed();
}

void country_game_data::change_province_score(const int change)
{
	this->change_score(change);

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_province_score(change * country::vassal_province_score_percent / 100);
	}
}

}
