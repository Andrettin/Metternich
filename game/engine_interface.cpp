#include "game/engine_interface.h"

#include "database/defines.h"
#include "game/game.h"
#include "holding/holding.h"
#include "landed_title/landed_title.h"
#include "landed_title/landed_title_tier.h"
#include "map/map.h"
#include "map/province.h"
#include "map/star_system.h"
#include "map/world.h"
#include "script/event/event_instance.h"
#include "technology/technology.h"
#include "technology/technology_area.h"
#include "util/container_util.h"

#include <QList>

namespace metternich {

engine_interface::engine_interface()
{
	connect(game::get(), &game::paused_changed, this, &engine_interface::paused_changed);
}

engine_interface::~engine_interface()
{
}

game *engine_interface::get_game() const
{
	return game::get();
}

QVariantList engine_interface::get_technologies() const
{
	return container::to_qvariant_list(technology::get_all_sorted());
}

QVariantList engine_interface::get_technology_areas() const
{
	return container::to_qvariant_list(technology_area::get_all_sorted());
}

QVariantList engine_interface::get_star_systems() const
{
	QVariantList star_system_list = container::to_qvariant_list(star_system::get_all());
	return star_system_list;
}

QVariantList engine_interface::get_worlds() const
{
	QVariantList world_list = container::to_qvariant_list(world::get_all());
	return world_list;
}

QVariantList engine_interface::get_map_worlds() const
{
	QVariantList world_list = container::to_qvariant_list(world::get_map_worlds());
	return world_list;
}

QVariantList engine_interface::get_cosmic_duchies() const
{
	QVariantList title_list;

	const std::vector<landed_title *> &duchies = landed_title::get_tier_titles(landed_title_tier::duchy);

	for (landed_title *duchy : duchies) {
		if (duchy->get_star_system() != nullptr) {
			title_list.append(QVariant::fromValue(duchy));
		}
	}

	return title_list;
}

province *engine_interface::get_selected_province() const
{
	return province::get_selected_province();
}

holding *engine_interface::get_selected_holding() const
{
	return holding::get_selected_holding();
}

const QRectF &engine_interface::get_cosmic_map_bounding_rect() const
{
	return map::get()->get_cosmic_map_bounding_rect();
}

bool engine_interface::is_paused() const
{
	return game::get()->is_paused();
}

void engine_interface::set_paused(const bool paused)
{
	game::get()->post_order([paused]() {
		game::get()->set_paused(paused);
	});
}

int engine_interface::get_map_mode() const
{
	return static_cast<int>(map::get()->get_mode());
}

void engine_interface::set_map_mode(const int map_mode)
{
	game::get()->post_order([map_mode]() {
		map::get()->set_mode(static_cast<metternich::map_mode>(map_mode));
	});
}

QVariantList engine_interface::get_event_instances() const
{
	std::shared_lock<std::shared_mutex> lock(this->event_instances_mutex);
	
	std::vector<event_instance *> event_instances;
	
	for (const qunique_ptr<event_instance> &event_instance : this->event_instances) {
		event_instances.push_back(event_instance.get());
	}
	
	return container::to_qvariant_list(event_instances);
}

void engine_interface::add_event_instance(qunique_ptr<event_instance> &&event_instance)
{
	{
		std::unique_lock<std::shared_mutex> lock(this->event_instances_mutex);

		if (this->event_instances.empty()) {
			game::get()->post_order([]() {
				game::get()->set_paused(true);
			});
		}

		this->event_instances.push_back(std::move(event_instance));
	}

	emit event_instances_changed();
}

void engine_interface::remove_event_instance(const QVariant &event_instance_variant)
{
	{
		std::unique_lock<std::shared_mutex> lock(this->event_instances_mutex);

		QObject *object = qvariant_cast<QObject *>(event_instance_variant);
		event_instance *event_instance_to_remove = static_cast<event_instance *>(object);

		for (size_t i = 0; i < this->event_instances.size(); ++i) {
			event_instance *event_instance = this->event_instances[i].get();
			if (event_instance == event_instance_to_remove) {
				this->event_instances.erase(this->event_instances.begin() + static_cast<int>(i));
				break;
			}
		}

		if (this->event_instances.empty()) {
			game::get()->post_order([]() {
				game::get()->set_paused(false);
			});
		}
	}

	emit event_instances_changed();
}

}
