#include "metternich.h"

#include "unit/army.h"

#include "character/party.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
#include "engine_interface.h"
#include "game/battle.h"
#include "game/domain_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "infrastructure/improvement.h"
#include "infrastructure/improvement_slot.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/context.h"
#include "ui/portrait.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace metternich {

army::army(const std::vector<military_unit *> &military_units, target_variant &&target)
	: military_units(military_units), target(std::move(target))
{
	assert_throw(!this->get_military_units().empty());

	this->domain = this->get_military_units().at(0)->get_country();

	for (military_unit *military_unit : this->get_military_units()) {
		military_unit->set_army(this);
	}

	std::visit([this](auto &&target_value) {
		using target_type = std::decay_t<decltype(target_value)>;

		static_assert(std::is_same_v<target_type, std::monostate> || std::is_same_v<target_type, const province *> || std::is_same_v<target_type, const site *>, "Invalid target variant type.");

		if constexpr (std::is_same_v<target_type, const province *>) {
			target_value->get_game_data()->add_entering_army(this);
		} else if constexpr (std::is_same_v<target_type, const site *>) {
			assert_throw(target_value->get_game_data()->can_be_visited_by(this->get_domain()));

			target_value->get_game_data()->add_visiting_army(this);
		}
	}, this->target);
}

army::~army()
{
	for (military_unit *military_unit : this->get_military_units()) {
		assert_throw(military_unit->get_army() == this);
		military_unit->set_army(nullptr);
	}

	std::visit([this](auto &&target_value) {
		using target_type = std::decay_t<decltype(target_value)>;

		static_assert(std::is_same_v<target_type, std::monostate> || std::is_same_v<target_type, const province *> || std::is_same_v<target_type, const site *>, "Invalid target variant type.");

		if constexpr (std::is_same_v<target_type, const province *>) {
			target_value->get_game_data()->remove_entering_army(this);
		} else if constexpr (std::is_same_v<target_type, const site *>) {
			target_value->get_game_data()->remove_visiting_army(this);
		}
	}, this->target);
}

QCoro::Task<void> army::do_turn()
{
	if (this->get_target_province() != nullptr) {
		const province *target_province = this->get_target_province();

		const std::vector<military_unit *> &province_military_units = target_province->get_game_data()->get_military_units();

		std::vector<military_unit *> garrison;
		for (military_unit *military_unit : province_military_units) {
			if (military_unit->is_hostile_to(this->get_domain())) {
				garrison.push_back(military_unit);
			}
		}

		bool success = true;
		const bool can_conquer_province = this->get_domain()->get_game_data()->can_attack(target_province->get_game_data()->get_owner());

		if (!garrison.empty() || can_conquer_province) {
			if (!garrison.empty()) {
				auto defending_army = make_qunique<army>(garrison, std::monostate());
				auto battle = make_qunique<metternich::battle>(this, defending_army.get(), QSize());
				const metternich::domain *scope = this->get_domain();
				if (defending_army->get_domain() == game::get()->get_player_country()) {
					scope = defending_army->get_domain();
				}
				battle->set_scope(scope);
				context battle_ctx;
				battle_ctx.root_scope = scope;
				battle_ctx.in_combat = true;
				battle->set_context(battle_ctx);

				battle->initialize();

				QFuture<bool> success_future = battle->get_future();

				if (this->get_domain() == game::get()->get_player_country() || defending_army->get_domain() == game::get()->get_player_country()) {
					game::get()->set_current_combat(std::move(battle));
				} else {
					QTimer::singleShot(0, [battle = std::move(battle)]() -> QCoro::Task<void> {
						co_await battle->start_coro();
					});
				}

				success = co_await success_future;
			} else {
				success = true;

				const portrait *war_minister_portrait = game::get()->get_player_country()->get_government()->get_war_minister_portrait();
				if (this->get_domain() == game::get()->get_player_country()) {
					engine_interface::get()->add_notification("Victory!", war_minister_portrait, std::format("We have won a battle in {}!", target_province->get_game_data()->get_current_cultural_name()));
				}
			}

			if (success && can_conquer_province) {
				target_province->get_game_data()->set_owner(this->get_domain());
			}
		}

		if (success) {
			for (military_unit *military_unit : this->get_military_units()) {
				military_unit->set_province(target_province);
			}
		}
	} else if (this->get_target_site() != nullptr) {
		const site *target_site = this->get_target_site();
		site_game_data *target_site_game_data = target_site->get_game_data();
		if (target_site_game_data->can_be_visited_by(this->get_domain())) {
			std::unique_ptr<party> party = this->to_party();
			if (!party->get_characters().empty()) {
				target_site_game_data->explore_dungeon(std::move(party));
			}
		}
	}
}

QVariantList army::get_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_military_units());
}

void army::add_military_unit(military_unit *military_unit)
{
	if (vector::contains(this->get_military_units(), military_unit)) {
		throw std::runtime_error(std::format("Tried to add military unit \"{}\" to an army, but it is already a part of it.", military_unit->get_name()));
	}

	this->military_units.push_back(military_unit);

	military_unit->set_army(this);

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void army::remove_military_unit(military_unit *military_unit)
{
	if (!vector::contains(this->get_military_units(), military_unit)) {
		throw std::runtime_error(std::format("Tried to remove military unit \"{}\" from an army, but it is not a part of it.", military_unit->get_name()));
	}

	std::erase(this->military_units, military_unit);

	military_unit->set_army(nullptr);

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

int army::get_score() const
{
	int score = 0;

	for (const military_unit *military_unit : this->get_military_units()) {
		score += military_unit->get_score();
	}

	return score;
}

const character *army::get_commander() const
{
	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_character() == nullptr) {
			continue;
		}

		return military_unit->get_character();
	}

	return nullptr;
}

std::unique_ptr<party> army::to_party() const
{
	std::vector<const character *> characters;

	for (const military_unit *military_unit : this->get_military_units()) {
		assert_throw(military_unit->get_character() != nullptr);
		characters.push_back(military_unit->get_character());
	}

	return std::make_unique<party>(characters);
}

}
