#include "metternich.h"

#include "character/family_tree_model.h"

#include "character/character.h"
#include "util/assert_util.h"
#include "util/exception_util.h"

namespace metternich {

family_tree_model::family_tree_model()
{
}

int family_tree_model::rowCount(const QModelIndex &parent) const
{
	if (this->character == nullptr) {
		return 0;
	}

	if (parent.isValid()) {
		const metternich::character *parent_character = reinterpret_cast<const metternich::character *>(parent.constInternalPointer());
		assert_throw(parent_character != nullptr);

		return static_cast<int>(parent_character->get_children().size());
	} else {
		return 1;
	}
}

int family_tree_model::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return 1;
}

QVariant family_tree_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const metternich::character *character = reinterpret_cast<const metternich::character *>(index.constInternalPointer());

		switch (role) {
			case Qt::DisplayRole:
				return QString::fromStdString(character->get_full_name());
			case static_cast<int>(role::character):
				return QVariant::fromValue(character);
			default:
				throw std::runtime_error(std::format("Invalid family tree model role: {}.", role));
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

QModelIndex family_tree_model::index(const int row, const int column, const QModelIndex &parent) const
{
	if (!this->hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	if (parent.isValid()) {
		const metternich::character *parent_character = reinterpret_cast<const metternich::character *>(parent.constInternalPointer());
		assert_throw(parent_character != nullptr);

		const metternich::character *row_character = dynamic_cast<const metternich::character *>(parent_character->get_children().at(row));
		assert_throw(row_character != nullptr);
		return this->createIndex(row, column, row_character);
	} else {
		assert_throw(row == 0);
		assert_throw(this->oldest_ancestor != nullptr);
		return this->createIndex(row, column, this->oldest_ancestor);
	}
}

QModelIndex family_tree_model::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}

	const metternich::character *character = reinterpret_cast<const metternich::character *>(index.constInternalPointer());
	assert_throw(character != nullptr);

	std::vector<const metternich::character *> parent_siblings;

	const metternich::character *parent_character = this->get_character_family_tree_parent(character);
	if (parent_character == nullptr) {
		return QModelIndex();
	}

	const metternich::character *grandparent_character = this->get_character_family_tree_parent(parent_character);
	if (grandparent_character != nullptr) {
		for (const character_base *child : grandparent_character->get_children()) {
			parent_siblings.push_back(static_cast<const metternich::character *>(child));
		}
	} else {
		parent_siblings = { this->oldest_ancestor };
	}

	for (size_t i = 0; i < parent_siblings.size(); ++i) {
		if (parent_siblings.at(i) == parent_character) {
			return this->createIndex(static_cast<int>(i), 0, parent_character);
		}
	}

	return QModelIndex();
}

void family_tree_model::set_character(const metternich::character *character)
{
	this->beginResetModel();

	this->character = character;

	this->oldest_ancestor = this->character;

	if (this->character != nullptr) {
		while (this->get_character_family_tree_parent(this->oldest_ancestor) != nullptr) {
			this->oldest_ancestor = this->get_character_family_tree_parent(this->oldest_ancestor);
		}

		assert_throw(this->oldest_ancestor != nullptr);
	}

	this->endResetModel();

	emit character_changed();
}

const metternich::character *family_tree_model::get_character_family_tree_parent(const metternich::character *character) const
{
	assert_throw(character != nullptr);

	if (character->get_mother() != nullptr) {
		if (character->get_father() == nullptr || (character->get_dynasty() != nullptr && character->get_mother()->get_dynasty() == character->get_dynasty() && character->get_father()->get_dynasty() != character->get_dynasty())) {
			return character->get_mother();
		}
	}

	return character->get_father();
}

QModelIndex family_tree_model::get_character_model_index(const metternich::character *character) const
{
	assert_throw(character != nullptr);

	if (character == this->oldest_ancestor) {
		return this->createIndex(0, 0, character);
	}

	const metternich::character *parent_character = this->get_character_family_tree_parent(character);
	assert_throw(parent_character != nullptr);

	const std::vector<character_base *> siblings = parent_character->get_children();

	for (size_t i = 0; i < siblings.size(); ++i) {
		if (siblings.at(i) == character) {
			return this->createIndex(static_cast<int>(i), 0, character);
		}
	}

	assert_throw(false);
	return QModelIndex();
}

}
