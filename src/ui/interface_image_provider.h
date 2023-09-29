#pragma once

#include "util/image_provider_base.h"

namespace metternich {

class interface_image_provider final : public image_provider_base
{
public:
	interface_image_provider();

	virtual QCoro::Task<void> load_image(const std::string id) override;
};

}
