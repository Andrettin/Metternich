#pragma once

#pragma warning(push, 0)
#include <QAbstractItemModel> 
#pragma warning(pop)

namespace metternich {

class scenario_model : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum role {
		scenario = Qt::UserRole
	};

	scenario_model();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override final;
	virtual QModelIndex parent(const QModelIndex &index) const override final;

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names = QAbstractItemModel::roleNames();

		role_names.insert(static_cast<int>(role::scenario), "scenario");

		return role_names;
	}
};

}
