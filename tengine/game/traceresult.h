#ifndef TINKER_TRACE_RESULT_H
#define TINKER_TRACE_RESULT_H

#include <tengine_config.h>

class CTraceResult : public TCollisionResult
{
public:
	CTraceResult()
	{
		pHit = NULL;
	}

public:
	class CBaseEntity*	pHit;
};

#endif
