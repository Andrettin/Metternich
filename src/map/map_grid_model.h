#pragma once

#pragma warning(push, 0)
#include <QAbstractItemModel> 
#pragma warning(pop)

namespace metternich {

class province;
class route;
class site;

struct map_block_data final
{
	std::vector<const province *> provinces;
	std::vector<const site *> sites;
	std::vector<const route *> routes;
};

class map_grid_model : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum class role {
		provinces = Qt::UserRole,
		sites,
		routes,
		map_block_start_x,
		map_block_start_y,
		map_block_width,
		map_block_height
	};

	map_grid_model();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;

	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override final
	{
		Q_UNUSED(parent);

		if (!this->hasIndex(row, column, parent)) {
			return QModelIndex();
		}

		return this->createIndex(row, column);
	}

	virtual QModelIndex parent(const QModelIndex &index) const override final
	{
		Q_UNUSED(index);
		
		return QModelIndex();
	}

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names;

		role_names.insert(static_cast<int>(role::provinces), "provinces");
		role_names.insert(static_cast<int>(role::sites), "sites");
		role_names.insert(static_cast<int>(role::map_block_start_x), "map_block_start_x");
		role_names.insert(static_cast<int>(role::map_block_start_y), "map_block_start_y");
		role_names.insert(static_cast<int>(role::map_block_width), "map_block_width");
		role_names.insert(static_cast<int>(role::map_block_height), "map_block_height");

		return role_names;
	}

private:
	std::vector<map_block_data> map_block_data;
};

}
