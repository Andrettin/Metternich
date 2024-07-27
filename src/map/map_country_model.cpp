#include "metternich.h"

#include "map/map_country_model.h"

#include "country/country.h"
#include "game/game.h"
#include "map/map_province_model.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "util/exception_util.h"
#include "util/geopolygon_util.h"

#include <QGeoPolygon>

namespace metternich {

void map_country_model::reset_model()
{
	this->beginResetModel();

	country_map<std::vector<const QGeoPolygon *>> country_province_geopolygons;

	for (const auto &province_geopolygon_data : this->map_province_model->get_province_geopolygons()) {
		const province *province = province_geopolygon_data.first;
		const QGeoPolygon *geopolygon = static_cast<const QGeoPolygon *>(province_geopolygon_data.second.get());

		if (province->get_game_data()->get_owner() == nullptr) {
			continue;
		}

		country_province_geopolygons[province->get_game_data()->get_owner()].push_back(geopolygon);
	}

	for (const country *country : game::get()->get_countries()) {
		this->create_country_geopolygons(country, country_province_geopolygons[country]);
	}

	this->endResetModel();
}

void map_country_model::create_country_geopolygons(const country *country, const std::vector<const QGeoPolygon *> &country_province_geopolygons)
{
	std::vector<QPolygonF> country_polygons;

	for (const QGeoPolygon *province_geopolygon : country_province_geopolygons) {
		QPolygonF province_polygon = geopolygon::to_polygon(*province_geopolygon);
		country_polygons.push_back(std::move(province_polygon));
	}

	bool country_polygons_changed = true;
	while (country_polygons_changed) {
		country_polygons_changed = false;

		for (size_t i = 0; i < country_polygons.size(); ++i) {
			for (size_t j = i + 1; j < country_polygons.size();) {
				QPolygonF &country_polygon = country_polygons[i];
				QPolygonF &other_country_polygon = country_polygons[j];

				if (country_polygon.intersects(other_country_polygon)) {
					country_polygon = country_polygon.united(other_country_polygon);
					country_polygons.erase(country_polygons.begin() + j);
					country_polygons_changed = true;
				} else {
					++j;
				}
			}
		}
	}

	std::vector<std::unique_ptr<QGeoPolygon>> country_geopolygons;
	for (const QPolygonF &country_polygon : country_polygons) {
		country_geopolygons.push_back(geopolygon::from_polygon(country_polygon));
	}

	this->country_geopolygons[country] = std::move(country_geopolygons);
}

int map_country_model::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	int row_count = 0;

	for (const auto &[country, geopolygons] : this->country_geopolygons) {
		row_count += static_cast<int>(geopolygons.size());
	}

	return row_count;
}

QVariant map_country_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const map_country_model::role model_role = static_cast<map_country_model::role>(role);
		const int geopolygon_index = index.row();
		const auto geopolygon_data = this->get_geopolygon_data(geopolygon_index);

		switch (model_role) {
			case role::country:
				return QVariant::fromValue(geopolygon_data.first);
			case role::geopolygon:
				return QVariant::fromValue(*geopolygon_data.second);
			default:
				throw std::runtime_error(std::format("Invalid map province model role: {}.", role));
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

std::pair<const country *, const QGeoPolygon *> map_country_model::get_geopolygon_data(const int index) const
{
	int i = 0;

	for (const auto &[country, geopolygons] : this->country_geopolygons) {
		const int geopolygons_size = static_cast<int>(geopolygons.size());
		if (index >= (i + geopolygons_size)) {
			i += geopolygons_size;
			continue;
		}

		return std::pair<const metternich::country *, const QGeoPolygon *>(country, geopolygons.at(index - i).get());
	}

	throw std::runtime_error(std::format("Geopolygon data not found for index {}.", index));
}

}
