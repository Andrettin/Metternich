#include "metternich.h"

#include "map/map.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_technology.h"
#include "economy/resource.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_container.h"
#include "map/province_map_data.h"
#include "map/route.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/image_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

#include "xbrz.h"

namespace metternich {

map::map()
{
}

map::~map()
{
}

void map::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "diplomatic_map_tile_scale") {
		this->diplomatic_map_tile_scale = decimillesimal_int(value);
	} else if (key == "minimap_tile_scale") {
		this->minimap_tile_scale = decimillesimal_int(value);
	} else {
		throw std::runtime_error(std::format("Invalid map data property: \"{}\".", key));
	}
}

void map::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "size") {
		const QSize map_size = scope.to_size();
		this->set_size(map_size);
		this->create_tiles();
	} else if (tag == "tile_provinces") {
		int y = 0;
		scope.for_each_child([this, &y](const gsml_data &child_scope) {
			for (size_t x = 0; x < child_scope.get_values().size(); ++x) {
				const std::string &value = child_scope.get_values()[x];
				province *province = nullptr;
				if (value != "n") {
					const int map_province_index = std::stoi(value);
					province = this->get_provinces().at(map_province_index);
				}

				if (province != nullptr) {
					const QPoint tile_pos(static_cast<int>(x), y);
					this->set_tile_province(tile_pos, province);
				}
			}
			++y;
		});

		QCoro::waitFor(this->process_border_tiles());
		for (const province *province : this->get_provinces()) {
			province->get_map_data()->on_map_created();
		}
		this->initialize_diplomatic_map();
		this->initialize_province_map();
	} else {
		throw std::runtime_error(std::format("Invalid map data scope: \"{}\".", tag));
	}
}

gsml_data map::to_gsml_data() const
{
	gsml_data data("map");

	data.add_child("size", gsml_data::from_size(this->get_size()));
	data.add_property("diplomatic_map_tile_scale", this->get_diplomatic_map_tile_scale().to_string());
	data.add_property("minimap_tile_scale", this->get_minimap_tile_scale().to_string());

	std::map<const province *, size_t> map_province_indices;
	for (size_t i = 0; i < this->get_provinces().size(); ++i) {
		map_province_indices[this->get_provinces().at(i)] = i;
	}

	gsml_data tile_provinces_data("tile_provinces");
	for (int y = 0; y < this->get_height(); ++y) {
		gsml_data tile_provinces_row_data;
		for (int x = 0; x < this->get_width(); ++x) {
			const QPoint tile_pos(x, y);
			const tile *tile = this->get_tile(tile_pos);
			assert_throw(tile != nullptr);
			tile_provinces_row_data.add_value(tile->get_province() != nullptr ? std::to_string(map_province_indices.find(tile->get_province())->second) : "n");
		}
		tile_provinces_data.add_child(std::move(tile_provinces_row_data));
	}
	data.add_child(std::move(tile_provinces_data));

	return data;
}

void map::create_tiles()
{
	this->tiles = std::make_unique<std::vector<tile>>();

	assert_throw(!this->get_size().isNull());

	const int tile_quantity = this->get_width() * this->get_height();

	this->tiles->resize(tile_quantity, tile());
}

QCoro::Task<void> map::initialize(const bool province_post_processing_enabled)
{
	if (province_post_processing_enabled) {
		//assign tiles without provinces to the most-adjacent province
		std::vector<QPoint> tiles_to_check;
		for (int x = 0; x < this->get_width(); ++x) {
			for (int y = 0; y < this->get_height(); ++y) {
				QPoint tile_pos(x, y);
				const tile *tile = this->get_tile(tile_pos);

				if (tile->get_province() != nullptr) {
					continue;
				}

				tiles_to_check.push_back(std::move(tile_pos));
			}
		}
		vector::shuffle(tiles_to_check);

		this->do_province_post_processing(tiles_to_check, true);
		this->do_province_post_processing(tiles_to_check, false);
	}

	co_await this->process_border_tiles();

	std::vector<province *> provinces;
	for (province *province : province::get_all()) {
		if (province->get_map_data()->is_on_map()) {
			provinces.push_back(province);
		}
	}

	std::vector<const site *> sites;
	for (const site *site : site::get_all()) {
		if (site->get_map_data()->is_on_map()) {
			sites.push_back(site);
		}
	}

	this->provinces = std::move(provinces);
	this->sites = std::move(sites);

	for (const province *province : this->get_provinces()) {
		province->get_map_data()->on_map_created();
	}

	this->initialize_diplomatic_map();
	this->initialize_province_map();

	emit provinces_changed();
	emit sites_changed();
}

void map::do_province_post_processing(std::vector<QPoint> tiles_to_check, const bool only_water_zones)
{
	//assign tiles without provinces to the most-adjacent province
	while (!tiles_to_check.empty()) {
		std::vector<QPoint> new_tiles_to_check;

		for (const QPoint &tile_pos : tiles_to_check) {
			try {
				const tile *tile = this->get_tile(tile_pos);
				if (tile->get_province() != nullptr) {
					continue;
				}

				std::map<province *, int, province_compare> adjacent_province_counts;
				std::vector<QPoint> adjacent_without_province;
				point::for_each_adjacent(tile_pos, [this, tile, &adjacent_province_counts, &adjacent_without_province, only_water_zones](const QPoint &adjacent_pos) {
					if (!this->contains(adjacent_pos)) {
						return;
					}

					const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);

					if (adjacent_tile->get_province() == nullptr) {
						adjacent_without_province.push_back(adjacent_pos);
						return;
					}

					if (only_water_zones && !adjacent_tile->get_province()->is_water_zone()) {
						return;
					}

					adjacent_province_counts[adjacent_tile->get_province()]++;
				});

				int best_count = 0;
				std::vector<province *> best_provinces;

				for (const auto &[province, province_count] : adjacent_province_counts) {
					if (province_count > best_count) {
						best_provinces.clear();
						best_count = province_count;
					} else if (province_count < best_count) {
						continue;
					}

					best_provinces.push_back(province);
				}

				if (!best_provinces.empty()) {
					this->set_tile_province(tile_pos, vector::get_random(best_provinces));
					vector::merge(new_tiles_to_check, std::move(adjacent_without_province));
				}
			} catch (...) {
				exception::report(std::current_exception());
			}
		}

		tiles_to_check = std::move(new_tiles_to_check);
		vector::shuffle(tiles_to_check);
	}
}

QCoro::Task<void> map::process_border_tiles()
{
	std::vector<QFuture<void>> futures;

	for (const province *province : province::get_all()) {
		province_map_data *province_map_data = province->get_map_data();
		if (!province_map_data->is_on_map()) {
			continue;
		}

		QFuture<void> future = QtConcurrent::run([this, province, province_map_data]() {
			for (const QPoint &tile_pos : province_map_data->get_tiles()) {
				bool is_border_tile = false;

				point::for_each_adjacent(tile_pos, [this, &tile_pos, province, province_map_data, &is_border_tile](const QPoint &adjacent_pos) {
					if (!this->contains(adjacent_pos)) {
						is_border_tile = true;
						return;
					}

					const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
					const metternich::province *adjacent_province = adjacent_tile->get_province();

					if (province != adjacent_province) {
						if (adjacent_province != nullptr && !vector::contains(province_map_data->get_neighbor_provinces(), adjacent_province)) {
							province_map_data->add_neighbor_province(adjacent_province);
						}

						is_border_tile = true;
					}
				});

				if (is_border_tile) {
					province_map_data->process_border_tile(tile_pos);
				}
			}
		});

		futures.push_back(std::move(future));
	}

	for (QFuture<void> &future : futures) {
		co_await future;
	}
}

void map::clear()
{
	for (province *province : province::get_all()) {
		province->reset_map_data();
	}

	for (site *site : site::get_all()) {
		site->reset_map_data();
	}

	for (route *route : route::get_all()) {
		route->reset_game_data();
	}

	this->provinces.clear();
	this->sites.clear();
	this->tiles.reset();
	this->ocean_diplomatic_map_image = QImage();
}

int map::get_pos_index(const QPoint &pos) const
{
	return point::to_index(pos, this->get_width());
}

tile *map::get_tile(const QPoint &pos) const
{
	const int index = this->get_pos_index(pos);
	return &this->tiles->at(index);
}

const metternich::province *map::get_tile_province(const QPoint &tile_pos) const
{
	const tile *tile = this->get_tile(tile_pos);
	if (tile == nullptr) {
		return nullptr;
	}

	return tile->get_province();
}

void map::set_tile_province(const QPoint &tile_pos, province *province)
{
	assert_throw(province != nullptr);

	tile *tile = this->get_tile(tile_pos);
	tile->set_province(province);
	province->get_map_data()->add_tile(tile_pos);
}

void map::set_tile_site(const QPoint &tile_pos, const site *site)
{
	tile *tile = this->get_tile(tile_pos);
	tile->set_site(site);

	if (site->get_province() != nullptr && site->get_province() != tile->get_province() && site->get_province()->get_map_data()->is_on_map()) {
		log::log_error(std::format("Site \"{}\" was not placed within its province.", site->get_identifier()));
	}

	switch (site->get_type()) {
		case site_type::holding:
		case site_type::habitable_world:
			if (tile->get_province() == nullptr || (site->get_province() != nullptr && tile->get_province() != site->get_province())) {
				log::log_error(std::format("Holding \"{}\" {} was not placed within its province.", site->get_identifier(), point::to_string(tile_pos)));
			}
			break;
		case site_type::resource:
			if (site->get_map_data()->get_resource() == nullptr) {
				log::log_error(std::format("Resource site \"{}\" {} has no resource.", site->get_identifier(), point::to_string(tile_pos)));
				break;
			}
			break;
		default:
			break;
	}

	if (tile->get_resource() != nullptr) {
		if (tile->get_resource()->is_near_water() && tile->get_province() != nullptr && !tile->get_province()->get_map_data()->is_near_water()) {
			log::log_error(std::format("Site \"{}\" {} has near water resource \"{}\", but is not near water.", site->get_identifier(), point::to_string(tile_pos), tile->get_resource()->get_identifier()));
		}

		if (tile->get_resource()->is_coastal() && tile->get_province() != nullptr && !tile->get_province()->get_map_data()->is_coastal()) {
			log::log_error(std::format("Site \"{}\" {} has coastal resource \"{}\", but is not coastal.", site->get_identifier(), point::to_string(tile_pos), tile->get_resource()->get_identifier()));
		}

		if (!vector::contains(tile->get_resource()->get_terrain_types(), tile->get_province()->get_map_data()->get_terrain())) {
			log::log_error(std::format("Site \"{}\" {} has resource \"{}\", which doesn't match its \"{}\" terrain type.", site->get_identifier(), point::to_string(tile_pos), tile->get_resource()->get_identifier(), tile->get_province()->get_map_data()->get_terrain()->get_identifier()));
		}
	}

	site->get_map_data()->set_tile_pos(tile_pos);

	assert_throw(tile->get_province() != nullptr);
	tile->get_province()->get_map_data()->process_site_tile(tile_pos);
}

QCoro::Task<void> map::set_tile_resource_discovered(const QPoint &tile_pos, const bool discovered)
{
	tile *tile = this->get_tile(tile_pos);

	if (discovered == tile->is_resource_discovered()) {
		co_return;
	}

	const resource *resource = tile->get_resource();
	assert_throw(resource != nullptr);
	assert_throw(tile->get_site() != nullptr);
	tile->get_site()->get_game_data()->set_resource_discovered(discovered);

	if (discovered && resource->get_discovery_technology() != nullptr && tile->get_owner() != nullptr) {
		for (const domain *domain : game::get()->get_countries()) {
			domain_game_data *domain_game_data = domain->get_game_data();
			domain_technology *domain_technology = domain->get_technology();

			if (!domain_game_data->is_tile_explored(tile_pos)) {
				continue;
			}

			if (domain_technology->can_gain_technology(resource->get_discovery_technology())) {
				co_await domain_technology->add_technology(resource->get_discovery_technology());

				if (game::get()->is_running()) {
					emit domain_technology->technology_researched(resource->get_discovery_technology());
				}
			}
		}
	}

	emit tile_resource_changed(tile_pos);
}

bool map::is_tile_near_celestial_body(const QPoint &tile_pos) const
{
	bool result = false;

	point::for_each_adjacent_until(tile_pos, [this, &result](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			return false;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);

		if (adjacent_tile->get_site() != nullptr && adjacent_tile->get_site()->is_celestial_body()) {
			result = true;
			return true;
		}

		return false;
	});

	return result;
}

bool map::is_tile_on_province_border(const QPoint &tile_pos) const
{
	const tile *tile = this->get_tile(tile_pos);
	const province *tile_province = tile->get_province();

	bool result = false;

	point::for_each_adjacent_until(tile_pos, [this, tile_province, &result](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			return false;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
		const province *adjacent_province = adjacent_tile->get_province();

		if (adjacent_province != tile_province) {
			result = true;
			return true;
		}

		return false;
	});

	return result;
}

bool map::is_tile_on_province_border_with(const QPoint &tile_pos, const province *other_province) const
{
	bool result = false;

	point::for_each_adjacent_until(tile_pos, [this, other_province, &result](const QPoint &adjacent_pos) {
		if (!this->contains(adjacent_pos)) {
			return false;
		}

		const metternich::tile *adjacent_tile = this->get_tile(adjacent_pos);
		const province *adjacent_province = adjacent_tile->get_province();

		if (adjacent_province == other_province) {
			result = true;
			return true;
		}

		return false;
	});

	return result;
}

QVariantList map::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}

QVariantList map::get_sites_qvariant_list() const
{
	return container::to_qvariant_list(this->get_sites());
}

void map::initialize_diplomatic_map()
{
	const decimillesimal_int &tile_scale = this->get_diplomatic_map_tile_scale();
	const QSize image_size = this->get_size() * tile_scale;

	if (image_size != this->diplomatic_map_image_size) {
		this->diplomatic_map_image_size = image_size;
		emit diplomatic_map_image_size_changed();
	}
}

void map::initialize_province_map()
{
	const int tile_scale = defines::get()->get_province_map_tile_scale();
	const QSize image_size = this->get_size() * tile_scale;
	if (image_size != this->province_map_image_size) {
		this->province_map_image_size = image_size;
		emit province_map_image_size_changed();
	}
}

QCoro::Task<void> map::create_ocean_diplomatic_map_image()
{
	const decimillesimal_int &tile_scale = this->get_diplomatic_map_tile_scale();
	QSize image_size;
	if (tile_scale < 1) {
		image_size = this->get_size() * tile_scale;
	} else {
		image_size = this->get_size();
	}

	this->ocean_diplomatic_map_image = QImage(image_size, QImage::Format_RGBA8888);
	this->ocean_diplomatic_map_image.fill(Qt::transparent);

	const QColor &color = defines::get()->get_ocean_color();

	for (int x = 0; x < image_size.width(); ++x) {
		for (int y = 0; y < image_size.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;
			const tile *tile = this->get_tile(tile_pos);

			if (tile->get_province() == nullptr) {
				continue;
			}

			if (!tile->get_province()->is_water_zone()) {
				continue;
			}

			this->ocean_diplomatic_map_image.setPixelColor(pixel_pos, color);
		}
	}

	if (tile_scale > 1) {
		QImage scaled_ocean_diplomatic_map_image;

		co_await QtConcurrent::run([this, tile_scale, &scaled_ocean_diplomatic_map_image]() {
			scaled_ocean_diplomatic_map_image = image::scale<QImage::Format_ARGB32>(this->ocean_diplomatic_map_image, centesimal_int(tile_scale), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
				xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
			});
		});

		this->ocean_diplomatic_map_image = std::move(scaled_ocean_diplomatic_map_image);
	}

	std::vector<QPoint> border_pixels;

	const QRect image_rect = this->ocean_diplomatic_map_image.rect();

	for (int x = 0; x < this->ocean_diplomatic_map_image.width(); ++x) {
		for (int y = 0; y < this->ocean_diplomatic_map_image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			const QColor pixel_color = this->ocean_diplomatic_map_image.pixelColor(pixel_pos);

			if (pixel_color.alpha() == 0) {
				continue;
			}

			if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (this->ocean_diplomatic_map_image.width() - 1) || pixel_pos.y() == (this->ocean_diplomatic_map_image.height() - 1)) {
				continue;
			}

			if (pixel_color != color) {
				//blended color
				border_pixels.push_back(pixel_pos);
				continue;
			}

			const QPoint north_pos = pixel_pos + QPoint(0, -1);
			const QPoint east_pos = pixel_pos + QPoint(1, 0);
			const bool is_border_pixel = this->ocean_diplomatic_map_image.pixelColor(north_pos).alpha() == 0 || this->ocean_diplomatic_map_image.pixelColor(east_pos).alpha() == 0;

			if (is_border_pixel) {
				border_pixels.push_back(pixel_pos);
			}
		}
	}

	const QColor &border_pixel_color = defines::get()->get_country_border_color();

	for (const QPoint &border_pixel_pos : border_pixels) {
		this->ocean_diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
	}
}

void map::create_minimap_image()
{
	this->minimap_image = QImage(this->get_size() * this->get_minimap_tile_scale(), QImage::Format_RGBA8888);
	this->minimap_image.fill(Qt::transparent);

	this->update_minimap_rect(QRect(QPoint(0, 0), this->get_size()));
}

void map::update_minimap_rect(const QRect &tile_rect)
{
	static const decimillesimal_int &minimap_tile_scale = this->get_minimap_tile_scale();

	const int start_x = (tile_rect.x() * minimap_tile_scale).to_int();
	const int start_y = (tile_rect.y() * minimap_tile_scale).to_int();

	const QSize minimap_size = this->minimap_image.size();
	const int end_x = std::min(minimap_size.width() - 1, (tile_rect.right() * minimap_tile_scale).to_int());
	const int end_y = std::min(minimap_size.height() - 1, (tile_rect.bottom() * minimap_tile_scale).to_int());

	for (int x = start_x; x <= end_x; ++x) {
		for (int y = start_y; y <= end_y; ++y) {
			const QPoint pixel_pos(x, y);
			const QPoint tile_pos = pixel_pos / minimap_tile_scale;

			if (game::get()->get_player_country()->get_game_data()->is_tile_explored(tile_pos)) {
				const tile *tile = this->get_tile(tile_pos);

				if (tile->get_province() != nullptr && tile->get_province()->is_water_zone()) {
					this->minimap_image.setPixelColor(pixel_pos, defines::get()->get_minimap_ocean_color());
					continue;
				}

				const domain *domain = tile->get_owner();

				if (domain != nullptr) {
					this->minimap_image.setPixelColor(pixel_pos, domain->get_game_data()->get_diplomatic_map_color());
					continue;
				} else if (tile->get_province() != nullptr && !tile->get_province()->is_water_zone()) {
					this->minimap_image.setPixelColor(pixel_pos, defines::get()->get_minor_nation_color());
					continue;
				}
			}

			this->minimap_image.setPixelColor(pixel_pos, defines::get()->get_unexplored_terrain()->get_color());
		}
	}
}

}
