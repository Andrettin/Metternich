#include "metternich.h"

#include "game/event_instance.h"

#include "engine_interface.h"
#include "game/event.h"
#include "game/event_option.h"
#include "game/game.h"
#include "script/context.h"
#include "util/assert_util.h"

namespace metternich {

event_instance::event_instance(const metternich::event *event, const QString &name, const QString &description, const context &ctx)
	: event(event), name(name), description(description), ctx(ctx)
{
	if (event->get_option_count() > 0) {
		for (int i = 0; i < event->get_option_count(); ++i) {
			if (!event->is_option_available(i, ctx)) {
				continue;
			}

			this->option_indexes.push_back(i);
			this->option_names.push_back(QString::fromStdString(event->get_option_name(i)));
			this->option_tooltips.push_back(QString::fromStdString(event->get_option_tooltip(i, ctx)));
		}

		assert_throw(!this->option_indexes.empty());
	} else {
		this->option_names.push_back(QString::fromStdString(event::option_default_name));
	}
}

void event_instance::choose_option(const int displayed_option_index)
{
	if (this->event->get_option_count() > 0) {
		const int option_index = this->option_indexes.at(displayed_option_index);
		this->event->do_option_effects(option_index, this->ctx);
	}

	engine_interface::get()->remove_event_instance(this);
}

}
