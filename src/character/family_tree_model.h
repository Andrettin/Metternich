#pragma once

#include "character/character_container.h"

#pragma warning(push, 0)
#include <QAbstractItemModel> 
#pragma warning(pop)

namespace metternich {

class family_tree_model : public QAbstractItemModel
{
	Q_OBJECT

	Q_PROPERTY(const metternich::character *character READ get_character WRITE set_character NOTIFY character_changed)

public:
	enum class role {
		character = Qt::UserRole
	};

	family_tree_model();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override final;
	virtual QModelIndex parent(const QModelIndex &index) const override final;

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names = QAbstractItemModel::roleNames();

		role_names.insert(static_cast<int>(role::character), "character");

		return role_names;
	}

	const metternich::character *get_character() const
	{
		return this->character;
	}

	void set_character(const metternich::character *character);

	const metternich::character *get_character_family_tree_parent(const metternich::character *character) const;
	std::vector<const metternich::character *> get_character_family_tree_children(const metternich::character *character) const;

	Q_INVOKABLE QModelIndex get_character_model_index(const metternich::character *character) const;

signals:
	void character_changed();

private:
	const metternich::character *character = nullptr;
	const metternich::character *oldest_ancestor = nullptr;
	character_map<QModelIndex> character_indexes;
};

}
