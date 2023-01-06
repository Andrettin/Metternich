#include "metternich.h"

#include "game/event_instance.h"

#include "engine_interface.h"
#include "game/event.h"
#include "game/event_option.h"
#include "game/game.h"
#include "script/context.h"

namespace metternich {

event_instance::event_instance(const metternich::event *event, const QString &name, const QString &description)
	: event(event), name(name), description(description)
{
	read_only_context ctx;
	ctx.current_country = game::get()->get_player_country();

	if (!event->get_options().empty()) {
		for (const std::unique_ptr<event_option> &option : event->get_options()) {
			this->options.push_back(QString::fromStdString(option->get_name()));
			this->option_tooltips.push_back(QString::fromStdString(option->get_tooltip(ctx)));
		}
	} else {
		this->options.push_back(QString::fromStdString(event_option::default_name));
	}
}

void event_instance::choose_option(const int option_index)
{
	const country *country = game::get()->get_player_country();

	context ctx;
	ctx.current_country = country;

	if (!this->event->get_options().empty()) {
		this->event->get_options().at(option_index)->do_effects(country, ctx);
	}

	engine_interface::get()->remove_event_instance(this);
}

}
