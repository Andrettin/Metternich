#pragma once

#pragma warning(push, 0)
#include <QAbstractItemModel> 
#pragma warning(pop)

namespace metternich {

class terrain_type;

class map_grid_model : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum class role {
		base_image_source = Qt::UserRole,
		image_source,
		underlay_image_sources,
		overlay_image_sources,
		site,
		province,
		terrain,
		resource,
		improvement,
		civilian_unit,
		upper_label
	};

	static QString build_image_source(const terrain_type *terrain, const short tile_frame);

	map_grid_model();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;

	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override final
	{
		Q_UNUSED(parent);

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

		role_names.insert(static_cast<int>(role::base_image_source), "base_image_source");
		role_names.insert(static_cast<int>(role::image_source), "image_source");
		role_names.insert(static_cast<int>(role::underlay_image_sources), "underlay_image_sources");
		role_names.insert(static_cast<int>(role::overlay_image_sources), "overlay_image_sources");
		role_names.insert(static_cast<int>(role::site), "site");
		role_names.insert(static_cast<int>(role::province), "province");
		role_names.insert(static_cast<int>(role::terrain), "terrain");
		role_names.insert(static_cast<int>(role::resource), "resource");
		role_names.insert(static_cast<int>(role::improvement), "improvement");
		role_names.insert(static_cast<int>(role::civilian_unit), "civilian_unit");
		role_names.insert(static_cast<int>(role::upper_label), "upper_label");

		return role_names;
	}

	void on_tile_terrain_changed(const QPoint &tile_pos);
	void on_tile_improvement_changed(const QPoint &tile_pos);
	void on_tile_civilian_unit_changed(const QPoint &tile_pos);
};

}
