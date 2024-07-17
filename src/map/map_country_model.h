#pragma once

#include "country/country_container.h"

#pragma warning(push, 0)
#include <QAbstractListModel> 
#pragma warning(pop)

Q_MOC_INCLUDE("map/map_province_model.h")

namespace metternich {

class country;
class map_province_model;

class map_country_model : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(const metternich::map_province_model* map_province_model MEMBER map_province_model WRITE set_map_province_model)

public:
	enum class role {
		country,
		geopolygon
	};

	void reset_model();
	void create_country_geopolygons(const country *country, const std::vector<const QGeoPolygon *> &country_province_geopolygons);

	void set_map_province_model(const metternich::map_province_model *map_province_model)
	{
		if (map_province_model == this->map_province_model) {
			return;
		}

		this->map_province_model = map_province_model;
		this->reset_model();
	}

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names;

		role_names.insert(static_cast<int>(role::country), "country");
		role_names.insert(static_cast<int>(role::geopolygon), "geopolygon");

		return role_names;
	}

	std::pair<const country *, const QGeoPolygon *> get_geopolygon_data(const int index) const;

private:
	const map_province_model *map_province_model = nullptr;
	country_map<std::vector<std::unique_ptr<QGeoPolygon>>> country_geopolygons;
};

}
