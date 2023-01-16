#include "metternich.h"

#include "country/landed_title_game_data.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "country/landed_title.h"
#include "game/game.h"
#include "util/assert_util.h"
#include "util/gender.h"

namespace metternich {

landed_title_game_data::landed_title_game_data(const metternich::landed_title *landed_title)
	: landed_title(landed_title), tier(landed_title->get_default_tier())
{
	connect(this, &landed_title_game_data::tier_changed, this, &landed_title_game_data::title_name_changed);
	connect(this, &landed_title_game_data::tier_changed, this, &landed_title_game_data::ruler_title_name_changed);
	connect(this, &landed_title_game_data::holder_changed, this, &landed_title_game_data::ruler_title_name_changed);
}

const std::string &landed_title_game_data::get_title_name() const
{
	return this->landed_title->get_title_name(this->tier);
}

const std::string &landed_title_game_data::get_ruler_title_name() const
{
	const gender gender = this->get_holder() != nullptr ? this->get_holder()->get_gender() : gender::male;
	return this->landed_title->get_ruler_title_name(this->tier, gender);
}

void landed_title_game_data::set_tier(const landed_title_tier tier)
{
	if (tier == this->get_tier()) {
		return;
	}

	assert_throw(tier >= this->landed_title->get_min_tier());
	assert_throw(tier <= this->landed_title->get_max_tier());

	this->tier = tier;

	if (game::get()->is_running()) {
		emit tier_changed();
	}
}

void landed_title_game_data::set_holder(const character *holder)
{
	if (holder == this->get_holder()) {
		return;
	}

	if (this->holder != nullptr) {
		this->holder->get_game_data()->remove_landed_title(this->landed_title);
	}

	this->holder = holder;

	if (this->holder != nullptr) {
		this->holder->get_game_data()->add_landed_title(this->landed_title);
	}

	if (game::get()->is_running()) {
		emit holder_changed();
	}
}

}
