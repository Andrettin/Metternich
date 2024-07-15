#pragma once

#pragma warning(push, 0)
#include <QAbstractListModel> 
#pragma warning(pop)

namespace metternich {

class site;

class map_site_model : public QAbstractListModel
{
	Q_OBJECT

public:
	enum class role {
		site,
		geocoordinate
	};

	map_site_model();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names;

		role_names.insert(static_cast<int>(role::site), "site");
		role_names.insert(static_cast<int>(role::geocoordinate), "geocoordinate");

		return role_names;
	}

private:
	std::vector<std::pair<const site *, QGeoCoordinate>> site_geocoordinates;
};

}
