#include "metternich.h"

#include "map/map_template.h"

#include "database/database.h"
#include "database/defines.h"
#include "economy/resource.h"
#include "map/map.h"
#include "map/map_generator.h"
#include "map/map_projection.h"
#include "map/province.h"
#include "map/province_map_data.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/world.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/geoshape_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/point_util.h"
#include "util/rect_util.h"
#include "util/set_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

#include <QElapsedTimer>
#include <QImageReader>

namespace metternich {

const std::set<std::string> map_template::database_dependencies = {
	//map templates must be initialized after sites, as sites add themselves to the world site list in their initialization function, and during map template initialization the sites are then added to the map template's site position map
	site::class_identifier,

	//map templates must also be initialized after provinces, as they set their settlements to have them as their provinces, which is needed for site position initialization
	province::class_identifier
};

bool map_template::is_site_in_province(const site *site, const province *province, const province_geodata_map_type &province_geodata_map)
{
	const auto find_iterator = province_geodata_map.find(province);

	if (find_iterator == province_geodata_map.end()) {
		return false;
	}

	const std::vector<std::unique_ptr<QGeoShape>> &province_geoshapes = find_iterator->second;

	const QGeoCoordinate qgeocoordinate = site->get_geocoordinate().to_qgeocoordinate();

	for (const std::unique_ptr<QGeoShape> &geoshape : province_geoshapes) {
		if (geoshape->contains(qgeocoordinate)) {
			return true;
		}
	}

	return false;
}

void map_template::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "ignored_provinces") {
		for (const std::string &value : values) {
			this->ignored_provinces.insert(province::get(value));
		}
	} else if (tag == "ignored_regions") {
		for (const std::string &value : values) {
			this->ignored_regions.insert(region::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void map_template::initialize()
{
	if (this->update_province_image) {
		this->write_province_image();
	}

	if (!this->get_province_image_filepath().empty()) {
		const QRect map_rect(QPoint(0, 0), this->get_size());

		const QImage province_image = QImage(path::to_qstring(this->get_province_image_filepath()));

		for (const site *site : this->get_world()->get_sites()) {
			QPoint tile_pos = site->get_pos();

			if (tile_pos == point::invalid_point) {
				assert_throw(site->get_geocoordinate().is_valid());

				if (!site->get_geocoordinate().is_null() && this->get_georectangle().contains(site->get_geocoordinate())) {
					tile_pos = this->get_geocoordinate_pos(site->get_geocoordinate()) + site->get_pos_offset();
				}
			}

			if (tile_pos == point::invalid_point) {
				continue;
			}

			if (tile_pos.y() < 0) {
				tile_pos.setY(this->get_size().height() - 1 + tile_pos.y());
			}

			if (!map_rect.contains(tile_pos)) {
				continue;
			}

			const province *tile_province = province::try_get_by_color(province_image.pixelColor(tile_pos));
			const province *site_province = tile_province;

			if (site->get_province() != nullptr) {
				site_province = site->get_province();
			}

			if (site_province == nullptr) {
				continue;
			}

			if (this->is_province_ignored(site_province)) {
				continue;
			}

			if (site->get_type() == site_type::none || site->get_type() == site_type::resource) {
				continue;
			}

			assert_throw(!site_province->is_water_zone());

			//if the site is not placed in its province, nudge its position to be in the nearest point in its province; also nudge sites if they are too close to other sites
			const bool is_pos_valid = (site_province == tile_province || province_image.isNull()) && this->is_pos_available_for_site(tile_pos, site_province, province_image);
			if (!is_pos_valid) {
				bool found_pos = false;
				int64_t best_distance = std::numeric_limits<int64_t>::max();
				QPoint best_tile_pos = tile_pos;

				static constexpr int max_range = 16;
				for (int i = 1; i <= max_range; ++i) {
					const QRect rect(tile_pos - QPoint(i, i), tile_pos + QPoint(i, i));

					bool checked_on_map = false;

					rect::for_each_edge_point(rect, [&](const QPoint &checked_pos) {
						if (!map_rect.contains(checked_pos)) {
							return;
						}

						checked_on_map = true;

						const province *checked_province = province::try_get_by_color(province_image.pixelColor(checked_pos));
						if (checked_province != site_province) {
							return;
						}

						if (!this->is_pos_available_for_site(checked_pos, site_province, province_image)) {
							return;
						}

						const int64_t distance = point::square_distance_to(tile_pos, checked_pos);
						if (distance < best_distance) {
							best_distance = distance;
							best_tile_pos = checked_pos;
							found_pos = true;
						}
					});

					if (found_pos) {
						break;
					}

					if (!checked_on_map) {
						break;
					}
				}

				if (!found_pos) {
					throw std::runtime_error(std::format("No position found for site \"{}\" in province \"{}\".", site->get_identifier(), site_province->get_identifier()));
				}

				tile_pos = best_tile_pos;
			}

			assert_throw(map_rect.contains(tile_pos));

			if (this->sites_by_position.contains(tile_pos)) {
				throw std::runtime_error(std::format("Both the sites of \"{}\" and \"{}\" occupy the {} position in map template \"{}\".", this->sites_by_position.find(tile_pos)->second->get_identifier(), site->get_identifier(), point::to_string(tile_pos), this->get_identifier()));
			}

			this->sites_by_position[tile_pos] = site;
		}
	}

	named_data_entry::initialize();
}

void map_template::check() const
{
	if (this->map_projection != nullptr) {
		this->map_projection->validate_area(this->get_georectangle(), this->get_size());
	}

	if (this->get_world() == nullptr && !this->is_universe()) {
		throw std::runtime_error(std::format("Map template \"{}\" has neither a world, nor is it a universe map template.", this->get_identifier()));
	}
}

QPoint map_template::get_geocoordinate_pos(const geocoordinate &geocoordinate) const
{
	return this->map_projection->geocoordinate_to_point(geocoordinate, this->get_georectangle(), this->get_size(), this->geocoordinate_x_offset);
}

geocoordinate map_template::get_pos_geocoordinate(const QPoint &pos) const
{
	return this->map_projection->point_to_geocoordinate(pos, this->get_georectangle(), this->get_size(), this->geocoordinate_x_offset);
}

void map_template::write_terrain_image()
{
	assert_throw(this->get_world() != nullptr);

	terrain_geodata_map terrain_geodata_map = this->get_world()->parse_terrain_geojson_folder();

	color_map<std::vector<std::unique_ptr<QGeoShape>>> geodata_map;

	for (auto &[terrain_variant, geoshapes] : terrain_geodata_map) {
		QColor color;

		if (std::holds_alternative<const terrain_feature *>(terrain_variant)) {
			const terrain_feature *terrain_feature = std::get<const metternich::terrain_feature *>(terrain_variant);

			if (terrain_feature->is_river()) {
				continue;
			}

			if (terrain_feature->is_hidden()) {
				continue;
			}

			color = terrain_feature->get_terrain_type()->get_color();
		} else {
			const terrain_type *terrain_type = std::get<const metternich::terrain_type *>(terrain_variant);
			color = terrain_type->get_color();
			assert_throw(color.isValid());
		}

		if (!color.isValid()) {
			throw std::runtime_error("Terrain variant has no valid color.");
		}

		vector::merge(geodata_map[color], std::move(geoshapes));
	}

	assert_throw(this->map_projection != nullptr);

	this->map_projection->validate_area(this->get_georectangle(), this->get_size());

	QImage base_image = QImage(this->get_size(), QImage::Format_RGBA8888);
	base_image.fill(Qt::transparent);

	QImage province_image;

	if (!this->get_province_image_filepath().empty()) {
		//write water zones
		province_image = QImage(path::to_qstring(this->get_province_image_filepath()));

		assert_throw(province_image.size() == base_image.size());

		for (int x = 0; x < province_image.width(); ++x) {
			for (int y = 0; y < province_image.height(); ++y) {
				const QColor province_color = province_image.pixelColor(x, y);

				if (province_color.alpha() == 0) {
					//ignore transparent pixels
					continue;
				}

				if (base_image.pixelColor(x, y).alpha() != 0) {
					//ignore already-written pixels
					continue;
				}

				const province *province = province::get_by_color(province_color);

				if (!province->is_water_zone()) {
					continue;
				}

				const terrain_type *terrain = defines::get()->get_default_water_zone_terrain();
				assert_throw(terrain != nullptr);

				const QColor terrain_color = terrain->get_color();

				assert_throw(terrain_color.isValid());

				base_image.setPixelColor(x, y, terrain_color);
			}
		}
	}

	//write terrain geoshapes
	std::filesystem::path output_filepath = "terrain.png";

	geoshape::write_image(output_filepath, geodata_map, this->get_georectangle(), this->get_size(), this->map_projection, base_image, this->geocoordinate_x_offset);

	if (!this->get_province_image_filepath().empty()) {
		//write terrain based on provinces, for pixels without any terrain data set for them
		QImage output_image(path::to_qstring(output_filepath));

		assert_throw(!output_image.isNull());
		assert_throw(province_image.size() == output_image.size());

		for (int x = 0; x < province_image.width(); ++x) {
			for (int y = 0; y < province_image.height(); ++y) {
				const QColor province_color = province_image.pixelColor(x, y);

				if (province_color.alpha() == 0) {
					//ignore transparent pixels
					continue;
				}

				if (output_image.pixelColor(x, y).alpha() != 0) {
					//ignore already-written pixels
					continue;
				}

				const province *province = province::get_by_color(province_color);

				if (province->is_water_zone()) {
					continue;
				}

				const terrain_type *terrain = defines::get()->get_default_province_terrain();

				assert_throw(terrain != nullptr);

				const QColor terrain_color = terrain->get_color();

				assert_throw(terrain_color.isValid());

				output_image.setPixelColor(x, y, terrain_color);
			}
		}

		output_image.save(path::to_qstring(output_filepath));
	}
}

void map_template::set_province_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_province_image_filepath()) {
		return;
	}

	this->province_image_filepath = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::write_province_image()
{
	try {
		assert_throw(this->get_world() != nullptr);

		province_geodata_map_type province_geodata_map = this->get_world()->parse_provinces_geojson_folder();

		color_map<std::vector<std::unique_ptr<QGeoShape>>> geodata_map;

		for (auto &[province, geoshapes] : province_geodata_map) {
			const QColor color = province->get_color();
			if (!color.isValid()) {
				throw std::runtime_error("Province \"" + province->get_identifier() + "\" has no valid color.");
			}

			vector::merge(geodata_map[color], std::move(geoshapes));
		}

		assert_throw(this->map_projection != nullptr);

		this->map_projection->validate_area(this->get_georectangle(), this->get_size());

		QImage base_image;

		if (!this->get_province_image_filepath().empty()) {
			base_image = QImage(path::to_qstring(this->get_province_image_filepath()));
			assert_throw(!base_image.isNull());
		}

		std::filesystem::path output_filepath = this->get_province_image_filepath();
		if (output_filepath.empty()) {
			output_filepath = "provinces.png";
		}

		QElapsedTimer elapsed_timer;
		elapsed_timer.start();

		geoshape::write_image(output_filepath, geodata_map, this->get_georectangle(), this->get_size(), this->map_projection, base_image, this->geocoordinate_x_offset);

		const std::chrono::milliseconds elapsed_ms(elapsed_timer.elapsed());
		log_info(std::format("Wrote province image for map template \"{}\" in {} minutes and {} seconds.", this->get_identifier(), std::chrono::duration_cast<std::chrono::minutes>(elapsed_ms).count(), (elapsed_ms.count() / 1000) % 60));
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
	}
}

QCoro::Task<void> map_template::apply() const
{
	map *map = map::get();
	map->clear();
	map->set_size(this->get_size());
	map->create_tiles();

	if (this->get_minimap_tile_scale() != 0) {
		map->set_minimap_tile_scale(this->get_minimap_tile_scale());
	} else {
		map->set_minimap_tile_scale(defines::get()->get_default_minimap_tile_scale());
	}

	if (this->get_diplomatic_map_tile_scale() != 0) {
		map->set_diplomatic_map_tile_scale(this->get_diplomatic_map_tile_scale());
	} else {
		map->set_diplomatic_map_tile_scale(defines::get()->get_default_diplomatic_map_tile_scale());
	}

	if (this->is_randomly_generated()) {
		map_generator map_generator(this);
		map_generator.generate();
	} else {
		co_await this->apply_provinces();
	}

	this->generate_additional_sites();
}

QCoro::Task<void> map_template::apply_provinces() const
{
	QImage province_image;

	if (this->get_main_map_template() != nullptr) {
		const QRect map_rect(this->get_map_start_pos(), this->get_size());

		QImageReader image_reader(path::to_qstring(this->get_main_map_template()->get_province_image_filepath()));
		image_reader.setClipRect(map_rect);

		province_image = image_reader.read();
	} else {
		assert_throw(!this->get_province_image_filepath().empty());

		province_image = QImage(path::to_qstring(this->get_province_image_filepath()));
	}

	if (province_image.format() != QImage::Format_ARGB32) {
		province_image = province_image.convertToFormat(QImage::Format_ARGB32);
	}

	map *map = map::get();

	const std::span<const QRgb> province_image_data{ reinterpret_cast<const QRgb *>(province_image.constBits()), static_cast<size_t>(province_image.sizeInBytes() / 4)};

	std::vector<QFuture<std::vector<std::pair<QPoint, province *>>>> futures;
	for (int y = 0; y < map->get_height(); ++y) {
		QFuture<std::vector<std::pair<QPoint, province *>>> future = QtConcurrent::run([this, y, province_image_data]() -> std::vector<std::pair<QPoint, province *>> {
			return this->apply_provinces_for_map_line(y, province_image_data);
		});
		futures.push_back(std::move(future));
	}

	for (QFuture<std::vector<std::pair<QPoint, province *>>> &future : futures) {
		const std::vector<std::pair<QPoint, province *>> tile_province_pairs = co_await qCoro(future).takeResult();

		for (const auto &[tile_pos, province] : tile_province_pairs) {
			map->set_tile_province(tile_pos, province);
		}
	}

	for (const province *province : province::get_all()) {
		if (!province->get_map_data()->is_on_map()) {
			continue;
		}

		province->get_map_data()->initialize_terrain();
	}

	//apply tile sites
	if (this->get_main_map_template() != nullptr) {
		const QRect map_rect(this->get_map_start_pos(), this->get_size());

		for (const auto &[tile_pos, site] : this->get_main_map_template()->get_sites_by_position()) {
			if (!map_rect.contains(tile_pos)) {
				continue;
			}

			if (site->get_province() != nullptr && this->is_province_ignored(site->get_province())) {
				//skip sites whose provinces are ignored
				continue;
			}

			map->set_tile_site(tile_pos - map_rect.topLeft(), site);
		}
	} else {
		for (const auto &[tile_pos, site] : this->get_sites_by_position()) {
			if (site->get_province() != nullptr && this->is_province_ignored(site->get_province())) {
				//skip sites whose provinces are ignored
				continue;
			}

			map->set_tile_site(tile_pos, site);
		}
	}
}

std::vector<std::pair<QPoint, province *>> map_template::apply_provinces_for_map_line(const int y, const std::span<const QRgb> &province_image_data) const
{
	map *map = map::get();

	std::vector<std::pair<QPoint, province *>> tile_province_pairs;
	tile_province_pairs.reserve(map->get_width());

	for (int x = 0; x < map->get_width(); ++x) {
		const QPoint tile_pos(x, y);
		const size_t tile_index = static_cast<size_t>(y) * static_cast<size_t>(map->get_width()) + static_cast<size_t>(x);
		const QColor tile_color = QColor::fromRgba(province_image_data[tile_index]);

		province *province = nullptr;
		if (tile_color.alpha() != 0) {
			province = province::get_by_color(tile_color);

			if (this->is_province_ignored(province)) {
				continue;
			}
		}

		if (province == nullptr) {
			continue;
		}

		tile_province_pairs.emplace_back(tile_pos, province);
	}

	return tile_province_pairs;
}

void map_template::generate_additional_sites() const
{
	if (this->get_world() == nullptr) {
		return;
	}

	//generate sites which belong to other worlds, but can be generated on this one
	std::vector<const site *> potential_sites;
	for (const site *site : site::get_all()) {
		if (site->get_map_data()->is_on_map()) {
			continue;
		}

		if (site->get_world() == this->get_world()) {
			continue;
		}

		if (!site->can_be_generated_on_world(this->get_world())) {
			continue;
		}

		potential_sites.push_back(site);
	}

	vector::shuffle(potential_sites);

	for (const site *site : potential_sites) {
		this->generate_site(site);
	}
}

void map_template::generate_site(const site *site) const
{
	std::set<const province *> province_set;

	set::merge(province_set, site->get_generation_provinces());

	for (const region *region : site->get_generation_regions()) {
		set::merge(province_set, region->get_provinces());
	}

	std::vector<const province *> potential_provinces = container::to_vector(province_set);
	vector::shuffle(potential_provinces);

	map *map = map::get();

	for (const province *province : potential_provinces) {
		const province_map_data *province_map_data = province->get_map_data();
		if (!province_map_data->is_on_map()) {
			continue;
		}

		if (province_map_data->get_settlement_sites().size() >= province::max_holdings) {
			continue;
		}

		const resource *resource = site->get_map_data()->get_resource();
		const bool is_near_water = resource != nullptr && resource->is_near_water();
		const bool is_coastal = resource != nullptr && resource->is_coastal();
		const std::vector<const terrain_type *> &site_terrains = site->get_terrain_types();

		const std::vector<QPoint> province_tiles = vector::shuffled(province_map_data->get_tiles());

		if (!site_terrains.empty()) {
			bool has_terrain = false;
			for (const terrain_type *terrain : site_terrains) {
				if (province_map_data->get_terrain() == terrain) {
					has_terrain = true;
					break;
				}
			}
			if (!has_terrain) {
				continue;
			}
		}

		if (is_coastal) {
			if (!province_map_data->is_coastal()) {
				continue;
			}
		} else if (is_near_water) {
			if (!province_map_data->is_near_water()) {
				continue;
			}
		}

		for (const QPoint &tile_pos : province_tiles) {
			const tile *tile = map->get_tile(tile_pos);
			if (tile->get_site() != nullptr) {
				continue;
			}

			if (!this->is_pos_available_for_site_generation(tile_pos, province)) {
				continue;
			}

			map->set_tile_site(tile_pos, site);
			return;
		}
	}
}

bool map_template::is_pos_available_for_site(const QPoint &tile_pos, const province *site_province, const QImage &province_image) const
{
	const QRect map_rect(QPoint(0, 0), this->get_size());
	bool available = true;

	static constexpr int coast_check_range = 0;
	const QRect coast_check_rect(tile_pos - QPoint(coast_check_range, coast_check_range), tile_pos + QPoint(coast_check_range, coast_check_range));

	rect::for_each_point_until(coast_check_rect, [this, &map_rect, &available, site_province, &province_image](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			available = false;
			return true;
		}

		if (!province_image.isNull()) {
			const province *tile_province = province::try_get_by_color(province_image.pixelColor(rect_pos));
			if (tile_province == nullptr || (tile_province->is_water_zone() && site_province != tile_province)) {
				available = false;
				return true;
			}
		}

		return false;
	});

	static const int province_check_range = 4 / defines::get()->get_province_map_tile_scale();
	const QRect province_check_rect(tile_pos - QPoint(province_check_range, province_check_range), tile_pos + QPoint(province_check_range, province_check_range));

	rect::for_each_point_until(province_check_rect, [this, &map_rect, &available, site_province, &province_image](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			available = false;
			return true;
		}

		if (!province_image.isNull()) {
			const province *tile_province = province::try_get_by_color(province_image.pixelColor(rect_pos));
			if (tile_province != nullptr && !tile_province->is_water_zone() && site_province != tile_province) {
				available = false;
				return true;
			}
		}

		return false;
	});

	if (!available) {
		return false;
	}

	static const int site_check_range = 8 / defines::get()->get_province_map_tile_scale();
	const QRect site_check_rect(tile_pos - QPoint(site_check_range, site_check_range), tile_pos + QPoint(site_check_range, site_check_range));

	rect::for_each_point_until(site_check_rect, [this, &map_rect, &available, site_province](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			return false;
		}

		if (this->sites_by_position.contains(rect_pos)) {
			available = false;
			return true;
		}

		return false;
	});

	return available;
}

bool map_template::is_pos_available_for_site_generation(const QPoint &tile_pos, const province *site_province) const
{
	const QRect map_rect(QPoint(0, 0), map::get()->get_size());
	bool available = true;

	static constexpr int coast_check_range = 0;
	const QRect coast_check_rect(tile_pos - QPoint(coast_check_range, coast_check_range), tile_pos + QPoint(coast_check_range, coast_check_range));

	rect::for_each_point_until(coast_check_rect, [this, &map_rect, &available, site_province](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			available = false;
			return true;
		}

		const tile *tile = map::get()->get_tile(rect_pos);
		const province *tile_province = tile->get_province();
		if (tile_province == nullptr || (tile_province->is_water_zone() && site_province != tile_province)) {
			available = false;
			return true;
		}

		return false;
	});

	static const int province_check_range = 4 / defines::get()->get_province_map_tile_scale();
	const QRect province_check_rect(tile_pos - QPoint(province_check_range, province_check_range), tile_pos + QPoint(province_check_range, province_check_range));

	rect::for_each_point_until(province_check_rect, [this, &map_rect, &available, site_province](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			available = false;
			return true;
		}

		const tile *tile = map::get()->get_tile(rect_pos);
		const province *tile_province = tile->get_province();
		if (tile_province != nullptr && !tile_province->is_water_zone() && site_province != tile_province) {
			available = false;
			return true;
		}

		return false;
	});

	if (!available) {
		return false;
	}

	static const int site_check_range = 8 / defines::get()->get_province_map_tile_scale();
	const QRect site_check_rect(tile_pos - QPoint(site_check_range, site_check_range), tile_pos + QPoint(site_check_range, site_check_range));

	rect::for_each_point_until(site_check_rect, [this, &map_rect, &available, site_province](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			return false;
		}

		const tile *tile = map::get()->get_tile(rect_pos);
		if (tile->get_site() != nullptr) {
			available = false;
			return true;
		}

		return false;
	});

	return available;
}

bool map_template::is_province_ignored(const province *province) const
{
	if (this->ignored_provinces.contains(province)) {
		return true;
	}

	for (const region *region : province->get_regions()) {
		if (this->ignored_regions.contains(region)) {
			return true;
		}
	}

	return false;
}

}
