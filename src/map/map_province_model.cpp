#include "metternich.h"

#include "map/map_province_model.h"

#include "game/game.h"
#include "game/scenario.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/world.h"
#include "util/exception_util.h"
#include "util/geopolygon_util.h"

#include <QGeoPolygon>

namespace metternich {

map_province_model::map_province_model()
{
	auto province_geodata_map = game::get()->get_scenario()->get_map_template()->get_world()->parse_provinces_geojson_folder();

	for (auto &[province, geoshapes] : province_geodata_map) {
		for (auto &geoshape : geoshapes) {
			geopolygon::simplify(*static_cast<QGeoPolygon *>(geoshape.get()));

			this->province_geopolygons.emplace_back(province, std::move(geoshape));
		}
	}
}

int map_province_model::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return static_cast<int>(this->province_geopolygons.size());
}

QVariant map_province_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const map_province_model::role model_role = static_cast<map_province_model::role>(role);
		const int geopolygon_index = index.row();
		const auto &geopolygon_data = this->province_geopolygons.at(geopolygon_index);

		switch (model_role) {
			case role::province:
				return QVariant::fromValue(geopolygon_data.first);
			case role::geopolygon:
				return QVariant::fromValue(*static_cast<const QGeoPolygon *>(geopolygon_data.second.get()));
			default:
				throw std::runtime_error(std::format("Invalid map province model role: {}.", role));
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

}
