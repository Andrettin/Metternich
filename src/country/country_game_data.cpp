#include "metternich.h"

#include "country/country_game_data.h"

#include "country/country.h"
#include "country/diplomacy_state.h"
#include "database/defines.h"
#include "game/game.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/tile.h"
#include "util/container_util.h"
#include "util/point_util.h"
#include "util/size_util.h"
#include "util/vector_util.h"

namespace metternich {

QVariantList country_game_data::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}

void country_game_data::add_province(const province *province)
{
	this->provinces.push_back(province);

	const map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

	for (const metternich::province *border_province : province_game_data->get_border_provinces()) {
		const metternich::province_game_data *border_province_game_data = border_province->get_game_data();
		if (border_province_game_data->get_owner() != this->country) {
			continue;
		}

		for (const QPoint &tile_pos : border_province_game_data->get_border_tiles()) {
			if (!map->is_tile_on_country_border(tile_pos)) {
				std::erase(this->border_tiles, tile_pos);
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

	const map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

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
		this->create_diplomatic_map_image();
	}
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
		this->overlord = other_country;
	} else {
		if (this->overlord == other_country) {
			this->overlord = nullptr;
		}
	}

	if (state == diplomacy_state::peace) {
		this->diplomacy_states.erase(other_country);
	} else {
		this->diplomacy_states[other_country] = state;
	}
}

const QColor &country_game_data::get_diplomatic_map_color() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->get_diplomatic_map_color();
	}

	return this->country->get_color();
}

void country_game_data::create_diplomatic_map_image()
{
	const QSize &tile_pixel_size = game::get()->get_diplomatic_map_tile_pixel_size();

	this->diplomatic_map_image = QImage(this->territory_rect.size() * tile_pixel_size, QImage::Format_RGBA8888);
	this->diplomatic_map_image.fill(Qt::transparent);

	this->selected_diplomatic_map_image = this->diplomatic_map_image;

	const map *map = map::get();

	const QColor &color = this->get_diplomatic_map_color();
	static const QColor selected_color(Qt::yellow);

	std::vector<QPoint> country_pixels;

	for (int x = 0; x < this->territory_rect.width(); ++x) {
		for (int y = 0; y < this->territory_rect.height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->territory_rect.topLeft() + relative_tile_pos);

			if (tile->get_owner() != this->country) {
				continue;
			}

			const QPoint top_left_pixel_pos = relative_tile_pos * size::to_point(tile_pixel_size);

			for (int pixel_x_offset = 0; pixel_x_offset < tile_pixel_size.width(); ++pixel_x_offset) {
				for (int pixel_y_offset = 0; pixel_y_offset < tile_pixel_size.height(); ++pixel_y_offset) {
					const QPoint pixel_pos = top_left_pixel_pos + QPoint(pixel_x_offset, pixel_y_offset);
					this->diplomatic_map_image.setPixelColor(pixel_pos, color);
					this->selected_diplomatic_map_image.setPixelColor(pixel_pos, selected_color);

					country_pixels.push_back(pixel_pos);
				}
			}
		}
	}

	this->diplomatic_map_border_pixels.clear();

	for (const QPoint &pixel_pos : country_pixels) {
		if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (this->diplomatic_map_image.width() - 1) || pixel_pos.y() == (this->diplomatic_map_image.height() - 1)) {
			this->diplomatic_map_border_pixels.push_back(pixel_pos);
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
			this->diplomatic_map_border_pixels.push_back(pixel_pos);
		}
	}

	const QColor &border_pixel_color = defines::get()->get_country_border_color();

	for (const QPoint &border_pixel_pos : this->diplomatic_map_border_pixels) {
		this->diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
		this->selected_diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
	}

	this->diplomatic_map_image_rect = QRect(this->territory_rect.topLeft() * size::to_point(tile_pixel_size), this->diplomatic_map_image.size());

	emit diplomatic_map_image_changed();
}

}
