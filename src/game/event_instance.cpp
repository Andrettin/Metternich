#include "metternich.h"

#include "game/event_instance.h"

#include "engine_interface.h"
#include "game/event.h"
#include "game/event_option.h"
#include "game/game.h"
#include "script/context.h"

namespace metternich {

event_instance::event_instance(const metternich::event *event, const QString &name, const QString &description, const context &ctx)
	: event(event), name(name), description(description), ctx(ctx)
{
	if (event->get_option_count() > 0) {
		for (int i = 0; i < event->get_option_count(); ++i) {
			this->options.push_back(QString::fromStdString(event->get_option_name(i)));
			this->option_tooltips.push_back(QString::fromStdString(event->get_option_tooltip(i, ctx)));
		}
	} else {
		this->options.push_back(QString::fromStdString(event::option_default_name));
	}
}

void event_instance::choose_option(const int option_index)
{
	if (this->event->get_option_count() > 0) {
		this->event->do_option_effects(option_index, this->ctx);
	}

	engine_interface::get()->remove_event_instance(this);
}

}
