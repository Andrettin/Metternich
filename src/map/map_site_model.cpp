#include "metternich.h"

#include "map/map_site_model.h"

#include "map/site.h"
#include "map/site_game_data.h"
#include "util/exception_util.h"

namespace metternich {

map_site_model::map_site_model()
{
	for (const site *site : site::get_all()) {
		const site_game_data *site_game_data = site->get_game_data();

		if (!site_game_data->is_on_map()) {
			continue;
		}

		if (!site->get_geocoordinate().is_valid() || site->get_geocoordinate().is_null()) {
			continue;
		}

		this->site_geocoordinates.emplace_back(site, site->get_geocoordinate().to_qgeocoordinate());
	}
}

int map_site_model::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return static_cast<int>(this->site_geocoordinates.size());
}

QVariant map_site_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const map_site_model::role model_role = static_cast<map_site_model::role>(role);
		const int site_data_index = index.row();
		const auto &site_data = this->site_geocoordinates.at(site_data_index);

		switch (model_role) {
			case role::site:
				return QVariant::fromValue(site_data.first);
			case role::geocoordinate:
				return QVariant::fromValue(site_data.second);
			default:
				throw std::runtime_error(std::format("Invalid map site model role: {}.", role));
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

}
