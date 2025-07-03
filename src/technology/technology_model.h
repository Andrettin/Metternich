#pragma once

#include "technology_container.h"

#pragma warning(push, 0)
#include <QAbstractItemModel> 
#pragma warning(pop)

namespace metternich {

class technology;

class technology_model : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum role {
		technology = Qt::UserRole
	};

	technology_model();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override final;
	virtual QModelIndex parent(const QModelIndex &index) const override final;

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names = QAbstractItemModel::roleNames();

		role_names.insert(static_cast<int>(role::technology), "technology");

		return role_names;
	}

	const std::vector<const metternich::technology *> &get_technology_children(const metternich::technology *technology) const
	{
		const auto find_iterator = this->technology_children.find(technology);
		if (find_iterator != this->technology_children.end()) {
			return find_iterator->second;
		}

		static const std::vector<const metternich::technology *> empty_vector;
		return empty_vector;
	}

private:
	std::vector<const metternich::technology *> top_level_technologies;
	technology_map<std::vector<const metternich::technology *>> technology_children;
};

}
