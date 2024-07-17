#pragma once

#pragma warning(push, 0)
#include <QAbstractListModel> 
#pragma warning(pop)

namespace metternich {

class province;

class map_province_model : public QAbstractListModel
{
	Q_OBJECT

public:
	enum class role {
		province,
		geopolygon
	};

	map_province_model();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names;

		role_names.insert(static_cast<int>(role::province), "province");
		role_names.insert(static_cast<int>(role::geopolygon), "geopolygon");

		return role_names;
	}

	const std::vector<std::pair<const province *, std::unique_ptr<QGeoShape>>> &get_province_geopolygons() const
	{
		return this->province_geopolygons;
	}

private:
	std::vector<std::pair<const province *, std::unique_ptr<QGeoShape>>> province_geopolygons;
};

}
