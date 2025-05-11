#include "metternich.h"

#include "game/scenario_model.h"

#include "game/scenario.h"
#include "util/date_util.h"
#include "util/exception_util.h"

namespace metternich {

scenario_model::scenario_model()
{
}

int scenario_model::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return static_cast<int>(reinterpret_cast<const metternich::scenario *>(parent.constInternalPointer())->get_child_scenarios().size());
	} else {
		return static_cast<int>(scenario::get_top_level_scenarios().size());
	}
}

int scenario_model::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return 1;
}

QVariant scenario_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const metternich::scenario *scenario = reinterpret_cast<const metternich::scenario *>(index.constInternalPointer());

		switch (role) {
			case Qt::DisplayRole:
				return QString::fromStdString(std::format("{}, {}", scenario->get_name(), date::year_to_labeled_string(scenario->get_start_year())));
			case role::scenario:
				return QVariant::fromValue(scenario);
			default:
				throw std::runtime_error(std::format("Invalid scenario model role: {}.", role));
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

QModelIndex scenario_model::index(const int row, const int column, const QModelIndex &parent) const
{
	if (!this->hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	if (parent.isValid()) {
		return this->createIndex(row, column, reinterpret_cast<const metternich::scenario *>(parent.constInternalPointer())->get_child_scenarios().at(row));
	} else {
		return this->createIndex(row, column, scenario::get_top_level_scenarios().at(row));
	}
}

QModelIndex scenario_model::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}

	const metternich::scenario *scenario = reinterpret_cast<const metternich::scenario *>(index.constInternalPointer());

	const std::vector<const metternich::scenario *> *parent_siblings = nullptr;

	const metternich::scenario *parent_scenario = scenario->get_parent_scenario();
	if (parent_scenario == nullptr) {
		return QModelIndex();
	}

	const metternich::scenario *grandparent_scenario = parent_scenario->get_parent_scenario();
	if (grandparent_scenario != nullptr) {
		parent_siblings = &grandparent_scenario->get_child_scenarios();
	} else {
		parent_siblings = &scenario::get_top_level_scenarios();
	}

	for (size_t i = 0; i < parent_siblings->size(); ++i) {
		if (parent_siblings->at(i) == parent_scenario) {
			return this->createIndex(static_cast<int>(i), 0, parent_scenario);
		}
	}

	return QModelIndex();
}

}
