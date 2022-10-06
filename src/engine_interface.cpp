#include "metternich.h"

#include "engine_interface.h"

#include "map/map_template.h"
#include "util/exception_util.h"

namespace metternich {

engine_interface::engine_interface()
{
	database::get()->register_on_initialization_function([]() {
		engine_interface::get()->set_running(true);
	});
}

engine_interface::~engine_interface()
{
}

QObject *engine_interface::get_map_template(const QString &identifier) const
{
	try {
		return map_template::get(identifier.toStdString());
	} catch (const std::exception &exception) {
		exception::report(exception);
		return nullptr;
	}
}

}
