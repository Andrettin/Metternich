#include "metternich.h"

#include "technology/technology_model.h"

#include "game/game.h"
#include "technology/technology.h"
#include "util/date_util.h"
#include "util/exception_util.h"

namespace metternich {

technology_model::technology_model()
{
	const auto erase_function = [](const metternich::technology *technology) {
		return !technology->is_available_for_country(game::get()->get_player_country());
	};

	this->top_level_technologies = technology::get_top_level_technologies();
	std::erase_if(this->top_level_technologies, erase_function);

	for (const metternich::technology *technology : technology::get_all()) {
		if (!technology->is_available_for_country(game::get()->get_player_country())) {
			continue;
		}

		std::vector<const metternich::technology *> technology_children = technology->get_child_technologies();
		std::erase_if(technology_children, erase_function);
		this->technology_children[technology] = std::move(technology_children);
	}
}

int technology_model::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return static_cast<int>(this->get_technology_children(reinterpret_cast<const metternich::technology *>(parent.constInternalPointer())).size());
	} else {
		return static_cast<int>(this->top_level_technologies.size());
	}
}

int technology_model::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return 1;
}

QVariant technology_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const metternich::technology *technology = reinterpret_cast<const metternich::technology *>(index.constInternalPointer());

		switch (role) {
			case Qt::DisplayRole:
				return QString::fromStdString(technology->get_name());
			case role::technology:
				return QVariant::fromValue(technology);
			default:
				throw std::runtime_error(std::format("Invalid technology model role: {}.", role));
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

QModelIndex technology_model::index(const int row, const int column, const QModelIndex &parent) const
{
	if (!this->hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	if (parent.isValid()) {
		return this->createIndex(row, column, this->get_technology_children(reinterpret_cast<const metternich::technology *>(parent.constInternalPointer())).at(row));
	} else {
		return this->createIndex(row, column, this->top_level_technologies.at(row));
	}
}

QModelIndex technology_model::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}

	const metternich::technology *technology = reinterpret_cast<const metternich::technology *>(index.constInternalPointer());

	const std::vector<const metternich::technology *> *parent_siblings = nullptr;

	const metternich::technology *parent_technology = technology->get_parent_technology();
	if (parent_technology == nullptr) {
		return QModelIndex();
	}

	const metternich::technology *grandparent_technology = parent_technology->get_parent_technology();
	if (grandparent_technology != nullptr) {
		parent_siblings = &this->get_technology_children(grandparent_technology);
	} else {
		parent_siblings = &this->top_level_technologies;
	}

	for (size_t i = 0; i < parent_siblings->size(); ++i) {
		if (parent_siblings->at(i) == parent_technology) {
			return this->createIndex(static_cast<int>(i), 0, parent_technology);
		}
	}

	return QModelIndex();
}

}
