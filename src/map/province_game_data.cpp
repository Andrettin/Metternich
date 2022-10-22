#include "metternich.h"

#include "map/province_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "game/game.h"
#include "map/map.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"

namespace metternich {

void province_game_data::set_owner(const country *country)
{
	if (country == this->get_owner()) {
		return;
	}

	const metternich::country *old_owner = this->owner;
	if (old_owner != nullptr) {
		old_owner->get_game_data()->remove_province(this->province);
	}

	this->owner = country;

	if (this->owner != nullptr) {
		this->owner->get_game_data()->add_province(this->province);
	}

	if (game::get()->is_running()) {
		if (old_owner == nullptr || this->owner == nullptr || old_owner->get_culture() != this->owner->get_culture()) {
			emit culture_changed();

			if (this->province->get_capital_settlement() != nullptr) {
				emit this->province->get_capital_settlement()->get_game_data()->culture_changed();
			}

			for (const QPoint &tile_pos : this->tiles) {
				const tile *tile = map::get()->get_tile(tile_pos);
				if (tile->get_site() != nullptr && tile->get_site() != this->province->get_capital_settlement()) {
					emit tile->get_site()->get_game_data()->culture_changed();
				}
			}
		}
	}
}

const culture *province_game_data::get_culture() const
{
	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_culture();
	}

	return nullptr;
}

const std::string &province_game_data::get_current_cultural_name() const
{
	return this->province->get_cultural_name(this->get_culture());
}

void province_game_data::add_tile(const QPoint &tile_pos)
{
	this->tiles.push_back(tile_pos);

	const tile *tile = map::get()->get_tile(tile_pos);
	if (tile->get_resource() != nullptr) {
		++this->resource_counts[tile->get_resource()];
	}
}

void province_game_data::add_border_tile(const QPoint &tile_pos)
{
	this->border_tiles.push_back(tile_pos);

	if (this->get_territory_rect().isNull()) {
		this->territory_rect = QRect(tile_pos, QSize(1, 1));
	} else {
		if (tile_pos.x() < this->territory_rect.x()) {
			this->territory_rect.setX(tile_pos.x());
		} else if (tile_pos.x() > this->territory_rect.right()) {
			this->territory_rect.setRight(tile_pos.x());
		}
		if (tile_pos.y() < this->territory_rect.y()) {
			this->territory_rect.setY(tile_pos.y());
		} else if (tile_pos.y() > this->territory_rect.bottom()) {
			this->territory_rect.setBottom(tile_pos.y());
		}
	}
}

}
