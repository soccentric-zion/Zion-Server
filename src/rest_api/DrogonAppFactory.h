#pragma once
// rest_api/DrogonAppFactory.h — configures and returns the Drogon app.

#include "config/Settings.h"
#include "services/AuthService.h"
#include "services/RunService.h"
#include "services/LogService.h"
#include "services/ModuleService.h"
#include "services/TargetService.h"
#include "services/ConfigService.h"
#include "services/TemplateService.h"
#include "services/ScheduleService.h"
#include "services/OtaService.h"
#include "services/NotificationService.h"
#include "services/CompareService.h"

#include <drogon/drogon.h>

namespace zion::rest {

// Configure Drogon routes, filters, and listeners from settings.
// Call before app().run().
void configure_drogon_app(const Settings&          settings,
                           AuthService&             auth_svc,
                           RunService&              run_svc,
                           LogService&              log_svc,
                           ModuleService&           mod_svc,
                           TargetService&           target_svc,
                           ConfigService&           config_svc,
                           TemplateService&         template_svc,
                           ScheduleService&         schedule_svc,
                           OtaService&              ota_svc,
                           NotificationService&     notif_svc,
                           CompareService&          cmp_svc);

}  // namespace zion::rest
