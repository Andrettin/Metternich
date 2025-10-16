#pragma once

#pragma warning(push, 0)
#include <QAbstractItemModel> 
#pragma warning(pop)

namespace metternich {

class terrain_type;

class combat_map_grid_model : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum class role {
		base_image_sources = Qt::UserRole,
		image_sources,
		overlay_image_sources,
		terrain,
		character
	};

	static QString build_image_source(const terrain_type *terrain, const short tile_frame);

	combat_map_grid_model();

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

		role_names.insert(static_cast<int>(role::base_image_sources), "base_image_sources");
		role_names.insert(static_cast<int>(role::image_sources), "image_sources");
		role_names.insert(static_cast<int>(role::overlay_image_sources), "overlay_image_sources");
		role_names.insert(static_cast<int>(role::terrain), "terrain");
		role_names.insert(static_cast<int>(role::character), "character");

		return role_names;
	}
};

}
