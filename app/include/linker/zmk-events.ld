#include <linker/linker-defs.h>

	SECTION_PROLOGUE(event_types,,)
	{
        	__event_type_start = .; \
        	KEEP(*(".event_type")); \
        	__event_type_end = .; \
	} GROUP_LINK_IN(ROMABLE_REGION)

        SECTION_PROLOGUE(event_subscriptions,,)
	{
        	__event_subscriptions_start = .; \
        	KEEP(*(".event_subscription")); \
        	__event_subscriptions_end = .; \
	} GROUP_LINK_IN(ROMABLE_REGION)

