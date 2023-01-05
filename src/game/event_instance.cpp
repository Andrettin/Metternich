#include "metternich.h"

#include "game/event_instance.h"

#include "engine_interface.h"

namespace metternich {

void event_instance::choose_option(const int option_index)
{
	Q_UNUSED(option_index);

	engine_interface::get()->remove_event_instance(this);
}

}
