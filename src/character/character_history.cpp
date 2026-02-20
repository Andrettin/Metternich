#include "metternich.h"

#include "character/character_history.h"

#include "character/character.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "character/trait.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "util/assert_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

character_history::character_history(const metternich::character *character) : character(character)
{
	this->level = character->get_level();
	this->traits = character->get_traits();
}

void character_history::process_gsml_property(const gsml_property &property, const QDate &date)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "rank") {
		assert_throw(this->character->get_character_class() != nullptr);
		this->level = this->character->get_character_class()->get_rank_level(value);
	} else if (key == "trait") {
		const trait *trait = trait::get(value);
		this->traits.push_back(trait);
	} else {
		data_entry_history::process_gsml_property(property, date);
	}
}

void character_history::set_spouse(const metternich::character *spouse)
{
	if (spouse == this->get_spouse()) {
		return;
	}

	const metternich::character *old_spouse = this->get_spouse();

	this->spouse = spouse;

	if (old_spouse != nullptr) {
		old_spouse->get_history()->set_spouse(nullptr);
	}

	if (spouse != nullptr) {
		spouse->get_history()->set_spouse(this->character);
	}
}

void character_history::set_heir(const metternich::character *heir)
{
	if (heir == this->get_heir()) {
		return;
	}

	const metternich::character *old_heir = this->get_heir();

	this->heir = heir;

	if (old_heir != nullptr) {
		old_heir->get_history()->remove_predecessor(this->character);
	}

	if (heir != nullptr) {
		heir->get_history()->add_predecessor(this->character);
	}
}

void character_history::calculate_heir()
{
	if (this->get_heir() != nullptr) {
		return;
	}

	std::vector<const metternich::character *> potential_heirs;

	for (const metternich::domain *ruled_domain : this->character->get_game_data()->get_ruled_domains()) {
		bool found_self = false;
		const metternich::character *successor = nullptr;
		for (const auto &[date, historical_ruler] : ruled_domain->get_game_data()->get_historical_rulers()) {
			if (historical_ruler == this->character) {
				found_self = true;
				continue;
			}

			if (found_self) {
				successor = historical_ruler;
				break;
			}
		}

		if (successor != nullptr) {
			if ((successor->get_dynasty() != nullptr && successor->get_dynasty() == this->character->get_dynasty()) || vector::contains(this->character->get_children(), successor)) {
				potential_heirs.push_back(successor);
			}
		}
	}

	if (!potential_heirs.empty()) {
		this->set_heir(vector::get_random(potential_heirs));
	}
}

}
